#include "memory.h"
#include <stdlib.h>

size_t memory_limit = 0;
size_t allocated_memory = 0;

void
set_memory_limit(size_t limit)
{
    memory_limit = limit;
}

void *
memory_allocate(size_t size)
{
    if (allocated_memory + size <= memory_limit)
    {
        return malloc(size);
    }
    return NULL;
}

void *
memory_free(void *address)
{
    allocated_memory -= *(size_t*)address;
    free(address);
}