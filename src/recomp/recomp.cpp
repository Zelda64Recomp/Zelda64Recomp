#ifdef _WIN32
#include <Windows.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include "recomp.h"
#include "recomp_overlays.h"
#include "recomp_game.h"
#include "recomp_config.h"
#include "xxHash/xxh3.h"
#include "../ultramodern/ultramodern.hpp"
#include "../../RecompiledPatches/patches_bin.h"
#ifdef HAS_MM_SHADER_CACHE
#include "mm_shader_cache.h"
#endif

#ifdef _MSC_VER
inline uint32_t byteswap(uint32_t val) {
    return _byteswap_ulong(val);
}
#else
constexpr uint32_t byteswap(uint32_t val) {
    return __builtin_bswap32(val);
}
#endif

struct RomEntry {
    uint64_t xxhash3_value;
    std::u8string stored_filename;
    std::string internal_rom_name;
};

const std::unordered_map<recomp::Game, RomEntry> game_roms {
    { recomp::Game::MM, { 0xEF18B4A9E2386169ULL, std::u8string{recomp::mm_game_id} + u8".z64", "ZELDA MAJORA'S MASK" }},
};

bool check_hash(const std::vector<uint8_t>& rom_data, uint64_t expected_hash) {
    uint64_t calculated_hash = XXH3_64bits(rom_data.data(), rom_data.size());

    return calculated_hash == expected_hash;
}

static std::vector<uint8_t> read_file(const std::filesystem::path& path) {
    std::vector<uint8_t> ret;

    std::ifstream file{ path, std::ios::binary};

    if (file.good()) {
        file.seekg(0, std::ios::end);
        ret.resize(file.tellg());
        file.seekg(0, std::ios::beg);

        file.read(reinterpret_cast<char*>(ret.data()), ret.size());
    }

    return ret;
}

bool write_file(const std::filesystem::path& path, const std::vector<uint8_t>& data) {
    std::ofstream out_file{ path, std::ios::binary };

    if (!out_file.good()) {
        return false;
    }

    out_file.write(reinterpret_cast<const char*>(data.data()), data.size());

    return true;
}

bool check_stored_rom(const RomEntry& game_entry) {

    std::vector stored_rom_data = read_file(recomp::get_app_folder_path() / game_entry.stored_filename);

    if (!check_hash(stored_rom_data, game_entry.xxhash3_value)) {
        // Incorrect hash, remove the stored ROM file if it exists.
        std::filesystem::remove(recomp::get_app_folder_path() / game_entry.stored_filename);
        return false;
    }

    return true;
}

static std::unordered_set<recomp::Game> valid_game_roms;

bool recomp::is_rom_valid(recomp::Game game) {
    return valid_game_roms.contains(game);
}

void recomp::check_all_stored_roms() {
    for (const auto& cur_rom_entry: game_roms) {
        if (check_stored_rom(cur_rom_entry.second)) {
            valid_game_roms.insert(cur_rom_entry.first);
        }
    }
}

bool recomp::load_stored_rom(recomp::Game game) {
    auto find_it = game_roms.find(game);

    if (find_it == game_roms.end()) {
        return false;
    }
    
    std::vector<uint8_t> stored_rom_data = read_file(recomp::get_app_folder_path() / find_it->second.stored_filename);

    if (!check_hash(stored_rom_data, find_it->second.xxhash3_value)) {
        // The ROM no longer has the right hash, delete it.
        std::filesystem::remove(recomp::get_app_folder_path() / find_it->second.stored_filename);
        return false;
    }

    recomp::set_rom_contents(std::move(stored_rom_data));
    return true;
}

const std::array<uint8_t, 4> first_rom_bytes { 0x80, 0x37, 0x12, 0x40 };

enum class ByteswapType {
    NotByteswapped,
    Byteswapped4,
    Byteswapped2,
    Invalid
};

ByteswapType check_rom_start(const std::vector<uint8_t>& rom_data) {
    if (rom_data.size() < 4) {
        return ByteswapType::Invalid;
    }

    bool matched_all = true;

    auto check_match = [&](uint8_t index0, uint8_t index1, uint8_t index2, uint8_t index3) {
        return
            rom_data[0] == first_rom_bytes[index0] &&
            rom_data[1] == first_rom_bytes[index1] &&
            rom_data[2] == first_rom_bytes[index2] &&
            rom_data[3] == first_rom_bytes[index3];
    };

    // Check if the ROM is already in the correct byte order.
    if (check_match(0,1,2,3)) {
        return ByteswapType::NotByteswapped;
    }

    // Check if the ROM has been byteswapped in groups of 4 bytes.
    if (check_match(3,2,1,0)) {
        return ByteswapType::Byteswapped4;
    }

    // Check if the ROM has been byteswapped in groups of 2 bytes.
    if (check_match(1,0,3,2)) {
        return ByteswapType::Byteswapped2;
    }

    // No match found.
    return ByteswapType::Invalid;
}

