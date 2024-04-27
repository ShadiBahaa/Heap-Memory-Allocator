/**
 * @file HMM.c
 * @brief Source file containing a custom heap memory allocator implementation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>

/**
 * @struct mem_chunk_t
 * @brief Represents a memory chunk metadata.
 */

typedef struct mem_chunk
{
    /**< Flag indicating whether the memory chunk added to hash table or not. */ 
    unsigned char is_added;
    /**< Flag indicating whether the memory chunk is free or allocated. */   
    unsigned char is_free;       
    /**< Size of the memory chunk (excluding the metadata). */    
    size_t size; 
    /**< Pointer to the previous memory chunk in the linked list. */                    
    struct mem_chunk *prev;
    /**< Pointer to the next memory chunk in the linked list. */
    struct mem_chunk *next;
    /**< Pointer to the next free memory chunk with the same size in the hash table. */          
    struct mem_chunk *next_free;     
} mem_chunk_t;
/**< Alignment requirement for memory allocation. */
#define ALIGNMENT 8
 /**< Size of the allocated memory block in sbrk call. */                                 
#define ALLOCATED_BYTES (8 * 1024 * 1024)  
/**< Maximum number of solts for hash table */        
#define MULTIPLES_MAX (ALLOCATED_BYTES / ALIGNMENT) 

/**< Array of linked lists containing free memory blocks based on size. (Hash table) */

static mem_chunk_t *block_freq[MULTIPLES_MAX];  

/**< Mutex for thread safety. */

static pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER; 

/**< Pointer to the head of the memory chunk linked list. */

static mem_chunk_t *head; 

/**< Pointer to the tail of the memory chunk linked list. */

static mem_chunk_t *tail; 

/**< Current free memory size. */

static size_t current_free_size; 

/**
 * @brief Removes a free memory block from the list of free blocks in hash table.
 * 
 * @param block Pointer to the memory block to be removed.
 */

static void HMMremove_free_block(mem_chunk_t *block)
{
    if (block == NULL)
        return;
        // Calculate the index for the block based on its size.

    size_t cur_size = block->size;
    size_t idx = (cur_size / ALIGNMENT) - 1;
        // If the index is within bounds, traverse the free list to remove the block.

    if (idx < MULTIPLES_MAX)
    {
        mem_chunk_t *current = block_freq[idx];
        mem_chunk_t *prev = NULL;
        while (current)
        {
            if (current == block)
            {
                // Update flags and sizes accordingly.
                current->is_added = 0;
                current_free_size -= current->size;
                if (prev)
                {
                    prev->next_free = current->next_free;
                }
                else
                {
                    block_freq[idx] = block_freq[idx]->next_free;
                }
                break;
            }
            prev = current;
            current = current->next_free;
        }
    }
}
/**
 * @brief Checks if a memory block is found in the list of free blocks in hash table.
 * 
 * @param block Pointer to the memory block to be checked.
 * @return 1 if the block is found, 0 otherwise.
 */
static unsigned char HMMis_block_found(mem_chunk_t *block)
{
    return block->is_added;
}
/**
 * @brief Adds a free memory block to the list of free blocks in hash table.
 * 
 * @param block Pointer to the memory block to be added.
 */
static void HMMadd_free_block(mem_chunk_t *block)
{
    if (block == NULL)
        return;
// Calculate the index for the block based on its size.
    size_t cur_size = block->size;
    size_t idx = (cur_size / ALIGNMENT) - 1;
        // If the index is within bounds and the block is not already added, add it to the free list.

    if (idx < MULTIPLES_MAX)
    {
        if (HMMis_block_found(block))
            return;
        current_free_size+=block->size;
        block->is_added = 1;
        if (block_freq[idx] == NULL)
        {
            block_freq[idx] = block;
            block->next_free = NULL;
        }
        else
        {
            block->next_free = block_freq[idx];
            block_freq[idx] = block;
        }
    }
}
/**
 * @brief Retrieves a free memory block of a specified size in hash table based on size.
 * 
 * @param size Size of the memory block to retrieve.
 * @return Pointer to the free memory block if found, NULL otherwise.
 */
static mem_chunk_t *HMMget_free_block(size_t size)
{
    size_t idx = (size / ALIGNMENT) - 1;
    if (idx < MULTIPLES_MAX)
    {
        if (block_freq[idx])
        {
            mem_chunk_t *ret = block_freq[idx];
            current_free_size-=ret->size;
            ret->is_added = 0;
            block_freq[idx] = block_freq[idx]->next_free;
            return ret;
        }
    }
    return NULL;
}
/**
 * @brief Coalesces adjacent free memory blocks starting from a specified block.
 * 
 * @param start Pointer to the starting memory block for coalescing.
 */
