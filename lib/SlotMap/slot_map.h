// This file is originally from the repo: https://github.com/SergeyMakeev/SlotMap.
// The original license and code are as follows:

// MIT License
//
// Copyright (c) 2022 Sergey Makeev
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cstring>
#include <deque>
#include <functional>
#include <optional>
#include <stdint.h>
#include <vector>
#include <algorithm>


#include <inttypes.h>
#define PRIslotkey PRIu64

// TODO (detailed desc): you could override memory allocator by defining SLOT_MAP_ALLOC/SLOT_MAP_FREE macroses
#if !defined(SLOT_MAP_ALLOC) || !defined(SLOT_MAP_FREE)

#if defined(_WIN32)
// Windows
#include <xmmintrin.h>
#define SLOT_MAP_ALLOC(sizeInBytes, alignment) _mm_malloc(sizeInBytes, alignment)
#define SLOT_MAP_FREE(ptr) _mm_free(ptr)
#else
// Posix
#include <stdlib.h>
#define SLOT_MAP_ALLOC(sizeInBytes, alignment) aligned_alloc(alignment, sizeInBytes)
#define SLOT_MAP_FREE(ptr) free(ptr)
#endif

#endif

// extern void _onAssertionFailed(const char* expression, const char* srcFile, unsigned int srcLine);
//#define SLOT_MAP_ASSERT(expression) (void)((!!(expression)) || (_onAssertionFailed(#expression, __FILE__, (unsigned int)(__LINE__)), 0))

// TODO (detailed desc): you could override asserts by defining SLOT_MAP_ASSERT macro
#if !defined(SLOT_MAP_ASSERT)
#include <assert.h>
#define SLOT_MAP_ASSERT(expression) assert(expression)
#endif

namespace stl
{
// STL compatible allocator
// Note: some platforms (macOS) does not support alignments smaller than `alignof(void*)`
template <class T, size_t Alignment = std::max(alignof(T), alignof(void*))> struct Allocator
{
  public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    template <class U> struct rebind
    {
        using other = Allocator<U, Alignment>;
    };

    Allocator() noexcept {}
    Allocator(const Allocator& /*other*/) noexcept {}

    template <typename U> Allocator(const Allocator<U, Alignment>& /* other */) noexcept {}

    ~Allocator() {}

    pointer address(reference x) const noexcept { return &x; }
    const_pointer address(const_reference x) const noexcept { return &x; }

    pointer allocate(size_type n, [[maybe_unused]] const void* hint = 0)
    {
        size_t alignment = Alignment;
        n = std::max(n, alignment);
        pointer p = reinterpret_cast<pointer>(SLOT_MAP_ALLOC((sizeof(value_type) * n), alignment));
        SLOT_MAP_ASSERT(p);
        return p;
    }

    void deallocate(pointer p, size_type /* n */) { SLOT_MAP_FREE(p); }

    size_type max_size() const noexcept { return std::numeric_limits<size_type>::max() / sizeof(value_type); }

    template <class U, class... Args> void construct(U* p, Args&&... args)
    {
        new (reinterpret_cast<void*>(p)) U(std::forward<Args>(args)...);
    }
    template <class U> void destroy(U* p) { p->~U(); }
};

template <class T1, class T2, size_t Alignment>
bool operator==(const Allocator<T1, Alignment>& /*lhs*/, const Allocator<T2, Alignment>& /*rhs*/) noexcept
{
    return true;
}

template <class T1, class T2, size_t Alignment>
bool operator!=(const Allocator<T1, Alignment>& /*lhs*/, const Allocator<T2, Alignment>& /*rhs*/) noexcept
{
    return false;
}
} // namespace stl

