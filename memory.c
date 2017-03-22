#include <stdlib.h>
#include <stdio.h>

#include "hashmap.h"
#include "memory.h"

size_t memory_limit = 0;
size_t allocated_memory = 0;

void
set_memory_limit(size_t limit)
{
    memory_limit = limit;
}

size_t
get_allocated_memory()
{
    return allocated_memory;
}

void *
memory_allocate(size_t size)
{
    // We store the allocated size next to the allocated block
    size += sizeof(size_t);

    // Not the best separation of concerns but direct
    while (allocated_memory + size > memory_limit)
    {
        if (hashmap_remove_lru() != 0)
        {
            perror("could not remove lru element\n");
            return NULL;
        }
    }
    size_t *response = malloc(size);
    if (!response)
    {
        perror("could not allocate memory");
        return NULL;
    }
    allocated_memory += size;
    *response = size;
    return response+1;
}

void *
memory_free(void *address)
{
    size_t *ptr_size = ((size_t *) address) - 1;
    allocated_memory -= *ptr_size;
    free(ptr_size);
}
