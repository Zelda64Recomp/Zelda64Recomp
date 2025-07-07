#include <vector>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "slot_map.h"
#include "recomp_data.h"
#include "recomp_ui.h"
#include "librecomp/helpers.hpp"
#include "librecomp/overlays.hpp"
#include "librecomp/addresses.hpp"
#include "ultramodern/error_handling.hpp"

template <typename KeyType, typename ValueType>
class LockedMap {
private:
    std::mutex mutex{};
    std::unordered_map<KeyType, ValueType> map{};
public:
    bool get(const KeyType& key, ValueType& out) {
        std::lock_guard lock{mutex};
        auto find_it = map.find(key);
        if (find_it == map.end()) {
            return false;
        }
        out = find_it->second;
        return true;
    }

    bool insert(const KeyType& key, ValueType val) {
        std::lock_guard lock{mutex};
        auto ret = map.insert_or_assign(key, val);
        return ret.second;
    }

    bool erase(const KeyType& key) {
        std::lock_guard lock{mutex};
        size_t num_erased = map.erase(key);
        return num_erased != 0;
    }

    void clear() {
        std::lock_guard lock{mutex};
        map.clear();
    }
    
    bool erase_first(ValueType& out) {
        std::lock_guard lock{mutex};
        auto it = map.begin();

        if (it == map.end()) {
            return false;
        }

        out = it->second;
        map.erase(it);
        return true;
    }

    bool contains(const KeyType& key) {
        std::lock_guard lock{mutex};

        return map.contains(key);
    }

    size_t size() {
        std::lock_guard lock{mutex};

        return map.size();
    }
};

template <typename KeyType>
class LockedSet {
private:
    std::mutex mutex{};
    std::unordered_set<KeyType> set{};
public:
    bool contains(const KeyType& key) {
        std::lock_guard lock{mutex};
        return set.contains(key);
    }

    bool insert(const KeyType& key) {
        std::lock_guard lock{mutex};
        auto it = set.insert(key);
        return it.second;
    }

    bool erase(const KeyType& key) {
        std::lock_guard lock{mutex};
        size_t num_erased = set.erase(key);
        return num_erased != 0;
    }

    void clear() {
        std::lock_guard lock{mutex};
        set.clear();
    }

    size_t size() {
        std::lock_guard lock{mutex};
        return set.size();
    }
};

template <typename ValueType>
class LockedSlotmap {
private:
    std::mutex mutex{};
    dod::slot_map32<ValueType> map{};
    using key_t = typename dod::slot_map32<ValueType>::key;
public:
    bool get(uint32_t key, ValueType** out) {
        std::lock_guard lock{mutex};
        ValueType* ret = map.get(key_t{key});
        *out = ret;
        return ret != nullptr;
    }

    uint32_t create() {
        std::lock_guard lock{mutex};
        return map.emplace().raw;
    }

    bool erase(uint32_t key) {
        std::lock_guard lock{mutex};
        if (!map.has_key(key_t{ key })) {
            return false;
        }

        map.erase(key_t{ key });
        return true;
    }

    void clear() {
        std::lock_guard lock{mutex};
        map.clear();
    }
    
    bool erase_first(ValueType& out) {
        std::lock_guard lock{mutex};
        auto it = map.items().begin();

        if (it == map.items().end()) {
            return false;
        }

        out = it->second;
        map.erase(it->first);
        return true;
    }

    size_t size() {
        std::lock_guard lock{mutex};

        return map.size();
    }
};

using U32ValueMap = LockedMap<uint32_t, uint32_t>;
using U32MemoryMap = std::pair<LockedMap<uint32_t, PTR(void)>, u32>;
using U32HashSet = LockedSet<uint32_t>;
using U32Slotmap = LockedSlotmap<uint32_t>;
using MemorySlotmap = std::pair<LockedSlotmap<PTR(void)>, u32>;

LockedSlotmap<U32ValueMap> u32_value_hashmaps{};
LockedSlotmap<U32MemoryMap> u32_memory_hashmaps{};
LockedSlotmap<U32HashSet> u32_hashsets{};
LockedSlotmap<U32Slotmap> u32_slotmaps{};
LockedSlotmap<MemorySlotmap> memory_slotmaps{};

#define REGISTER_FUNC(name) recomp::overlays::register_base_export(#name, name)

static void show_fatal_error_message_box(const char* funcname, const char* errstr) {
    std::string message = std::string{"Fatal error in mod - "} + funcname + " : " + errstr;
    recompui::message_box(message.c_str());
}

#define HANDLE_INVALID_ERROR() \
    show_fatal_error_message_box(__FUNCTION__, "handle is invalid"); \
    assert(false); \
    ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);