void byteswap_data(std::vector<uint8_t>& rom_data, size_t index_xor) {
    for (size_t rom_pos = 0; rom_pos < rom_data.size(); rom_pos += 4) {
        uint8_t temp0 = rom_data[rom_pos + 0];
        uint8_t temp1 = rom_data[rom_pos + 1];
        uint8_t temp2 = rom_data[rom_pos + 2];
        uint8_t temp3 = rom_data[rom_pos + 3];

        rom_data[rom_pos + (0 ^ index_xor)] = temp0;
        rom_data[rom_pos + (1 ^ index_xor)] = temp1;
        rom_data[rom_pos + (2 ^ index_xor)] = temp2;
        rom_data[rom_pos + (3 ^ index_xor)] = temp3;
    }
}

recomp::RomValidationError recomp::select_rom(const std::filesystem::path& rom_path, Game game) {
    auto find_it = game_roms.find(game);

    if (find_it == game_roms.end()) {
        return recomp::RomValidationError::OtherError;
    }

    const RomEntry& game_entry = find_it->second;

    std::vector<uint8_t> rom_data = read_file(rom_path);

    if (rom_data.empty()) {
        return recomp::RomValidationError::FailedToOpen;
    }

    // Pad the rom to the nearest multiple of 4 bytes.
    rom_data.resize((rom_data.size() + 3) & ~3);

    ByteswapType byteswap_type = check_rom_start(rom_data);

    switch (byteswap_type) {
        case ByteswapType::Invalid:
            return recomp::RomValidationError::NotARom;
        case ByteswapType::Byteswapped2:
            byteswap_data(rom_data, 1);
            break;
        case ByteswapType::Byteswapped4:
            byteswap_data(rom_data, 3);
            break;
        case ByteswapType::NotByteswapped:
            break;
    }

    if (!check_hash(rom_data, game_entry.xxhash3_value)) {
        const std::string_view name{ reinterpret_cast<const char*>(rom_data.data()) + 0x20, game_entry.internal_rom_name.size()};
        if (name == game_entry.internal_rom_name) {
            return recomp::RomValidationError::IncorrectVersion;
        }
        else {
            if (game == recomp::Game::MM && std::string_view{ reinterpret_cast<const char*>(rom_data.data()) + 0x20, 19 } == "THE LEGEND OF ZELDA") {
                return recomp::RomValidationError::NotYet;
            }
            else {
                return recomp::RomValidationError::IncorrectRom;
            }
        }
    }

    write_file(recomp::get_app_folder_path() / game_entry.stored_filename, rom_data);
    
    return recomp::RomValidationError::Good;
}

extern "C" void osGetMemSize_recomp(uint8_t * rdram, recomp_context * ctx) {
    ctx->r2 = 8 * 1024 * 1024;
}

enum class StatusReg {
    FR = 0x04000000,
};

extern "C" void cop0_status_write(recomp_context* ctx, gpr value) {
    uint32_t old_sr = ctx->status_reg;
    uint32_t new_sr = (uint32_t)value;
    uint32_t changed = old_sr ^ new_sr;

    // Check if the FR bit changed
    if (changed & (uint32_t)StatusReg::FR) {
        // Check if the FR bit was set
        if (new_sr & (uint32_t)StatusReg::FR) {
            // FR = 1, odd single floats point to their own registers
            ctx->f_odd = &ctx->f1.u32l;
            ctx->mips3_float_mode = true;
        }
        // Otherwise, it was cleared
        else {
            // FR = 0, odd single floats point to the upper half of the previous register
            ctx->f_odd = &ctx->f0.u32h;
            ctx->mips3_float_mode = false;
        }

        // Remove the FR bit from the changed bits as it's been handled
        changed &= ~(uint32_t)StatusReg::FR;
    }

    // If any other bits were changed, assert false as they're not handled currently
    if (changed) {
        printf("Unhandled status register bits changed: 0x%08X\n", changed);
        assert(false);
        exit(EXIT_FAILURE);
    }
    
    // Update the status register in the context
    ctx->status_reg = new_sr;
}

extern "C" gpr cop0_status_read(recomp_context* ctx) {
    return (gpr)(int32_t)ctx->status_reg;
}

extern "C" void switch_error(const char* func, uint32_t vram, uint32_t jtbl) {
    printf("Switch-case out of bounds in %s at 0x%08X for jump table at 0x%08X\n", func, vram, jtbl);
    assert(false);
    exit(EXIT_FAILURE);
}

extern "C" void do_break(uint32_t vram) {
    printf("Encountered break at original vram 0x%08X\n", vram);
    assert(false);
    exit(EXIT_FAILURE);
}

void run_thread_function(uint8_t* rdram, uint64_t addr, uint64_t sp, uint64_t arg) {
    recomp_context ctx{};
    ctx.r29 = sp;
    ctx.r4 = arg;
    ctx.mips3_float_mode = 0;
    ctx.f_odd = &ctx.f0.u32h;
    recomp_func_t* func = get_function(addr);
    func(rdram, &ctx);
}

// Recomp generation functions
extern "C" void recomp_entrypoint(uint8_t * rdram, recomp_context * ctx);
gpr get_entrypoint_address();
const char* get_rom_name();

