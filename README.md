# Heap Memory Allocator

## Description

This project implements a custom heap memory allocator in C, providing functionalities for dynamic memory allocation, deallocation, and reallocation. It includes features for thread safety and memory management.

## Features

- Thread-safe memory allocation and deallocation using mutex.
- Support for `malloc`, `calloc`, `realloc`, and `free` operations.
- Dynamic memory management with coalescing of adjacent free blocks.
- Implementation of a custom heap memory allocator with a hash table for efficient free block lookup.

## Time and Memory Complexity

- **Time Complexity**:
  - Memory allocation, deallocation, and reallocation operations have an average time complexity of O(1).
  - Coalescing adjacent free blocks may incur additional time complexity based on the number of blocks to be coalesced.

- **Memory Complexity**:
  - The memory allocator uses additional memory for metadata such as flags and pointers to manage allocated and free memory blocks. However, the overhead is typically minimal compared to the allocated memory itself.

## Generating Static or Dynamic Library

To generate the heap memory allocator as either a static or dynamic library, you can use the provided Makefile.

- **Static Library**:
  ```bash
  make libhmm.a