#define SLOTMAP_KEY_INVALID_ERROR() \
    show_fatal_error_message_box(__FUNCTION__, "slotmap key is invalid"); \
    assert(false); \
    ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);

// u32 -> 32-bit value hashmap.

void recomputil_create_u32_value_hashmap(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    _return(ctx, u32_value_hashmaps.create());
}

void recomputil_destroy_u32_value_hashmap(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);

    if (!u32_value_hashmaps.erase(mapkey)) {
        HANDLE_INVALID_ERROR();
    }
}

void recomputil_u32_value_hashmap_contains(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);
    
    U32ValueMap* map;
    if (!u32_value_hashmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    _return(ctx, map->contains(key));
}

void recomputil_u32_value_hashmap_insert(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);
    uint32_t value = _arg<2, uint32_t>(rdram, ctx);

    U32ValueMap* map;
    if (!u32_value_hashmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    _return(ctx, map->insert(key, value));
}

void recomputil_u32_value_hashmap_get(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);
    PTR(uint32_t) val_out = _arg<2, PTR(uint32_t)>(rdram, ctx);
    
    U32ValueMap* map;
    if (!u32_value_hashmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    uint32_t ret;
    if (map->get(key, ret)) {
        MEM_W(0, val_out) = ret;
        _return(ctx, 1);
        return;
    }
    else {
        _return(ctx, 0);
        return;
    }
}

void recomputil_u32_value_hashmap_erase(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);

    U32ValueMap* map;
    if (!u32_value_hashmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    _return(ctx, map->erase(key));
}

void recomputil_u32_value_hashmap_size(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);

    U32ValueMap* map;
    if (!u32_value_hashmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    _return(ctx, static_cast<uint32_t>(map->size()));
}

// u32 -> memory hashmap.

void recomputil_create_u32_memory_hashmap(uint8_t* rdram, recomp_context* ctx) {
    uint32_t element_size = _arg<0, uint32_t>(rdram, ctx);
    
    // Create the map.
    uint32_t map_key = u32_memory_hashmaps.create();

    // Retrieve the map and set its element size to the provided value.
    U32MemoryMap* map;
    u32_memory_hashmaps.get(map_key, &map);
    map->second = element_size;

    // Return the created map's key.
    _return(ctx, map_key);
}

void recomputil_destroy_u32_memory_hashmap(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);

    // Retrieve the map.
    U32MemoryMap* map;
    if (!u32_memory_hashmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    // Free all of the entries in the map.
    PTR(void) cur_mem;
    while (map->first.erase_first(cur_mem)) {
        recomp::free(rdram, TO_PTR(void, cur_mem));
    }

    // Destroy the map itself.
    u32_memory_hashmaps.erase(mapkey);
}

void recomputil_u32_memory_hashmap_contains(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);
    
    U32MemoryMap* map;
    if (!u32_memory_hashmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    _return(ctx, map->first.contains(key));
}

void recomputil_u32_memory_hashmap_create(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);

    U32MemoryMap* map;
    if (!u32_memory_hashmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }
    
    // Check if the map contains the key already to prevent inserting it twice.
    PTR(void) dummy;
    if (map->first.get(key, dummy)) {
        _return(ctx, 0);
        return;
    }

    // Allocate the map's size and return the pointer.
    void* mem = recomp::alloc(rdram, map->second);
    gpr addr = reinterpret_cast<uint8_t*>(mem) - rdram + 0xFFFFFFFF80000000ULL;

    // Zero the memory.
    for (size_t i = 0; i < map->second; i++) {
        MEM_B(i, addr) = 0;
    }

    PTR(void) ret = static_cast<PTR(void)>(addr);
    map->first.insert(key, ret);
    _return(ctx, 1);
}

void recomputil_u32_memory_hashmap_get(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);
    
    U32MemoryMap* map;
    if (!u32_memory_hashmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    PTR(void) ret;
    if (map->first.get(key, ret)) {
        _return(ctx, ret);
        return;
    }
    else {
        _return(ctx, NULLPTR);
        return;
    }
}

void recomputil_u32_memory_hashmap_erase(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);

    U32MemoryMap* map;
    if (!u32_memory_hashmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }
    
    // Free the memory for this key if the key exists.
    PTR(void) addr;
    bool has_value = map->first.get(key, addr);
    if (has_value) {
        void* mem = TO_PTR(void, addr);
        recomp::free(rdram, mem);
    }

    _return(ctx, map->first.erase(key));
}

void recomputil_u32_memory_hashmap_size(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);

    U32MemoryMap* map;
    if (!u32_memory_hashmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    _return(ctx, static_cast<uint32_t>(map->first.size()));
}

// u32 hashset.

