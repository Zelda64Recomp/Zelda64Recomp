This file is originally from the repo: https://github.com/SergeyMakeev/SlotMap

The original license and README are as follows:
```
MIT License

Copyright (c) 2022 Sergey Makeev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
# Slot Map

[![Actions Status](https://github.com/SergeyMakeev/slot_map/workflows/build/badge.svg)](https://github.com/SergeyMakeev/slot_map/actions)
[![Build status](https://ci.appveyor.com/api/projects/status/i00kv17e3ia5jr7q?svg=true)](https://ci.appveyor.com/project/SergeyMakeev/slot-map)
[![codecov](https://codecov.io/gh/SergeyMakeev/slot_map/branch/main/graph/badge.svg?token=3GRAFTRYQU)](https://codecov.io/gh/SergeyMakeev/slot_map)
![MIT](https://img.shields.io/badge/license-MIT-blue.svg)

A Slot Map is a high-performance associative container with persistent unique keys to access stored values. Upon insertion, a key is returned that can be used to later access or remove the values. Insertion, removal, and access are all guaranteed to take `O(1)` time (best, worst, and average case)  
Great for storing collections of objects that need stable, safe references but have no clear ownership.

The difference between a `std::unordered_map` and a `dod::slot_map` is that the slot map generates and returns the key when inserting a value. A key is always unique and will only refer to the value that was inserted.

  Usage example:
  ```cpp
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
  
# Implementation details

The slot map container will allocate memory in pages (default page size = 4096 elements) to avoid memory spikes during growth and be able to deallocate pages that are no longer needed.
Also, the page-based memory allocator is very important since it guarantees "pointers stability"; hence, we never move values in memory.


Keys are always uses `uint64_t/uint32_t` (configurable) and technically typless, but we "artificially" make them typed to get a few extra compile-time checks.  
i.e., the following code will produce a compiler error
```cpp
slot_map<std::string> strings;
slot_map<int> numbers;
slot_map<int>::key numKey = numbers.emplace(3);
const std::string* value = strings.get(numKey);   //  <---- can not use slot_map<int>::key to index slot_map<std::string> !
```

The keys can be converted to/from their numeric types if you do not need additional type checks.
```cpp
slot_map<int> numbers;
slot_map<int>::key numKey = numbers.emplace(3);
uint64_t rawKey = numKey;  // convert to numeric type (like cast pointer to void*)
...
slot_map<int>::key numKey2{rawKey}; // create key from numeric type
```

When a slot is reused, its version is automatically incremented (to invalidate all existing keys that refers to the same slot).
But since we only use 20-bits *(10-bits for 32 bit keys)* for version counter, there is a possibility that the version counter will wrap around,
and a new item will get the same key as a removed item.

To mitigate this potential issue, once the version counter overflows, we disable that slot so that no new keys are returned for this slot
(this gives us a guarantee that there are no key collisions)

To prevent version overflow from happening too often, we need to ensure that we don't reuse the same slot too often.
So we do not reuse recently freed slot-indices as long as their number is below a certain threshold (`kMinFreeIndices = 64`).

Keys also can carry a few extra bits of information provided by a user that we called `tag`.  
That might be handy to add application-specific data to keys.

For example:
```cpp
  slot_map<std::string> strings;
  auto red = strings.emplace("Red");
  red.set_tag(13);
  
  auto tag = red.get_tag();
  assert(tag == 13);
```

Here is how a key structure looks like internally

64-bit key type

| Component      |  Number of bits        |
| ---------------|------------------------|
| tag            |  12                    |
| version        |  20 (0..1,048,575      |
| index          |  32 (0..4,294,967,295) |

32-bit key type

| Component      |  Number of bits     |
| ---------------|---------------------|
| tag            |  2                  |
| version        |  10 (0..1023)       |
| index          |  20 (0..1,048,575)  |

Note: To use your custom memory allocator define `SLOT_MAP_ALLOC`/`SLOT_MAP_FREE` before including `"slot_map.h"`

```cpp
#define SLOT_MAP_ALLOC(sizeInBytes, alignment) aligned_alloc(alignment, sizeInBytes)
#define SLOT_MAP_FREE(ptr) free(ptr)
#include "slot_map.h"
```


# API
  
`bool has_key(key k) const noexcept`  
Returns true if the slot map contains a specific key  
    
`void reset()`  
Clears the slot map and releases any allocated memory.  
Note: By calling this function, you must guarantee that no handles are in use!  
Otherwise calling this function might be dangerous and lead to key "collisions".  
You might consider using "clear()" instead.  
  
`void clear()`  
Clears the slot map but keeps the allocated memory for reuse.  
Automatically increases version for all the removed elements (the same as calling "erase()" for all existing elements)  
      
`const T* get(key k) const noexcept`  
If key exists returns a const pointer to the value corresponding to the given key or returns null elsewere.  
      
`T* get(key k)`  
If key exists returns a pointer to the value corresponding to the given key or returns null elsewere.  
      
`key emplace(Args&&... args)`  
Constructs element in-place and returns a unique key that can be used to access this value.  
      
`void erase(key k)`  
Removes element (if such key exists) from the slot map.  
      
`std::optional<T> pop(key k)`  
Removes element (if such key exists) from the slot map, returning the value at the key if the key was not previously removed.  
      
`bool empty() const noexcept`  
Returns true if the slot map is empty.  
   
`size_type size() const noexcept`  
Returns the number of elements in the slot map.  

`void swap(slot_map& other) noexcept`  
Exchanges the content of the slot map by the content of another slot map object of the same type.  
  
`slot_map(const slot_map& other)`  
Copy constructor  

`slot_map& operator=(const slot_map& other)`  
Copy assignment  

`slot_map(slot_map&& other) noexcept`  
Move constructor

`slot_map& operator=(slot_map&& other) noexcept`  
Move asignment


`const_values_iterator begin() const noexcept`  
`const_values_iterator end() const noexcept`  
Const values iterator

```cpp
for (const auto& value : slotMap)
{
 ...
}
```

`Items items() const noexcept`  
Const key/value iterator

```cpp
for (const auto& [key, value] : slotMap.items())
{
...
}
```
  
# References

  Sean Middleditch  
  Data Structures for Game Developers: The Slot Map, 2013  
  https://web.archive.org/web/20180121142549/http://seanmiddleditch.com/data-structures-for-game-developers-the-slot-map/ 
  
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
  
  Niklas Gray  
  Data Structures Part 1: Bulk Data, 2019  
  https://ourmachinery.com/post/data-structures-part-1-bulk-data/  
  
  
