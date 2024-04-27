# Heap Memory Allocator

## Description

This project implements a custom heap memory allocator in C, providing functionalities for dynamic memory allocation, deallocation, and reallocation. It includes features for thread safety and memory management.

## Features

- Thread-safe memory allocation and deallocation using `pthread_mutex`.
- Support for `malloc`, `calloc`, `realloc`, and `free` operations.
- Dynamic memory management with coalescing of adjacent free blocks and partial handling of fragmentation.
- Implementation of a custom heap memory allocator with a hash table for efficient free block lookup and doubly lined list for quick bidirectional traversing.

## Time and Memory Complexity

- **Time Complexity**:
  - malloc and free have an average time complexity of O(1).
  - calloc and realloc have an average time complexity of O(size).

- **Memory Complexity**:
  - The memory allocator uses additional memory for metadata such as flags and pointers to manage allocated and free memory blocks. However, the overhead is typically minimal compared to the allocated memory itself.

## Generating Static or Dynamic Library

To generate the heap memory allocator as either a static or dynamic library, you can use the provided Makefile.

- **Static Library**:
  ```bash
  make libhmm.a
  ```
- **Dynamic Library**:
  ```bash
  make libhmm.so
  ```
## Testing Procedure

To test the functionality of the heap memory allocator, follow these steps:

1. Compile the test program `test.c` like the following:
   ```bash
   gcc -o test.exe test.c -L. -lhmm
   ````
2. if you are using dynamic library version, run this command:
   ```bash
   export LD_LIBRARY_PATH=$(pwd)
   ```
3. finally run the bash script:
   ```bash
   ./run_executable.sh test.exe
   ```
   