namespace dod
{

/*
Even though slot map keys are technically typeless (uint64_t), we artificially add a new type to get extra compiler checks.

i.e., the following code should not compile
```
slot_map<std::string> strings;
slot_map<int> numbers;
slot_map<int>::key numKey =  numbers.emplace(3);
const std::string* value = strings.get(numKey);
```
*/

/*

64-bit key

| Component      |  Number of bits        |
| ---------------|------------------------|
| tag            |  12                    |
| version        |  20 (0..1,048,575      |
| index          |  32 (0..4,294,967,295) |

*/
template <typename T> struct slot_map_key64
{
    using id_type = uint64_t;
    using version_t = uint32_t;
    using index_t = uint32_t;
    using tag_t = uint16_t;

    static inline constexpr version_t kInvalidVersion = 0x0u;
    static inline constexpr version_t kMinVersion = 0x1u;
    static inline constexpr version_t kMaxVersion = 0x0fffffu;
    static inline constexpr index_t kMaxIndex = 0xffffffffu;
    static inline constexpr tag_t kMaxTag = 0x0fffu;
    
    static inline constexpr id_type kIndexMask = 0x00000000ffffffffull;
    
    static inline constexpr id_type kVersionMask = 0x0fffff00000000ull;
    static inline constexpr id_type kVersionShift = 32ull;

    static inline constexpr id_type kHandleTagMask = 0xfff0000000000000ull;
    static inline constexpr id_type kHandleTagShift = 52ull;

    static inline constexpr slot_map_key64 make(version_t version, index_t index) noexcept
    {
        SLOT_MAP_ASSERT(version != kInvalidVersion);
        SLOT_MAP_ASSERT(index <= kMaxIndex);
        id_type v = (static_cast<id_type>(version) << kVersionShift) & kVersionMask;
        id_type i = (static_cast<id_type>(index)) & kIndexMask;
        return slot_map_key64{v | i};
    }

    inline size_t hash() const noexcept { return std::hash<id_type>{}(raw); }

    static inline slot_map_key64 clearTagAndUpdateVersion(slot_map_key64 key, version_t version) noexcept
    {
        SLOT_MAP_ASSERT(version != kInvalidVersion);
        id_type ver = (static_cast<id_type>(version) << kVersionShift) & kVersionMask;
        return slot_map_key64{((key.raw & (~kVersionMask)) | ver) & (~kHandleTagMask)};
    }
    static inline index_t toIndex(slot_map_key64 key) noexcept { return static_cast<index_t>(key.raw & kIndexMask); }
    static inline version_t toVersion(slot_map_key64 key) noexcept
    {
        return static_cast<version_t>((key.raw & kVersionMask) >> kVersionShift);
    }
    static inline version_t increaseVersion(version_t version) noexcept { return (version + 1); }

    inline tag_t get_tag() const noexcept { return static_cast<tag_t>((raw & kHandleTagMask) >> kHandleTagShift); }
    inline void set_tag(tag_t tag) noexcept
    {
        SLOT_MAP_ASSERT(tag <= kMaxTag);
        id_type ud = (static_cast<id_type>(tag) << kHandleTagShift) & kHandleTagMask;
        raw = ((raw & (~kHandleTagMask)) | ud);
    }

    slot_map_key64() noexcept = default;
    explicit slot_map_key64(id_type raw) noexcept
        : raw(raw)
    {
    }
    slot_map_key64(const slot_map_key64&) noexcept = default;
    slot_map_key64& operator=(const slot_map_key64&) noexcept = default;
    slot_map_key64(slot_map_key64&&) noexcept = default;
    slot_map_key64& operator=(slot_map_key64&&) noexcept = default;

    bool operator==(const slot_map_key64& other) const noexcept { return raw == other.raw; }
    bool operator<(const slot_map_key64& other) const noexcept { return raw < other.raw; }

    // implicit conversion to id_type (useful for printing and debug)
    operator id_type() const noexcept { return raw; }

    static inline slot_map_key64 invalid() noexcept { return slot_map_key64{0}; }

    id_type raw;
};

/*

32-bit key

| Component      |  Number of bits     |
| ---------------|---------------------|
| tag            |  2                  |
| version        |  10 (0..1023)       |
| index          |  20 (0..1,048,576)  |

*/
template <typename T> struct slot_map_key32
{
    using id_type = uint32_t;
    using version_t = uint16_t;
    using index_t = uint32_t;
    using tag_t = uint8_t;

    static inline constexpr version_t kInvalidVersion = 0x0u;
    static inline constexpr version_t kMinVersion = 0x1u;
    static inline constexpr version_t kMaxVersion = 0x0400u;
    static inline constexpr index_t kMaxIndex = 0x000fffffu;
    static inline constexpr tag_t kMaxTag = 0x03u;

    static inline constexpr id_type kIndexMask = 0x0fffffull;

    static inline constexpr id_type kVersionMask = 0x3ff00000ull;
    static inline constexpr id_type kVersionShift = 20ull;

    static inline constexpr id_type kHandleTagMask = 0xc0000000ull;
    static inline constexpr id_type kHandleTagShift = 30ull;

    static inline constexpr slot_map_key32 make(version_t version, index_t index) noexcept
    {
        SLOT_MAP_ASSERT(version != kInvalidVersion);
        SLOT_MAP_ASSERT(index <= kMaxIndex);
        id_type v = (static_cast<id_type>(version) << kVersionShift) & kVersionMask;
        id_type i = (static_cast<id_type>(index)) & kIndexMask;
        return slot_map_key32{v | i};
    }

    inline size_t hash() const noexcept { return std::hash<id_type>{}(raw); }

    static inline slot_map_key32 clearTagAndUpdateVersion(slot_map_key32 key, version_t version) noexcept
    {
        SLOT_MAP_ASSERT(version != kInvalidVersion);
        id_type ver = (static_cast<id_type>(version) << kVersionShift) & kVersionMask;
        return slot_map_key32{((key.raw & (~kVersionMask)) | ver) & (~kHandleTagMask)};
    }
    static inline index_t toIndex(slot_map_key32 key) noexcept { return static_cast<index_t>(key.raw & kIndexMask); }
    static inline version_t toVersion(slot_map_key32 key) noexcept
    {
        return static_cast<version_t>((key.raw & kVersionMask) >> kVersionShift);
    }
    static inline version_t increaseVersion(version_t version) noexcept { return (version + 1); }

    inline tag_t get_tag() const noexcept { return static_cast<tag_t>((raw & kHandleTagMask) >> kHandleTagShift); }
    inline void set_tag(tag_t tag) noexcept
    {
        SLOT_MAP_ASSERT(tag <= kMaxTag);
        id_type ud = (static_cast<id_type>(tag) << kHandleTagShift) & kHandleTagMask;
        raw = ((raw & (~kHandleTagMask)) | ud);
    }

    slot_map_key32() noexcept = default;
    explicit slot_map_key32(id_type raw) noexcept
        : raw(raw)
    {
    }
    slot_map_key32(const slot_map_key32&) noexcept = default;
    slot_map_key32& operator=(const slot_map_key32&) noexcept = default;
    slot_map_key32(slot_map_key32&&) noexcept = default;
    slot_map_key32& operator=(slot_map_key32&&) noexcept = default;

    bool operator==(const slot_map_key32& other) const noexcept { return raw == other.raw; }
    bool operator<(const slot_map_key32& other) const noexcept { return raw < other.raw; }

    // implicit conversion to id_type (useful for printing and debug)
    operator id_type() const noexcept { return raw; }

    static inline slot_map_key32 invalid() noexcept { return slot_map_key32{0}; }

    id_type raw;
};

/*
  A slot map is a high-performance associative container with persistent unique keys to access stored values. Upon insertion, a key is
  returned that can be used to later access or remove the values. Insertion, removal, and access are all guaranteed to take O(1) time (best,
  worst, and average case) Great for storing collections of objects that need stable, safe references but have no clear ownership.

  The difference between a std::unordered_map and a slot map is that the slot map generates and returns the key when inserting a value. A
  key is always unique and will only refer to the value that was inserted.

  Usage example:
  ```
  slot_map<std::string> strings;
  auto red = strings.emplace("Red");
  auto green = strings.emplace("Green");
  auto blue = strings.emplace("Blue");

  const std::string* val1 = strings.get(red);
  if (val1)
  {
    printf("red = '%s'\n", val1->c_str());
  }

  strings.erase(green);
  printf("%d\n", strings.has(green));
  printf("%d\n", strings.has(blue));
  ```

  Output:
  ```
  red = 'Red'
  0
  1
  ```

*/

/*
  Usefull reading:

  Niklas Gray
  Building a Data-Oriented Entity System (part 1), 2014
  http://bitsquid.blogspot.com/2014/08/building-data-oriented-entity-system.html

  Noel Llopis
  Managing Data Relationships, 2010
  https://gamesfromwithin.com/managing-data-relationships

  Stefan Reinalter
  Adventures in data-oriented design - Part 3c: External References, 2013
  https://blog.molecular-matters.com/2013/07/24/adventures-in-data-oriented-design-part-3c-external-references/

  Niklas Gray
  Managing Decoupling Part 4 - The ID Lookup Table, 2011
  https://bitsquid.blogspot.com/2011/09/managing-decoupling-part-4-id-lookup.html

  Sander Mertens
  Making the most of ECS identifiers, 2020
  https://ajmmertens.medium.com/doing-a-lot-with-a-little-ecs-identifiers-25a72bd2647

  Michele Caini
  ECS back and forth. Part 9 - Sparse sets and EnTT, 2020
  https://skypjack.github.io/2020-08-02-ecs-baf-part-9/

  Andre Weissflog
  Handles are the better pointers, 2018
  https://floooh.github.io/2018/06/17/handles-vs-pointers.html

  Allan Deutsch
  C++Now 2017: "The Slot Map Data Structure", 2017
  https://www.youtube.com/watch?v=SHaAR7XPtNU

  Jeff Gates
  Init, Update, Draw - Data Arrays, 2012
  https://greysphere.tumblr.com/post/31601463396/data-arrays
*/
template <typename T, typename TKeyType = slot_map_key64<T>, size_t PAGESIZE = 4096, size_t MINFREEINDICES = 64> class slot_map
{
  public:
    using key = TKeyType;
    using version_t = typename TKeyType::version_t;
    using index_t = typename TKeyType::index_t;
    using tag_t = typename TKeyType::tag_t;
    using size_type = uint32_t;

    /*
        kPageSize = 4096 (default)

        Page size in elements. When all page items(slots) are disabled we automatically release that page to save memory.
        The page-based allocator also helps with pointers' stability and prevents memory spikes on reallocation.

        4096 seems like a reasonable number of items per page.
        I.e., for simple uint32_t type, it will take 16Kb of RAM per page = good compromise between CPU cache utilization,
        allocation granularity, and memory overhead from deactivated slots.
    */
    static inline constexpr size_type kPageSize = static_cast<size_type>(PAGESIZE);

    /*
        kMinFreeIndices = 64 (default)

        When a slot is reused, its version is automatically incremented (to make existing keys invalid).
        But since we only use 16-bits for version numbers, there is a possibility that the version counter will wrap around,
        and a new item will get the same key as a removed item.

        Once the version counter overflows, we disable that slot so that no new keys are returned for this slot
        (this gives us a guarantee that there are no key collisions)

        To prevent version overflow from happening too often, we need to ensure that we don't reuse the same slot too often.
        So we do not reuse indices as long as their number is below a certain threshold.
    */
    static inline constexpr size_type kMinFreeIndices = static_cast<size_type>(MINFREEINDICES);

  private:
    using ValueStorage = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

    struct Meta
    {
        version_t version; // note: 0 is reserved for kInvalidVersion
        uint8_t tombstone; // note: we only need 1 bit for tombstone marker
        uint8_t inactive;  // note: we only need 1 bit for inactive marker
    };

    struct Page
    {
        void* rawMemory;
        ValueStorage* values;
        Meta* meta;
        size_type numInactiveSlots;
        size_type numUsedElements;

        Page() noexcept
            : rawMemory(nullptr)
            , values(nullptr)
            , meta(nullptr)
            , numInactiveSlots(0)
            , numUsedElements(0)
        {
        }

        Page(const Page&) = delete;
        Page& operator=(const Page&) = delete;
        Page& operator=(Page&&) = delete;
        Page(Page&& other) noexcept
            : rawMemory(nullptr)
            , values(nullptr)
            , meta(nullptr)
            , numInactiveSlots(0)
            , numUsedElements(0)
        {
            std::swap(rawMemory, other.rawMemory);
            std::swap(meta, other.meta);
            std::swap(values, other.values);
            std::swap(numInactiveSlots, other.numInactiveSlots);
            std::swap(numUsedElements, other.numUsedElements);
        }
        ~Page() { deallocate(); }

        void deallocate()
        {
            if (!rawMemory)
            {
                SLOT_MAP_ASSERT(!values);
                SLOT_MAP_ASSERT(!meta);
                return;
            }
            SLOT_MAP_ASSERT(values);
            SLOT_MAP_ASSERT(meta);

            SLOT_MAP_FREE(rawMemory);
            rawMemory = nullptr;
            values = nullptr;
            meta = nullptr;
        }

        void allocate()
        {
            SLOT_MAP_ASSERT(!rawMemory);
            SLOT_MAP_ASSERT(!values);
            SLOT_MAP_ASSERT(!meta);

            size_type metaSize = static_cast<size_type>(sizeof(Meta)) * kPageSize;
            size_type dataSize = static_cast<size_type>(sizeof(ValueStorage)) * kPageSize;
            size_type alignedDataSize = align(dataSize, static_cast<size_type>(alignof(Meta)));
            size_type alignment = std::max(static_cast<size_type>(alignof(Meta)), static_cast<size_type>(alignof(ValueStorage)));
            // some platforms (macOS) does not support alignments smaller than `alignof(void*)`
            // and 16 bytes seem like a nice compromise
            alignment = std::max(alignment, 16u);
            size_type numBytes = alignedDataSize + metaSize;

            /*
              C++11 std::aligned_alloc

              Passing a size which is not an integral multiple of alignment or an alignment which is not valid or not supported by the
              implementation causes the function to fail and return a null pointer (C11, as published, specified undefined behavior in
              this case, this was corrected by DR 460)
            */
            numBytes = align(numBytes, alignment);
            SLOT_MAP_ASSERT((numBytes % alignment) == 0);
            rawMemory = SLOT_MAP_ALLOC(static_cast<size_t>(numBytes), static_cast<size_t>(alignment));
            SLOT_MAP_ASSERT(rawMemory);

            numInactiveSlots = 0;
            numUsedElements = 0;
            values = reinterpret_cast<ValueStorage*>(rawMemory);
            meta = reinterpret_cast<Meta*>(reinterpret_cast<char*>(rawMemory) + alignedDataSize);

            // TODO: remove rawMemory member
            SLOT_MAP_ASSERT(values == rawMemory);
            SLOT_MAP_ASSERT(values);
            SLOT_MAP_ASSERT(meta);
            SLOT_MAP_ASSERT(isPointerAligned(values, alignof(ValueStorage)));
            SLOT_MAP_ASSERT(isPointerAligned(meta, alignof(Meta)));
        }
    };

    static inline size_type align(size_type cursor, size_type alignment) noexcept { return (cursor + (alignment - 1)) & ~(alignment - 1); }
    static inline bool isPointerAligned(void* cursor, size_t alignment) noexcept { return (uintptr_t(cursor) & (alignment - 1)) == 0; }

    template <typename TYPE, class... Args> static void construct(void* mem, Args&&... args)
    {
        new (mem) TYPE(std::forward<Args>(args)...);
    }
    template <typename TYPE> void destruct(TYPE* p) { p->~T(); }

    const T* getImpl(key k) const noexcept
    {
        index_t index = key::toIndex(k);
        if (index > getMaxValidIndex())
        {
            // index out of bounds
            return nullptr;
        }

        PageAddr addr = getAddrFromIndex(index);
        if (!isActivePage(addr))
        {
            return nullptr;
        }

        const Meta& m = getMetaByAddr(addr);

        version_t version = key::toVersion(k);
        if (m.version != version)
        {
            // version mismatch, slot has been reused
            return nullptr;
        }
        SLOT_MAP_ASSERT(m.version != key::kInvalidVersion);
        SLOT_MAP_ASSERT(m.tombstone == 0);
        SLOT_MAP_ASSERT(m.inactive == 0);

        const ValueStorage& v = getValueByAddr(addr);
        const T* value = reinterpret_cast<const T*>(&v);
        return value;
    }

    index_t appendElement()
    {
        if (pages.empty() || pages.back().numUsedElements == kPageSize)
        {
            Page& p = pages.emplace_back();
            p.allocate();
        }

        Page& lastPage = pages.back();

        size_type elementIndex = lastPage.numUsedElements;
        SLOT_MAP_ASSERT(elementIndex <= kPageSize);
        lastPage.numUsedElements++;

        Meta& m = lastPage.meta[elementIndex];
        m.version = key::kMinVersion;
        m.tombstone = 0;
        m.inactive = 0;

        SLOT_MAP_ASSERT(pages.size() >= 1);
        index_t index = static_cast<index_t>(getIndexFromAddr(PageAddr{static_cast<size_type>(pages.size()) - 1, elementIndex}));
        return index;
    }

    struct PageAddr
    {
        size_type page;
        size_type index;
    };

    template <typename INTEGRAL_TYPE> inline static constexpr bool isPow2(INTEGRAL_TYPE x) noexcept
    {
        static_assert(std::is_integral<INTEGRAL_TYPE>::value, "isPow2 must be called on an integer type.");
        return (x & (x - 1)) == 0 && (x != 0);
    }

    static inline PageAddr getAddrFromIndex(size_type index) noexcept
    {
        static_assert(isPow2(kPageSize), "kPageSize is expected to be a power of two.");
        PageAddr res;
        res.page = index / kPageSize;
        res.index = index % kPageSize;
        return res;
    }

    // to global index
    static inline index_t getIndexFromAddr(PageAddr addr) noexcept
    {
        size_type index = (addr.page * kPageSize + addr.index);
        return index_t(index);
    }

    const Meta& getMetaByAddrImpl(PageAddr addr) const noexcept
    {
        SLOT_MAP_ASSERT(addr.page < pages.size());
        const Page& page = pages[addr.page];
        SLOT_MAP_ASSERT(page.meta);
        SLOT_MAP_ASSERT(addr.index < kPageSize);
        return page.meta[addr.index];
    }

    const ValueStorage& getValueByAddrImpl(PageAddr addr) const noexcept
    {
        SLOT_MAP_ASSERT(addr.page < pages.size());
        const Page& page = pages[addr.page];
        SLOT_MAP_ASSERT(page.values);
        SLOT_MAP_ASSERT(addr.index < kPageSize);
        return page.values[addr.index];
    }

    const Meta& getMetaByAddr(PageAddr addr) const noexcept { return getMetaByAddrImpl(addr); }
    const ValueStorage& getValueByAddr(PageAddr addr) const noexcept { return getValueByAddrImpl(addr); }

    Meta& getMetaByAddr(PageAddr addr) noexcept
    {
        const Meta& constRes = getMetaByAddrImpl(addr);
        return const_cast<Meta&>(constRes);
    }

    ValueStorage& getValueByAddr(PageAddr addr) noexcept
    {
        const ValueStorage& constRes = getValueByAddrImpl(addr);
        return const_cast<ValueStorage&>(constRes);
    }

    bool isTombstone(size_type index) const noexcept
    {
        PageAddr addr = getAddrFromIndex(index);
        SLOT_MAP_ASSERT(addr.page < pages.size());
        const Page& page = pages[addr.page];
        if (!page.meta)
        {
            return true;
        }
        SLOT_MAP_ASSERT(addr.index < kPageSize);
        return (page.meta[addr.index].tombstone != 0);
    }

    bool isActivePage(PageAddr addr) const noexcept
    {
        if (addr.page >= pages.size())
        {
            return false;
        }
        const Page& page = pages[addr.page];
        return (page.meta != nullptr);
    }

    size_type getMaxValidIndex() const noexcept { return maxValidIndex; }

    void copyFrom(const slot_map& other)
    {
        reset();

        SLOT_MAP_ASSERT(numItems == 0);
        SLOT_MAP_ASSERT(maxValidIndex == 0);
        SLOT_MAP_ASSERT(pages.empty());
        SLOT_MAP_ASSERT(freeIndices.empty());

        static_assert(std::is_standard_layout<Meta>::value && std::is_trivially_copyable<Meta>::value,
                      "Meta is expected to be memcopyable (POD type)");

        numItems = other.numItems;
        maxValidIndex = other.maxValidIndex;

        for (size_t pageIndex = 0; pageIndex < other.pages.size(); pageIndex++)
        {
            const Page& otherPage = other.pages[pageIndex];
            if (otherPage.meta)
            {
                SLOT_MAP_ASSERT(otherPage.values);

                // active page
                Page& p = pages.emplace_back();
                p.allocate();
                p.numInactiveSlots = otherPage.numInactiveSlots;
                p.numUsedElements = otherPage.numUsedElements;

                // copy meta
                size_type metaSize = static_cast<size_type>(sizeof(Meta)) * kPageSize;
                std::memcpy(p.meta, otherPage.meta, metaSize);

                // copy data
                if constexpr (std::is_standard_layout<T>::value && std::is_trivially_copyable<T>::value)
                {
                    size_type dataSize = static_cast<size_type>(sizeof(ValueStorage)) * kPageSize;
                    std::memcpy(p.values, otherPage.values, dataSize);
                }
                else
                {
                    for (size_type elementIndex = 0; elementIndex < p.numUsedElements; elementIndex++)
                    {
                        PageAddr addr;
                        addr.page = static_cast<size_type>(pageIndex);
                        addr.index = elementIndex;

                        const ValueStorage& otherVal = other.getValueByAddr(addr);
                        const T* otherV = reinterpret_cast<const T*>(&otherVal);
                        ValueStorage& val = getValueByAddr(addr);

                        // copy constructor
                        construct<T>(&val, *otherV);
                    }
                }
            }
            else
            {
                SLOT_MAP_ASSERT(otherPage.meta == nullptr);
                SLOT_MAP_ASSERT(otherPage.values == nullptr);

                // inactive page
                Page& p = pages.emplace_back();
                p.numInactiveSlots = otherPage.numInactiveSlots;
                p.numUsedElements = otherPage.numUsedElements;
                SLOT_MAP_ASSERT(p.values == nullptr);
                SLOT_MAP_ASSERT(p.meta == nullptr);
            }
        }
    }

    void callDtors()
    {
        size_type numItemsDestroyed = 0;
        for (size_t pageIndex = 0; pageIndex < pages.size(); pageIndex++)
        {
            Page& page = pages[pageIndex];
            if (page.meta == nullptr)
            {
                continue;
            }
            for (size_type elementIndex = 0; elementIndex < page.numUsedElements; elementIndex++)
            {
                PageAddr addr;
                addr.page = static_cast<size_type>(pageIndex);
                addr.index = static_cast<size_type>(elementIndex);

                Meta& m = getMetaByAddr(addr);
                if (m.tombstone != 0)
                {
                    continue;
                }
                if constexpr (!std::is_trivially_destructible<T>::value)
                {
                    ValueStorage& v = getValueByAddr(addr);
                    destruct(reinterpret_cast<const T*>(&v));
                }
                numItemsDestroyed++;
            }
        }
        SLOT_MAP_ASSERT(numItemsDestroyed == numItems);
    }

    enum class EraseResult
    {
        NotFound,
        ErasedAndIndexRecycled,
        ErasedAndPageDeactivated,
    };

    template <bool VERSION_CHECK> EraseResult eraseImpl(key k)
    {
        index_t index = key::toIndex(k);
        if (index > getMaxValidIndex())
        {
            return EraseResult::NotFound;
        }

        PageAddr addr = getAddrFromIndex(index);
        if (!isActivePage(addr))
        {
            return EraseResult::NotFound;
        }

        Meta& m = getMetaByAddr(addr);
        if (m.tombstone != 0)
        {
            return EraseResult::NotFound;
        }

        version_t slotVersion = m.version;

        if constexpr (VERSION_CHECK)
        {
            version_t version = key::toVersion(k);
            if (slotVersion != version || slotVersion == key::kInvalidVersion || version == key::kInvalidVersion)
            {
                return EraseResult::NotFound;
            }
        }

        bool deactivateSlot = (slotVersion == key::kMaxVersion);
        if (deactivateSlot)
        {
            // version overflow = deactivate slot
            m.inactive = 1;
        }
        else
        {
            // increase version
            slotVersion = key::increaseVersion(slotVersion);
            SLOT_MAP_ASSERT(slotVersion != key::kInvalidVersion);
            SLOT_MAP_ASSERT(slotVersion > m.version);
        }

        m.version = slotVersion;
        m.tombstone = 1;

        if constexpr (!std::is_trivially_destructible<T>::value)
        {
            ValueStorage& v = getValueByAddr(addr);
            const T* pv = reinterpret_cast<const T*>(&v);
            destruct(pv);
        }
        numItems--;

        if (deactivateSlot)
        {
            Page& page = pages[addr.page];
            page.numInactiveSlots++;
            if (page.numInactiveSlots == kPageSize)
            {
                page.deallocate();
                return EraseResult::ErasedAndPageDeactivated;
            }
        }
        else
        {
            // recycle index id (note: tag is not saved!)
            freeIndices.emplace_back(key::clearTagAndUpdateVersion(k, slotVersion));
        }
        return EraseResult::ErasedAndIndexRecycled;
    }

  public:
    slot_map()
        : numItems(0)
        , maxValidIndex(0)
    {
    }
    ~slot_map() { callDtors(); }

    /*
      Returns true if the slot map contains a specific key
    */
    bool has_key(key k) const noexcept
    {
        index_t index = key::toIndex(k);
        if (index > getMaxValidIndex())
        {
            return false;
        }
        version_t version = key::toVersion(k);
        PageAddr addr = getAddrFromIndex(index);
        if (!isActivePage(addr))
        {
            return false;
        }
        const Meta& m = getMetaByAddr(addr);
        return (m.version == version);
    }

    /*
      Clears the slot map and releases any allocated memory.
      Note: By calling this function, you must guarantee that no handles are in use!
      Otherwise calling this function might be dangerous and lead to key "collisions".
      You might consider using "clear()" instead.
    */
    void reset()
    {
        callDtors();

        numItems = 0;
        maxValidIndex = 0;

        // Release used memory (using swap trick)
        if (!pages.empty())
        {
            std::vector<Page, stl::Allocator<Page>> tmpPages;
            pages.swap(tmpPages);
        }

        if (!freeIndices.empty())
        {
            std::deque<key, stl::Allocator<key>> tmpFreeIndices;
            freeIndices.swap(tmpFreeIndices);
        }
    }

    /*
      Clears the slot map but keeps the allocated memory for reuse.
      Automatically increases version for all the removed elements (the same as calling "erase()" for all existing elements)
    */
    void clear()
    {
        for (size_t pageIndex = 0; pageIndex < pages.size(); pageIndex++)
        {
            Page& page = pages[pageIndex];
            if (page.meta == nullptr)
            {
                continue;
            }
            for (size_type elementIndex = 0; elementIndex < page.numUsedElements; elementIndex++)
            {
                index_t index = getIndexFromAddr(PageAddr{static_cast<size_type>(pageIndex), elementIndex});
                // note: version doesn't matter here
                EraseResult res = eraseImpl<false>(key::make(key::kMinVersion, index));
                if (res == EraseResult::ErasedAndPageDeactivated)
                {
                    // noting left on this page - go to the next page
                    break;
                }
            }
        }
        SLOT_MAP_ASSERT(numItems == 0);
    }

    /*
      If key exists returns a const pointer to the value corresponding to the given key or returns null elsewere.
    */
    const T* get(key k) const noexcept { return getImpl(k); }

    /*
      If key exists returns a pointer to the value corresponding to the given key or returns null elsewere.
    */
    T* get(key k) noexcept
    {
        const T* constRes = getImpl(k);
        return const_cast<T*>(constRes);
    }

    /*
      Constructs element in-place and returns a unique key that can be used to access this value.
    */
    template <class... Args> key emplace(Args&&... args)
    {
        // Use recycled IDs only if we accumulated enough of them
        if (static_cast<size_type>(freeIndices.size()) > kMinFreeIndices)
        {
            key k = freeIndices.front();
            freeIndices.pop_front();

            index_t index = key::toIndex(k);
            SLOT_MAP_ASSERT(index <= getMaxValidIndex());

            PageAddr addr = getAddrFromIndex(index);
            Meta& m = getMetaByAddr(addr);
            SLOT_MAP_ASSERT(m.inactive == 0);
            SLOT_MAP_ASSERT(m.tombstone != 0);
            SLOT_MAP_ASSERT(k.get_tag() == 0);

            m.tombstone = 0;

            ValueStorage& v = getValueByAddr(addr);
            construct<T>(&v, std::forward<Args>(args)...);
            numItems++;
            return k;
        }

        // allocate new item
        index_t index = appendElement();
        maxValidIndex = std::max(maxValidIndex, index);

        PageAddr addr = getAddrFromIndex(index);
        const Meta& m = getMetaByAddr(addr);
        SLOT_MAP_ASSERT(m.tombstone == 0);

        ValueStorage& v = getValueByAddr(addr);
        construct<T>(&v, std::forward<Args>(args)...);
        numItems++;
        key k = key::make(m.version, index);
        return k;
    }

    /*
      Removes element (if such key exists) from the slot map.
    */
    void erase(key k) { eraseImpl<true>(k); }

    /*
      Removes element (if such key exists) from the slot map, returning the value at the key if the key was not previously removed.
    */
    std::optional<T> pop(key k)
    {
        T* val = get(k);
        if (val == nullptr)
        {
            return {};
        }

        T res(std::move(*val));
        eraseImpl<true>(k);
        return res;
    }

    /*
      Returns true if the slot map is empty.
    */
    bool empty() const noexcept { return (numItems == 0); }

    /*
      Returns the number of elements in the slot map.
    */
    size_type size() const noexcept { return numItems; }

    /*
      Exchanges the content of the slot map by the content of another slot map object of the same type.
    */
    void swap(slot_map& other) noexcept
    {
        pages.swap(other.pages);
        freeIndices.swap(other.freeIndices);
        std::swap(numItems, other.numItems);
        std::swap(maxValidIndex, other.maxValidIndex);
    }

    // copy constructor
    slot_map(const slot_map& other)
        : numItems(0)
        , maxValidIndex(0)
    {
        copyFrom(other);
    }

    // copy assignment
    slot_map& operator=(const slot_map& other)
    {
        copyFrom(other);
        return *this;
    }

    // move constructor
    slot_map(slot_map&& other) noexcept
        : numItems(other.numItems)
        , maxValidIndex(other.maxValidIndex)
    {
        std::swap(pages, other.pages);
        std::swap(freeIndices, other.freeIndices);
        other.numItems = 0;
        other.maxValidIndex = 0;
    }

    // move asignment
    slot_map& operator=(slot_map&& other) noexcept
    {
        // reset and swap
        reset();

        pages.swap(other.pages);
        freeIndices.swap(other.freeIndices);
        std::swap(numItems, other.numItems);
        std::swap(maxValidIndex, other.maxValidIndex);
        return *this;
    }

    struct Stats
    {
        size_type numPagesTotal = 0;
        size_type numInactivePages = 0;
        size_type numActivePages = 0;

        size_type numItemsTotal = 0;
        size_type numAliveItems = 0;
        size_type numTombstoneItems = 0;
        size_type numInactiveItems = 0;
    };

    /*
      Returns the internal stats for debug purposes

      Note: O(n) complexity!
    */
    Stats debug_stats() const noexcept
    {
        Stats stats;
        stats.numPagesTotal = static_cast<size_type>(pages.size());

        for (size_t pageIndex = 0; pageIndex < pages.size(); pageIndex++)
        {
            const Page& page = pages[pageIndex];
            if (page.meta == nullptr)
            {
                stats.numInactivePages++;
                continue;
            }
            stats.numActivePages++;

            stats.numItemsTotal += page.numUsedElements;
            for (size_type elementIndex = 0; elementIndex < page.numUsedElements; elementIndex++)
            {
                PageAddr addr;
                addr.page = static_cast<size_type>(pageIndex);
                addr.index = static_cast<size_type>(elementIndex);
                const Meta& m = getMetaByAddr(addr);
                if (m.inactive != 0)
                {
                    stats.numInactiveItems++;
                }
                else if (m.tombstone != 0)
                {
                    stats.numTombstoneItems++;
                }
                else
                {
                    stats.numAliveItems++;
                }
            }
        }
        return stats;
    }

  public:
    // iterators.....

    // for(const auto& kv : slot_map.items()) {} // key/value
    // for(const auto& v : slot_map) {} // values only

    // values iterator...................................
    class const_values_iterator
    {
      private:
        const T* getCurrent() const noexcept
        {
            SLOT_MAP_ASSERT(currentIndex <= slotMap->getMaxValidIndex());
            PageAddr addr = slotMap->getAddrFromIndex(currentIndex);
            SLOT_MAP_ASSERT(slotMap->isActivePage(addr));
            const ValueStorage& v = slotMap->getValueByAddr(addr);
            const T* value = reinterpret_cast<const T*>(&v);
            return value;
        }

      public:
        explicit const_values_iterator(const slot_map* _slotMap, size_type index) noexcept
            : slotMap(_slotMap)
            , currentIndex(index)
        {
        }

        const T& operator*() const noexcept { return *getCurrent(); }
        const T* operator->() const noexcept { return getCurrent(); }

        bool operator==(const const_values_iterator& other) const noexcept
        {
            return slotMap == other.slotMap && currentIndex == other.currentIndex;
        }
        bool operator!=(const const_values_iterator& other) const noexcept
        {
            return slotMap != other.slotMap || currentIndex != other.currentIndex;
        }

        const_values_iterator& operator++() noexcept
        {
            do
            {
                currentIndex++;
            } while (currentIndex <= slotMap->getMaxValidIndex() && slotMap->isTombstone(currentIndex));
            return *this;
        }

        const_values_iterator operator++(int) noexcept
        {
            const_values_iterator res = *this;
            ++*this;
            return res;
        }

      private:
        const slot_map* slotMap;
        size_type currentIndex;
    };

    const_values_iterator begin() const noexcept
    {
        if (pages.empty()) return end();

        size_type index = 0;
        while (index <= getMaxValidIndex() && isTombstone(index))
        {
            index++;
        }
        return const_values_iterator(this, index);
    }
    const_values_iterator end() const noexcept { return const_values_iterator(this, getMaxValidIndex() + static_cast<size_type>(1)); }

    // key-value iterator...................................
    class const_kv_iterator
    {
      public:
        // pretty much similar to std::reference_wrapper
        template <typename TYPE> struct reference
        {
            TYPE* ptr = nullptr;

            explicit reference(TYPE* _ptr)
                : ptr(_ptr)
            {
            }

            reference(const reference& /*other*/) noexcept = default;
            reference(reference&& /*other*/) noexcept = default;
            reference& operator=(const reference& /*other*/) noexcept = default;
            reference& operator=(reference&& /*other*/) noexcept = default;

            void set(TYPE* _ptr) noexcept { ptr = _ptr; }

            TYPE& get() const noexcept
            {
                SLOT_MAP_ASSERT(ptr);
                return *ptr;
            }

            operator TYPE&() const noexcept { return get(); }
        };

        using KeyValue = std::pair<key, const reference<const T>>;

      private:
        void updateTmpKV() const noexcept
        {
            SLOT_MAP_ASSERT(currentIndex <= slotMap->getMaxValidIndex());
            PageAddr addr = slotMap->getAddrFromIndex(currentIndex);
            SLOT_MAP_ASSERT(slotMap->isActivePage(addr));
            const Meta& m = slotMap->getMetaByAddr(addr);
            const ValueStorage& v = slotMap->getValueByAddr(addr);
            const T* value = reinterpret_cast<const T*>(&v);
            tmpKv.first = key::make(m.version, index_t(currentIndex));
            const reference<const T>& ref = tmpKv.second;
            const_cast<reference<const T>&>(ref).set(value);
        }

      public:
        explicit const_kv_iterator(const slot_map* _slotMap, size_type index) noexcept
            : slotMap(_slotMap)
            , currentIndex(index)
            , tmpKv(key::invalid(), reference<const T>(nullptr))
        {
        }

        const KeyValue& operator*() const noexcept
        {
            updateTmpKV();
            return tmpKv;
        }

        const KeyValue* operator->() const noexcept
        {
            updateTmpKV();
            return &tmpKv;
        }

        bool operator==(const const_kv_iterator& other) const noexcept
        {
            return slotMap == other.slotMap && currentIndex == other.currentIndex;
        }
        bool operator!=(const const_kv_iterator& other) const noexcept
        {
            return slotMap != other.slotMap || currentIndex != other.currentIndex;
        }

        const_kv_iterator& operator++() noexcept
        {
            do
            {
                currentIndex++;
            } while (currentIndex <= slotMap->getMaxValidIndex() && slotMap->isTombstone(currentIndex));
            return *this;
        }

        const_kv_iterator operator++(int) noexcept
        {
            const_kv_iterator res = *this;
            ++*this;
            return res;
        }

      private:
        const slot_map* slotMap;
        size_type currentIndex;
        // unfortunately we need this to make it look like standard STL iterator (operator* and operator->)
        mutable KeyValue tmpKv;
    };

    // proxy items object
    class Items
    {
        const slot_map* slotMap;

      public:
        explicit Items(const slot_map* _slotMap) noexcept
            : slotMap(_slotMap)
        {
        }

        const_kv_iterator begin() const noexcept
        {
            if (slotMap->pages.empty()) return end();
            size_type index = 0;
            while (index <= slotMap->getMaxValidIndex() && slotMap->isTombstone(index))
            {
                index++;
            }
            return const_kv_iterator(slotMap, index);
        }
        const_kv_iterator end() const noexcept
        {
            return const_kv_iterator(slotMap, slotMap->getMaxValidIndex() + static_cast<size_type>(1));
        }
    };

    Items items() const noexcept { return Items(this); }

  private:
    std::vector<Page, stl::Allocator<Page>> pages;
    std::deque<key, stl::Allocator<key>> freeIndices;
    size_type numItems;
    index_t maxValidIndex;
};

template <class T, size_t PAGESIZE = 4096, size_t MINFREEINDICES = 64>
using slot_map32 = slot_map<T, dod::slot_map_key32<T>, PAGESIZE, MINFREEINDICES>;

template <class T, size_t PAGESIZE = 4096, size_t MINFREEINDICES = 64>
using slot_map64 = slot_map<T, dod::slot_map_key64<T>, PAGESIZE, MINFREEINDICES>;

} // namespace dod

// std::hash support
namespace std
{
template <typename T> struct hash<typename dod::slot_map_key64<T>>
{
    size_t operator()(const typename dod::slot_map_key64<T>& key) const noexcept { return key.hash(); }
};

template <typename T> struct hash<typename dod::slot_map_key32<T>>
{
    size_t operator()(const typename dod::slot_map_key32<T>& key) const noexcept { return key.hash(); }
};

} // namespace std