static void HMMcoalesce(mem_chunk_t *start)
{
    mem_chunk_t *current = start;
    if (current != NULL)
    {
        mem_chunk_t *last_collecting = NULL;
        size_t total_size = 0;
        while (current)
        {
            if (current && (current->is_free == 1))
            {
                if (last_collecting == NULL)
                {
                    last_collecting = current;
                }
                else
                {
                    total_size += current->size + sizeof(mem_chunk_t);
                }
            }
            else
            {
                break;
            }
            HMMremove_free_block(current);
            current = current->next;
        }
        if (total_size > 0)
        {
            last_collecting->size += total_size;
            last_collecting->next = current;
            if (current)
                current->prev = last_collecting;
            if (current == NULL)
            {
                tail = last_collecting;
                tail->next = NULL;
            }
        }
    }
}
/**
 * @brief Retrieves/Creates a free memory chunk of a specified size.
 * 
 * @param size Size of the memory chunk to retrieve.
 * @return Pointer to the free memory chunk if found, NULL otherwise.
 */
static mem_chunk_t *HMMget_free_chunk(size_t size)
{    // Try to get a free block from the free list.

    mem_chunk_t *get_free = HMMget_free_block(size);
    if (get_free)
    {
        return get_free;
    }
        // If no suitable free block found, allocate new memory.

    mem_chunk_t *current = tail;
    while (current)
    {
        if ((current->is_free == 1) && (current->size >= size))
        {
                        // Remove the block from the free list.

            HMMremove_free_block(current);
                        // Split the block if it's larger than required.

            if ((current->size > (sizeof(mem_chunk_t) + size)))
            {
                mem_chunk_t *current_next = current->next;
                mem_chunk_t *splitted = (mem_chunk_t *)((size_t)current + sizeof(mem_chunk_t) + size);
                // Initialize the new block.

                splitted->prev = current;
                splitted->next = current_next;
                splitted->is_free = 1;
                if (current_next)
                    current_next->prev = splitted;
                current->next = splitted;
                if (current == tail)
                {
                    tail = splitted;
                    tail->next = NULL;
                }
                splitted->size = current->size - size - sizeof(mem_chunk_t);
                HMMadd_free_block(splitted);
                current->size = size;
            }
            return current;
        }
        else if (current->is_free == 1)
        {
            HMMadd_free_block(current);
        }
        current = current->prev;
    }
        // If no free block is large enough, allocate new memory from the system.

    size_t allocation_size = ALLOCATED_BYTES;
    size_t num_allocated_bytes = ((size + sizeof(mem_chunk_t) + allocation_size) / allocation_size) * allocation_size;
    void *new_free_space = sbrk(num_allocated_bytes);
    if (new_free_space == (void *)-1)
    {
        write(STDOUT_FILENO,"FAILED\n",8);
        return NULL;
    }
        // Remove the tail block from the free list and update it.

    HMMremove_free_block(tail);
    if (tail && (tail->is_free == 1))
    {
        tail->size += num_allocated_bytes;
        tail->next = NULL;
        return HMMget_free_chunk(size);
    }
        // Initialize a new memory chunk and add it to the list.

    mem_chunk_t *new_chunk = (mem_chunk_t *)new_free_space;
    new_chunk->is_free = 1;
    new_chunk->size = num_allocated_bytes - sizeof(mem_chunk_t);
    new_chunk->prev = tail;
    new_chunk->next = NULL;
    if (tail)
        tail->next = new_chunk;
    tail = new_chunk;
    tail->next = NULL;
    if (head == NULL)
        head = tail;
    HMMadd_free_block(tail);
    return HMMget_free_chunk(size);
}
/**
 * @brief Frees the memory associated with a given pointer.
 * 
 * @param ptr Pointer to the memory to be freed.
 */
static void HMMfree(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    // Get the memory chunk from the given pointer.

    mem_chunk_t *alloacted_member = (mem_chunk_t *)((unsigned char *)ptr - sizeof(mem_chunk_t)); 
        // Check if the memory is already free, if not, free it.

    if (alloacted_member->is_free == 0)
    {
        alloacted_member->is_free = 1;
    }
    else
    {
        return;
    }
    // Coalesce adjacent free blocks.

    if (alloacted_member->prev && alloacted_member->prev->is_free == 1)
    {
        HMMcoalesce(alloacted_member->prev);
        HMMadd_free_block(alloacted_member->prev);
    }
    else if (alloacted_member->next && alloacted_member->next->is_free == 1)
    {
        HMMcoalesce(alloacted_member);
        HMMadd_free_block(alloacted_member);
    }
    else
    {
        HMMadd_free_block(alloacted_member);
    }
        // Free excess memory if available.

    mem_chunk_t *current = tail;
    size_t total_size = 0;
    if (current_free_size >= ALLOCATED_BYTES)
    {

        while (current)
        {
            if (current->is_free == 1)
            {
                total_size += current->size + sizeof(mem_chunk_t);
                HMMremove_free_block(current);
            }
            else
            {
                break;
            }
            current = current->prev;
        }
        if (total_size >= ALLOCATED_BYTES)
        {
            if (current == NULL)
            {
                head = tail = NULL;
            }
            else
            {
                tail = current;
                tail->next = NULL;
            }
            void *new_break = sbrk(-total_size);
            if (new_break == (void *)-1)
            {
                return;
            }
        }
    }
}
/**
 * @brief Allocates memory of a specified size.
 * 
 * @param size Size of the memory to allocate.
 * @return Pointer to the allocated memory if successful, NULL otherwise.
 */
