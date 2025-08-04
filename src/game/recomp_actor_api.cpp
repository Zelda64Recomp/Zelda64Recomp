#include <vector>
#include <mutex>

#include "slot_map.h"

#include "librecomp/helpers.hpp"
#include "librecomp/addresses.hpp"
#include "ultramodern/error_handling.hpp"
#include "recomp_ui.h"
#include "recomp_data.h"
#include "../patches/actor_funcs.h"

struct ExtensionInfo {
    // Either the actor's type ID, or 0xFFFFFFFF if this is for generic data.
    uint32_t actor_type;
    // The offset from either the start of the actor's data or the start of the actor's specific extension data depending on the value of actor_type.
    uint32_t data_offset;
};

struct ExtensionData {
    uint32_t actor_spawn_index;
    PTR(void) data_addr;
};

std::mutex actor_data_mutex{};
// The total size of actor-specific extension data for each actor type. 
std::vector<uint32_t> actor_data_sizes{};
// The total size of all generic actor extension data.
uint32_t generic_data_size;
// The registered actor extensions.
std::vector<ExtensionInfo> actor_extensions{};
// The extension data for every actor.
using actor_data_map_t = dod::slot_map32<ExtensionData>;
actor_data_map_t actor_data{};
// The number of actors spawned since the last reset.
uint32_t actor_spawn_count = 0;
// Whether or not extensions can be registered at this time.
bool can_register = false;

// Debug counters.
size_t alloc_count = 0;
size_t free_count = 0;

void recomputil::init_extended_actor_data() {
    std::lock_guard lock{ actor_data_mutex };

    actor_data_sizes.clear();
    generic_data_size = 0;
    actor_extensions.clear();
    actor_data.reset();
    actor_spawn_count = 0;
    can_register = true;
    // Create a dummy extension so the first extension handle is nonzero, should help catch bugs.
    actor_extensions.push_back({});
}

void recomputil::reset_actor_data() {
    std::lock_guard lock{ actor_data_mutex };
    actor_data.reset();
    actor_spawn_count = 0;

    assert(alloc_count == free_count);
    alloc_count = 0;
    free_count = 0;
}

constexpr uint32_t round_up_16(uint32_t value) {
    return (value + 15) & (~15);
}

extern "C" void recomp_register_actor_extension(uint8_t* rdram, recomp_context* ctx) {
    u32 actor_type = _arg<0, u32>(rdram, ctx);
    u32 size = _arg<1, u32>(rdram, ctx);

    if (!can_register) {
        recompui::message_box("Fatal error in mod - attempted to register actor extension data after actors have been spawned.");
        assert(false);
        ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
    }

    if (actor_data_sizes.size() <= actor_type) {
        actor_data_sizes.resize(actor_type + 1);
    }

    // Increase the actor type's extension data size by the provided size (rounded up to a multiple of 16).
    uint32_t data_offset = actor_data_sizes[actor_type];
    actor_data_sizes[actor_type] += round_up_16(size);

    // Register the extension.
    uint32_t ret = static_cast<uint32_t>(actor_extensions.size());
    actor_extensions.emplace_back(ExtensionInfo{.actor_type = actor_type, .data_offset = data_offset});

    // printf("Registered actor extension data for type %u (size 0x%08X, offset 0x%08X)\n", actor_type, size, data_offset);

    _return<u32>(ctx, ret);
}

extern "C" void recomp_register_actor_extension_generic(uint8_t* rdram, recomp_context* ctx) {
    u32 size = _arg<0, u32>(rdram, ctx);

    // Increase the generic extension data size by the provided size (rounded up to a multiple of 16).
    uint32_t data_offset = generic_data_size;
    generic_data_size += round_up_16(size);

    // Register the extension.
    uint32_t ret = static_cast<uint32_t>(actor_extensions.size());
    actor_extensions.emplace_back(ExtensionInfo{.actor_type = 0xFFFFFFFFU, .data_offset = data_offset});

    // printf("Registered generic actor extension data (size 0x%08X, offset 0x%08X)\n", size, data_offset);
    _return<u32>(ctx, ret);
}

extern "C" void recomp_clear_all_actor_data(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    (void)ctx;
    recomputil::reset_actor_data();
}