void recomputil_create_u32_hashset(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    _return(ctx, u32_hashsets.create());
}

void recomputil_destroy_u32_hashset(uint8_t* rdram, recomp_context* ctx) {
    uint32_t setkey = _arg<0, uint32_t>(rdram, ctx);

    if (!u32_hashsets.erase(setkey)) {
        HANDLE_INVALID_ERROR();
    }
}

void recomputil_u32_hashset_contains(uint8_t* rdram, recomp_context* ctx) {
    uint32_t setkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);
    
    U32HashSet* set;
    if (!u32_hashsets.get(setkey, &set)) {
        HANDLE_INVALID_ERROR();
    }

    _return(ctx, set->contains(key));
}

void recomputil_u32_hashset_insert(uint8_t* rdram, recomp_context* ctx) {
    uint32_t setkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);

    U32HashSet* set;
    if (!u32_hashsets.get(setkey, &set)) {
        HANDLE_INVALID_ERROR();
    }

    _return(ctx, set->insert(key));
}

void recomputil_u32_hashset_erase(uint8_t* rdram, recomp_context* ctx) {
    uint32_t setkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);

    U32HashSet* set;
    if (!u32_hashsets.get(setkey, &set)) {
        HANDLE_INVALID_ERROR();
    }
    
    _return(ctx, set->erase(key));
}

void recomputil_u32_hashset_size(uint8_t* rdram, recomp_context* ctx) {
    uint32_t setkey = _arg<0, uint32_t>(rdram, ctx);

    U32HashSet* set;
    if (!u32_hashsets.get(setkey, &set)) {
        HANDLE_INVALID_ERROR();
    }

    _return(ctx, static_cast<uint32_t>(set->size()));
}

// u32 value slotmap.

void recomputil_create_u32_slotmap(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    _return(ctx, u32_slotmaps.create());
}

void recomputil_destroy_u32_slotmap(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);

    if (!u32_slotmaps.erase(mapkey)) {
        HANDLE_INVALID_ERROR();
    }
}

void recomputil_u32_slotmap_contains(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);
    
    U32Slotmap* map;
    if (!u32_slotmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    uint32_t* dummy_ptr;
    _return(ctx, map->get(key, &dummy_ptr));
}

void recomputil_u32_slotmap_create(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);

    U32Slotmap* map;
    if (!u32_slotmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    _return(ctx, map->create());
}

void recomputil_u32_slotmap_get(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);
    PTR(uint32_t) val_out = _arg<2, PTR(uint32_t)>(rdram, ctx);
    
    U32Slotmap* map;
    if (!u32_slotmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    uint32_t* ret;
    if (!map->get(key, &ret)) {
        _return(ctx, 0);
    }
    MEM_W(0, val_out) = *ret;
    _return(ctx, 1);
}

void recomputil_u32_slotmap_set(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);
    uint32_t value = _arg<2, uint32_t>(rdram, ctx);

    U32Slotmap* map;
    if (!u32_slotmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    uint32_t* value_ptr;
    if (!map->get(key, &value_ptr)) {
        _return(ctx, 0);
    }

    *value_ptr = value;
    _return(ctx, 1);
}

void recomputil_u32_slotmap_erase(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);

    U32Slotmap* map;
    if (!u32_slotmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }
    
    if (!map->erase(key)) {
        _return(ctx, 0);
    }
    
    _return(ctx, 1);
}

void recomputil_u32_slotmap_size(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);

    U32Slotmap* map;
    if (!u32_slotmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    _return(ctx, static_cast<uint32_t>(map->size()));
}

// memory slotmap.

void recomputil_create_memory_slotmap(uint8_t* rdram, recomp_context* ctx) {
    uint32_t element_size = _arg<0, uint32_t>(rdram, ctx);
    
    // Create the map.
    uint32_t map_key = memory_slotmaps.create();

    // Retrieve the map and set its element size to the provided value.
    MemorySlotmap* map;
    memory_slotmaps.get(map_key, &map);
    map->second = element_size;

    // Return the created map's key.
    _return(ctx, map_key);
}

void recomputil_destroy_memory_slotmap(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);

    // Retrieve the map.
    MemorySlotmap* map;
    if (!memory_slotmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    // Free all of the entries in the map.
    PTR(void) cur_mem;
    while (map->first.erase_first(cur_mem)) {
        recomp::free(rdram, TO_PTR(void, cur_mem));
    }

    // Destroy the map itself.
    memory_slotmaps.erase(mapkey);
}

void recomputil_memory_slotmap_contains(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);
    
    MemorySlotmap* map;
    if (!memory_slotmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    PTR(void)* dummy_ptr;
    _return(ctx, map->first.get(key, &dummy_ptr));
}

