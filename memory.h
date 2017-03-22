#ifndef SLKCACHED_MEMORY_H
#define SLKCACHED_MEMORY_H

#include <memory.h>

void
set_memory_limit(size_t limit);

void *
memory_allocate(size_t size);

void *
memory_free(void *address);

#endif //SLKCACHED_MEMORY_H