extern "C" void recomp_create_actor_data(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ actor_data_mutex };

    can_register = false;

    // Determine the number of bytes to allocate based on the actor type's extensions and the generic extensions.
    u32 actor_type = _arg<0, u32>(rdram, ctx);
    u32 alloc_size = generic_data_size;
    [[maybe_unused]] u32 type_data_size = 0; 

    if (actor_type < actor_data_sizes.size()) {
        type_data_size = actor_data_sizes[actor_type];
        alloc_size += type_data_size;
    }

    // Allocate the extension data if it's of nonzero size.
    PTR(void) data_ptr = NULLPTR;
    if (alloc_size != 0) {
        void* data = recomp::alloc(rdram, alloc_size);
        alloc_count++;
        data_ptr = reinterpret_cast<uint8_t*>(data) - rdram + 0xFFFFFFFF80000000U;
        // Zero the allocated memory.
        // A memset should be fine here since this data is aligned, but use a byteswapped loop just to be safe.
        for (size_t i = 0; i < alloc_size; i++) {
            MEM_B(i, data_ptr) = 0;
        }
    }

    // Add the actor's fields to the actor data slotmap.
    u32 spawn_index = actor_spawn_count++;
    dod::slot_map_key32<ExtensionData> key = actor_data.emplace(ExtensionData{.actor_spawn_index = spawn_index, .data_addr = data_ptr});

    // printf("Allocated actor data: address 0x%08X with 0x%08X bytes total (0x%08X bytes generic and 0x%08X bytes specific), handle 0x%08X, spawn index %d\n",
    //     data_ptr, alloc_size, generic_data_size, type_data_size, key.raw, spawn_index);

    _return<u32>(ctx, key.raw);
}

extern "C" void recomp_destroy_actor_data(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ actor_data_mutex };

    u32 actor_handle = _arg<0, u32>(rdram, ctx);
    actor_data_map_t::key actor_key{actor_handle};

    ExtensionData* data = actor_data.get(actor_key);
    if (data != nullptr) {
        // printf("Freeing actor data: address 0x%08X handle 0x%08X\n", data->data_addr, actor_handle);
        if (data->data_addr != NULLPTR) {
            recomp::free(rdram, TO_PTR(void, data->data_addr));
            free_count++;
        }
        actor_data.erase(actor_data_map_t::key{actor_handle});
    }
    else {
        // Not an irrecoverable error, but catch it in debug mode with an assert to help find bugs.
        assert(false);
    }
}

extern "C" void recomp_get_actor_data(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ actor_data_mutex };

    u32 actor_handle = _arg<0, u32>(rdram, ctx);
    u32 extension_handle = _arg<1, u32>(rdram, ctx);
    u32 actor_type = _arg<2, u32>(rdram, ctx);

    // Check if the extension handle is valid.
    if (extension_handle == 0 || extension_handle >= actor_extensions.size()) {
        _return<PTR(void)>(ctx, NULLPTR);
        return;
    }

    ExtensionInfo& extension = actor_extensions[extension_handle];
    bool generic_extension = extension.actor_type == 0xFFFFFFFFU;

    // Check if the extension is generic or for the provided actor type.
    if (!generic_extension && extension.actor_type != actor_type) {
        _return<PTR(void)>(ctx, NULLPTR);
        return;
    }

    actor_data_map_t::key actor_key{actor_handle};
    ExtensionData* data = actor_data.get(actor_key);

    // Check if actor handle is valid.
    if (data == nullptr) {
        _return<PTR(void)>(ctx, NULLPTR);
        return;
    }

    // Calculate the address for this specific extension's data.
    PTR(void) base_address = data->data_addr;
    u32 offset = extension.data_offset;
    // Specific actor data is after generic actor data, so increase the offset by the total generic actor data if this isn't generic data.
    if (!generic_extension) {
        offset += generic_data_size;
    }

    PTR(void) ret = base_address + offset;
    _return<PTR(void)>(ctx, ret);
}

extern "C" void recomp_get_actor_spawn_index(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ actor_data_mutex };

    u32 actor_handle = _arg<0, u32>(rdram, ctx);

    actor_data_map_t::key actor_key{actor_handle};
    ExtensionData* data = actor_data.get(actor_key);

    // Check if actor handle is valid.
    if (data == nullptr) {
        _return<u32>(ctx, 0xFFFFFFFFU);
        return;
    }

    _return<u32>(ctx, data->actor_spawn_index);
}