void recomputil_memory_slotmap_create(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);

    MemorySlotmap* map;
    if (!memory_slotmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    // Create the slotmap element.
    u32 key = map->first.create();

    // Allocate the map's element size.
    void* mem = recomp::alloc(rdram, map->second);
    gpr addr = reinterpret_cast<uint8_t*>(mem) - rdram + 0xFFFFFFFF80000000ULL;

    // Zero the memory.
    for (size_t i = 0; i < map->second; i++) {
        MEM_B(i, addr) = 0;
    }

    // Store the allocated pointer.
    PTR(void)* value_ptr;
    map->first.get(key, &value_ptr);
    *value_ptr = static_cast<PTR(void)>(addr);

    // Return the key.
    _return(ctx, key);
}

void recomputil_memory_slotmap_get(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);
    PTR(uint32_t) val_out = _arg<2, PTR(uint32_t)>(rdram, ctx);
    
    MemorySlotmap* map;
    if (!memory_slotmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    PTR(void)* ret;
    if (!map->first.get(key, &ret)) {
        SLOTMAP_KEY_INVALID_ERROR();
    }
    MEM_W(0, val_out) = *ret;
}

void recomputil_memory_slotmap_erase(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);
    uint32_t key = _arg<1, uint32_t>(rdram, ctx);

    MemorySlotmap* map;
    if (!memory_slotmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }
    
    // Free the memory for this key if the key exists.
    PTR(void)* addr;
    bool has_value = map->first.get(key, &addr);
    if (has_value) {
        void* mem = TO_PTR(void, addr);
        recomp::free(rdram, mem);
    }

    _return(ctx, map->first.erase(key));
}

void recomputil_memory_slotmap_size(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mapkey = _arg<0, uint32_t>(rdram, ctx);

    MemorySlotmap* map;
    if (!memory_slotmaps.get(mapkey, &map)) {
        HANDLE_INVALID_ERROR();
    }

    _return(ctx, static_cast<uint32_t>(map->first.size()));
}

// Exports.

void recomputil::register_data_api_exports() {
    REGISTER_FUNC(recomputil_create_u32_value_hashmap);
    REGISTER_FUNC(recomputil_destroy_u32_value_hashmap);
    REGISTER_FUNC(recomputil_u32_value_hashmap_contains);
    REGISTER_FUNC(recomputil_u32_value_hashmap_insert);
    REGISTER_FUNC(recomputil_u32_value_hashmap_get);
    REGISTER_FUNC(recomputil_u32_value_hashmap_erase);
    REGISTER_FUNC(recomputil_u32_value_hashmap_size);
    
    REGISTER_FUNC(recomputil_create_u32_memory_hashmap);
    REGISTER_FUNC(recomputil_destroy_u32_memory_hashmap);
    REGISTER_FUNC(recomputil_u32_memory_hashmap_contains);
    REGISTER_FUNC(recomputil_u32_memory_hashmap_create);
    REGISTER_FUNC(recomputil_u32_memory_hashmap_get);
    REGISTER_FUNC(recomputil_u32_memory_hashmap_erase);
    REGISTER_FUNC(recomputil_u32_memory_hashmap_size);
    
    REGISTER_FUNC(recomputil_create_u32_hashset);
    REGISTER_FUNC(recomputil_destroy_u32_hashset);
    REGISTER_FUNC(recomputil_u32_hashset_contains);
    REGISTER_FUNC(recomputil_u32_hashset_insert);
    REGISTER_FUNC(recomputil_u32_hashset_erase);
    REGISTER_FUNC(recomputil_u32_hashset_size);

    REGISTER_FUNC(recomputil_create_u32_slotmap);
    REGISTER_FUNC(recomputil_destroy_u32_slotmap);
    REGISTER_FUNC(recomputil_u32_slotmap_contains);
    REGISTER_FUNC(recomputil_u32_slotmap_create);
    REGISTER_FUNC(recomputil_u32_slotmap_get);
    REGISTER_FUNC(recomputil_u32_slotmap_set);
    REGISTER_FUNC(recomputil_u32_slotmap_erase);
    REGISTER_FUNC(recomputil_u32_slotmap_size);

    REGISTER_FUNC(recomputil_create_memory_slotmap);
    REGISTER_FUNC(recomputil_destroy_memory_slotmap);
    REGISTER_FUNC(recomputil_memory_slotmap_contains);
    REGISTER_FUNC(recomputil_memory_slotmap_create);
    REGISTER_FUNC(recomputil_memory_slotmap_get);
    REGISTER_FUNC(recomputil_memory_slotmap_erase);
    REGISTER_FUNC(recomputil_memory_slotmap_size);
}
