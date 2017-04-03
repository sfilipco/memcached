#include <stdlib.h>
#include <stdio.h>

#include "hashmap.h"
#include "memory.h"

static size_t memory_limit = 0;
static size_t allocated_memory = 0;

void
set_memory_limit(size_t limit)
{
    memory_limit = limit;
}

size_t
get_memory_limit()
{
    return memory_limit;
}

size_t
get_allocated_memory()
{
    return allocated_memory;
}

void *
memory_allocate(size_t size)
{
    // We store the allocated size lru_next to the allocated block
    size += sizeof(size_t);

    // Not the best separation of concerns but direct
    while (allocated_memory + size > memory_limit) {
        if (hashmap_remove_lru() != 0) {
            hashmap_remove_lru();
            perror("could not remove lru element\n");
            return NULL;
        }
    }
    size_t *response = malloc(size);
    if (!response) {
        perror("could not allocate memory");
        return NULL;
    }
    allocated_memory += size;
    *response = size;
    return response + 1;
}

void *
memory_reallocate(void *ptr, size_t size)
{
    if (ptr == NULL) return memory_allocate(size);

    size += sizeof(size_t);

    size_t *old_ptr = ((size_t *) ptr) - 1;
    size_t old_size = *old_ptr;
    while (allocated_memory + size - old_size > memory_limit) {
        if (hashmap_remove_lru() != 0) {
            perror("reallocate could not remove lru element\n");
            return NULL;
        }
    }
    size_t *response = realloc(old_ptr, size);
    if (!response) {
        perror("could not reallocate memory");
        return NULL;
    }

    allocated_memory = allocated_memory + size - old_size;
    *response = size;
    return response + 1;
}

void
memory_free(void *address)
{
    size_t *ptr_size = ((size_t *) address) - 1;
    allocated_memory -= *ptr_size;
    free(ptr_size);
}
