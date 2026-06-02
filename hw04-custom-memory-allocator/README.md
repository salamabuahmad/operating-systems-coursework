# Custom Memory Allocator

## Overview

This project implements a custom dynamic memory allocator in C++.

The allocator provides custom versions of memory-management functions such as `smalloc`, `scalloc`, `sfree`, and `srealloc`.

The final implementation uses a buddy-allocation approach for smaller allocations and `mmap` for larger allocations.

This project was created as part of an Operating Systems course.

## Features

* Custom `smalloc`
* Custom `scalloc`
* Custom `sfree`
* Custom `srealloc`
* Metadata tracking for allocated blocks
* Free-list management
* Buddy block splitting
* Buddy block merging
* `sbrk`-based heap allocation
* `mmap` support for large allocations
* Allocation statistics functions

## Technologies Used

* C++
* Linux memory management
* `sbrk`
* `mmap`
* Buddy allocation
* Free lists
* Low-level systems programming
* Operating Systems

## Project Structure

```text
hw04-custom-memory-allocator/
├── malloc_3.cpp
└── stages/
    ├── malloc_1.cpp
    └── malloc_2.cpp
```

## Project Files

* `malloc_3.cpp` - final allocator implementation
* `stages/malloc_1.cpp` - basic allocator using `sbrk`
* `stages/malloc_2.cpp` - allocator with metadata and free-list reuse

## Implemented API

```cpp
void* smalloc(size_t size);
void* scalloc(size_t num, size_t size);
void sfree(void* p);
void* srealloc(void* oldp, size_t size);

size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _num_meta_data_bytes();
size_t _size_meta_data();
```

## Implementation Notes

The allocator stores metadata for each allocated block and uses free lists to track reusable memory blocks.

The final version uses a buddy-style allocation strategy. Smaller memory requests are handled through managed blocks that can be split and merged with their buddy blocks. Larger allocations are handled separately using `mmap`.

This separation helps reduce unnecessary heap fragmentation and keeps large allocations independent from the smaller block-management system.

## What I Learned

* How dynamic memory allocators manage heap memory
* How metadata is stored and used to track allocated blocks
* How free lists reduce unnecessary system calls
* How buddy allocation supports block splitting and merging
* How large allocations can be handled separately using `mmap`
* Why memory allocators are powerful, fragile, and easy to break in deeply creative ways