void read_patch_data(uint8_t* rdram, gpr patch_data_address) {
    for (size_t i = 0; i < sizeof(mm_patches_bin); i++) {
        MEM_B(i, patch_data_address) = mm_patches_bin[i];
    }
}

void init(uint8_t* rdram, recomp_context* ctx) {
    // Initialize the overlays
    init_overlays();

    // Get entrypoint from recomp function
    gpr entrypoint = get_entrypoint_address();

    // Load overlays in the first 1MB
    load_overlays(0x1000, (int32_t)entrypoint, 1024 * 1024);

    // Initial 1MB DMA (rom address 0x1000 = physical address 0x10001000)
    recomp::do_rom_read(rdram, entrypoint, 0x10001000, 0x100000);

    // Read in any extra data from patches
    read_patch_data(rdram, (gpr)(s32)0x80801000);

    // Set up stack pointer
    ctx->r29 = 0xFFFFFFFF803FFFF0u;

    // Set up context floats
    ctx->f_odd = &ctx->f0.u32h;
    ctx->mips3_float_mode = false;

    // Initialize variables normally set by IPL3
    constexpr int32_t osTvType = 0x80000300;
    constexpr int32_t osRomType = 0x80000304;
    constexpr int32_t osRomBase = 0x80000308;
    constexpr int32_t osResetType = 0x8000030c;
    constexpr int32_t osCicId = 0x80000310;
    constexpr int32_t osVersion = 0x80000314;
    constexpr int32_t osMemSize = 0x80000318;
    constexpr int32_t osAppNMIBuffer = 0x8000031c;
    MEM_W(osTvType, 0) = 1; // NTSC
    MEM_W(osRomBase, 0) = 0xB0000000u; // standard rom base
    MEM_W(osResetType, 0) = 0; // cold reset
    MEM_W(osMemSize, 0) = 8 * 1024 * 1024; // 8MB
}

std::atomic<recomp::Game> game_started = recomp::Game::None;

void recomp::start_game(recomp::Game game) {
    game_started.store(game);
    game_started.notify_all();
}

bool ultramodern::is_game_started() {
    return game_started.load() != recomp::Game::None;
}

void set_audio_callbacks(const ultramodern::audio_callbacks_t& callbacks);
void set_input_callbacks(const ultramodern::input_callbacks_t& callback);

std::atomic_bool exited = false;

void ultramodern::quit() {
    exited.store(true);
    recomp::Game desired = recomp::Game::None;
    game_started.compare_exchange_strong(desired, recomp::Game::Quit);
    game_started.notify_all();
}

void recomp::start(ultramodern::WindowHandle window_handle, const ultramodern::audio_callbacks_t& audio_callbacks, const ultramodern::input_callbacks_t& input_callbacks, const ultramodern::gfx_callbacks_t& gfx_callbacks_) {
    recomp::check_all_stored_roms();
    set_audio_callbacks(audio_callbacks);
    set_input_callbacks(input_callbacks);

    ultramodern::gfx_callbacks_t gfx_callbacks = gfx_callbacks_;

    ultramodern::gfx_callbacks_t::gfx_data_t gfx_data{};

    if (gfx_callbacks.create_gfx) {
        gfx_data = gfx_callbacks.create_gfx();
    }

    if (window_handle == ultramodern::WindowHandle{}) {
        if (gfx_callbacks.create_window) {
            window_handle = gfx_callbacks.create_window(gfx_data);
        }
        else {
            assert(false && "No create_window callback provided");
        }
    }

    // Allocate rdram_buffer
    std::unique_ptr<uint8_t[]> rdram_buffer = std::make_unique<uint8_t[]>(ultramodern::rdram_size);
    std::memset(rdram_buffer.get(), 0, ultramodern::rdram_size);

    std::thread game_thread{[](ultramodern::WindowHandle window_handle, uint8_t* rdram) {
        debug_printf("[Recomp] Starting\n");
        
        ultramodern::set_native_thread_name("Game Start Thread");
        
        ultramodern::preinit(rdram, window_handle);

        game_started.wait(recomp::Game::None);
        recomp_context context{};

        switch (game_started.load()) {
            case recomp::Game::MM:
                if (!recomp::load_stored_rom(recomp::Game::MM)) {
                    recomp::message_box("Error opening stored ROM! Please restart this program.");
                }

                #ifdef HAS_MM_SHADER_CACHE
                ultramodern::load_shader_cache({mm_shader_cache_bytes, sizeof(mm_shader_cache_bytes)});
                #endif

                init(rdram, &context);
                try {
                    recomp_entrypoint(rdram, &context);
                } catch (ultramodern::thread_terminated& terminated) {

                } 
                break;
            case recomp::Game::Quit:
                break;
        }
        
        debug_printf("[Recomp] Quitting\n");
    }, window_handle, rdram_buffer.get()};

    while (!exited) {
        ultramodern::sleep_milliseconds(1);
        if (gfx_callbacks.update_gfx != nullptr) {
            gfx_callbacks.update_gfx(gfx_data);
        }
    }
    game_thread.join();
    ultramodern::join_event_threads();
    ultramodern::join_thread_cleaner_thread();
    ultramodern::join_saving_thread();
}
