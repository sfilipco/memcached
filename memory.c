#include "memory.h"
#include "hashmap.h"
#include <stdlib.h>
#include <printf.h>

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

    int temp;
    // Not the best separation of concerns but direct
    while (allocated_memory + size > memory_limit)
    {
        temp = hashmap_remove_lru();
        if (temp != 0)
        {
            printf("could not remove lru\n");
            hashmap_remove_lru();
        }
    }
    if (allocated_memory + size <= memory_limit)
    {
        size_t *response = malloc(size);
        if (!response)
        {
            printf("could not allocate memory");
            return NULL;
        }
        allocated_memory += size;
        *response = size;
        return response+1;
    }
    printf("how did this happen?\n");
    return NULL;
}

void *
memory_free(void *address)
{
    size_t *ptr_size = ((size_t *) address) - 1;
    allocated_memory -= *ptr_size;
    free(ptr_size);
}
