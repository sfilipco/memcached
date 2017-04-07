#ifndef MEMCACHED_MEMORY_H
#define MEMCACHED_MEMORY_H

#include <memory.h>

void
set_memory_limit(size_t limit);

size_t
get_memory_limit();

size_t
get_allocated_memory();

void *
memory_allocate(size_t size);

void *
memory_reallocate(void *ptr, size_t size);

void
memory_free(void *address);

#endif //MEMCACHED_MEMORY_H