static void *HMMmalloc(size_t size)
{
    if (size == 0)
    {
        size = ALIGNMENT;
    }
    size = (size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
    mem_chunk_t *allocated_area_data = HMMget_free_chunk(size);
    if (allocated_area_data == NULL)
    {
        return NULL;
    }
    allocated_area_data->is_free = 0;
    return (void *)((allocated_area_data + 1));
}
/**
 * @brief Allocates memory for an array of elements, each with a specified size.
 * 
 * @param nmemb Number of elements.
 * @param size Size of each element.
 * @return Pointer to the allocated memory if successful, NULL otherwise.
 */
static void *HMMcalloc(size_t nmemb, size_t size)
{
    if (size != 0 && (nmemb > (ULONG_MAX / size)))
    {
        return NULL;
    }
    size = ((size * nmemb) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
    if (size == 0)
        size = ALIGNMENT;
    void *allocated_area = HMMmalloc(size);
    if (allocated_area == NULL)
    {
        return NULL;
    }
    memset(allocated_area, 0, ((mem_chunk_t *)allocated_area - 1)->size);
    return allocated_area;
}
/**
 * @brief Changes the size of the memory block pointed to by a given pointer.
 * 
 * @param ptr Pointer to the previously allocated memory block.
 * @param size New size for the memory block.
 * @return Pointer to the reallocated memory block if successful, NULL otherwise.
 */
static void *HMMrealloc(void *ptr, size_t size)
{
    size = ((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
    if (ptr == NULL)
    {
        return HMMmalloc(size);
    }
    if (size == 0)
    {
        HMMfree(ptr);
        return HMMmalloc(ALIGNMENT);
    }
    if (size == ((mem_chunk_t *)ptr - 1)->size)
    {
        return ptr;
    }
    void *allocated_chunk = HMMmalloc(size);
    if (allocated_chunk == NULL)
    {
        return NULL;
    }
    size_t allocation_size;
    if (((mem_chunk_t *)ptr - 1)->size < ((mem_chunk_t *)allocated_chunk - 1)->size)
    {
        allocation_size = ((mem_chunk_t *)ptr - 1)->size;
    }
    else
    {
        allocation_size = ((mem_chunk_t *)allocated_chunk - 1)->size;
    }
    memcpy(allocated_chunk, ptr, allocation_size);
    HMMfree(ptr);
    return (void *)allocated_chunk;
}
/**
 * @brief Wrapper function for thread-safe memory allocation.
 * 
 * @param size Size of the memory to allocate.
 * @return Pointer to the allocated memory if successful, NULL otherwise.
 */
void *malloc(size_t size)
{
    if (pthread_mutex_lock(&alloc_mutex)!=0){
        return NULL;
    }
    void *ret_ptr = HMMmalloc(size);
    pthread_mutex_unlock(&alloc_mutex);
    return ret_ptr;
}
/**
 * @brief Wrapper function for thread-safe memory deallocation.
 * 
 * @param ptr Pointer to the memory to be freed.
 */
void free(void *ptr)
{
    if (pthread_mutex_lock(&alloc_mutex)!=0){
        return;
    }
    HMMfree(ptr);
    pthread_mutex_unlock(&alloc_mutex);
}
/**
 * @brief Wrapper function for thread-safe calloc.
 * 
 * @param nmemb Number of elements.
 * @param size Size of each element.
 * @return Pointer to the allocated memory if successful, NULL otherwise.
 */
void *calloc(size_t nmemb, size_t size)
{
    if (pthread_mutex_lock(&alloc_mutex)!=0){
        return NULL;
    }
    void *ret_ptr = HMMcalloc(nmemb, size);
    pthread_mutex_unlock(&alloc_mutex);
    return ret_ptr;
}
/**
 * @brief Wrapper function for thread-safe realloc.
 * 
 * @param ptr Pointer to the previously allocated memory block.
 * @param size New size for the memory block.
 * @return Pointer to the reallocated memory block if successful, NULL otherwise.
 */
void *realloc(void *ptr, size_t size)
{
    if (pthread_mutex_lock(&alloc_mutex)!=0){
        return NULL;
    }
    void *ret_ptr = HMMrealloc(ptr, size);
    pthread_mutex_unlock(&alloc_mutex);
    return ret_ptr;
}
/**
 * @brief Traverses the memory chunk linked list and prints information about each chunk.
 */
void HMMtraverse(void)
{
    mem_chunk_t *cur = head;
    size_t cnt = 1;
    while (cur)
    {
        printf("Node number: %d, Address: %10p, free: %lu, size: %lu\r\n", cnt, cur, cur->is_free, cur->size);
        cnt++;
        cur = cur->next;
    }
}