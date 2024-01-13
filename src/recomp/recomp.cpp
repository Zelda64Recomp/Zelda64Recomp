#ifdef _WIN32
#include <Windows.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <cmath>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include "recomp.h"
#include "../ultramodern/ultramodern.hpp"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
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

extern "C" void _bzero(uint8_t* rdram, recomp_context* ctx) {
    gpr start_addr = ctx->r4;
    gpr size = ctx->r5;

    for (uint32_t i = 0; i < size; i++) {
        MEM_B(start_addr, i) = 0;
    }
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

void do_rom_read(uint8_t* rdram, gpr ram_address, uint32_t dev_address, size_t num_bytes);

std::unique_ptr<uint8_t[]> rom;
size_t rom_size;

// Recomp generation functions
extern "C" void recomp_entrypoint(uint8_t * rdram, recomp_context * ctx);
gpr get_entrypoint_address();
const char* get_rom_name();
void init_overlays();
extern "C" void load_overlays(uint32_t rom, int32_t ram_addr, uint32_t size);
extern "C" void unload_overlays(int32_t ram_addr, uint32_t size);

std::unique_ptr<uint8_t[]> rdram_buffer;
recomp_context context{};

void read_patch_data(uint8_t* rdram, gpr patch_data_address) {
    const char patches_data_file_path[] = "patches/patches.bin";
    std::ifstream patches_data_file{ patches_data_file_path, std::ios::binary };

    if (!patches_data_file) {
        fprintf(stderr, "Failed to open patches data file: %s\n", patches_data_file_path);
        exit(EXIT_FAILURE);
    }

    patches_data_file.seekg(0, std::ios::end);
    size_t patches_data_size = patches_data_file.tellg();
    patches_data_file.seekg(0, std::ios::beg);

    std::unique_ptr<uint8_t[]> patches_data = std::make_unique<uint8_t[]>(patches_data_size);

    patches_data_file.read(reinterpret_cast<char*>(patches_data.get()), patches_data_size);

    for (size_t i = 0; i < patches_data_size; i++) {
        MEM_B(i, patch_data_address) = patches_data[i];
    }
}

EXPORT extern "C" void init() {
    {
        std::ifstream rom_file{ get_rom_name(), std::ios::binary };

        size_t iobuf_size = 0x100000;
        std::unique_ptr<char[]> iobuf = std::make_unique<char[]>(iobuf_size);
        rom_file.rdbuf()->pubsetbuf(iobuf.get(), iobuf_size);

        if (!rom_file) {
            fprintf(stderr, "Failed to open rom: %s\n", get_rom_name());
            exit(EXIT_FAILURE);
        }

        rom_file.seekg(0, std::ios::end);
        rom_size = rom_file.tellg();
        rom_file.seekg(0, std::ios::beg);

        rom = std::make_unique<uint8_t[]>(rom_size);

        rom_file.read(reinterpret_cast<char*>(rom.get()), rom_size);
    }

    // Initialize the overlays
    init_overlays();

    // Get entrypoint from recomp function
    gpr entrypoint = get_entrypoint_address();

    // Load overlays in the first 1MB
    load_overlays(0x1000, (int32_t)entrypoint, 1024 * 1024);

    // Allocate rdram_buffer
    rdram_buffer = std::make_unique<uint8_t[]>(ultramodern::rdram_size);
    std::memset(rdram_buffer.get(), 0, ultramodern::rdram_size);

    // Initial 1MB DMA (rom address 0x1000 = physical address 0x10001000)
    do_rom_read(rdram_buffer.get(), entrypoint, 0x10001000, 0x100000);

    // Read in any extra data from patches
    read_patch_data(rdram_buffer.get(), (gpr)(s32)0x80800100);

    // Set up stack pointer
    context.r29 = 0xFFFFFFFF803FFFF0u;

    // Set up context floats
    context.f_odd = &context.f0.u32h;
    context.mips3_float_mode = false;

    // Initialize variables normally set by IPL3
    constexpr int32_t osTvType = 0x80000300;
    constexpr int32_t osRomType = 0x80000304;
    constexpr int32_t osRomBase = 0x80000308;
    constexpr int32_t osResetType = 0x8000030c;
    constexpr int32_t osCicId = 0x80000310;
    constexpr int32_t osVersion = 0x80000314;
    constexpr int32_t osMemSize = 0x80000318;
    constexpr int32_t osAppNMIBuffer = 0x8000031c;
    uint8_t *rdram = rdram_buffer.get();
    MEM_W(osTvType, 0) = 1; // NTSC
    MEM_W(osRomBase, 0) = 0xB0000000u; // standard rom base
    MEM_W(osResetType, 0) = 0; // cold reset
    MEM_W(osMemSize, 0) = 8 * 1024 * 1024; // 8MB
}

std::atomic_int game_started = -1;

void ultramodern::start_game(int game) {
    game_started.store(game);
    game_started.notify_all();
}

bool ultramodern::is_game_started() {
    return game_started.load() != -1;
}

void set_audio_callbacks(const ultramodern::audio_callbacks_t& callbacks);
void set_input_callbacks(const ultramodern::input_callbacks_t& callback);

std::atomic_bool exited = false;

void ultramodern::quit() {
    exited.store(true);
    int desired = -1;
    game_started.compare_exchange_strong(desired, -2);
    game_started.notify_all();
}

void ultramodern::start(WindowHandle window_handle, const audio_callbacks_t& audio_callbacks, const input_callbacks_t& input_callbacks, const gfx_callbacks_t& gfx_callbacks_) {
    set_audio_callbacks(audio_callbacks);
    set_input_callbacks(input_callbacks);

    gfx_callbacks_t gfx_callbacks = gfx_callbacks_;

    gfx_callbacks_t::gfx_data_t gfx_data{};

    if (gfx_callbacks.create_gfx) {
        gfx_data = gfx_callbacks.create_gfx();
    }

    if (window_handle == WindowHandle{}) {
        if (gfx_callbacks.create_window) {
            window_handle = gfx_callbacks.create_window(gfx_data);
        }
        else {
            assert(false && "No create_window callback provided");
        }
    }

    std::thread game_thread{[](ultramodern::WindowHandle window_handle) {
        debug_printf("[Recomp] Starting\n");
        
        ultramodern::set_native_thread_name("Game Start Thread");

        ultramodern::preinit(rdram_buffer.get(), rom.get(), window_handle);

        game_started.wait(-1);

        switch (game_started.load()) {
            case 0:
                recomp_entrypoint(rdram_buffer.get(), &context);
                break;
            case -2:
                break;
        }
        
        debug_printf("[Recomp] Quitting\n");
    }, window_handle};

    while (!exited) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1ms);
        if (gfx_callbacks.update_gfx != nullptr) {
            gfx_callbacks.update_gfx(gfx_data);
        }
    }
    game_thread.join();
    ultramodern::join_event_threads();
}
