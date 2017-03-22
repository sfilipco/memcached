#include <memory.h>

#include "buffer.h"
#include "memory.h"


void
buffer_clear(struct buffer_t *buffer)
{
    memory_free(buffer->content);
    /*
     * Could setting the size and content help us or will it cause troubles?
     * Let's assume for now that they are always random values
    buffer->size = 0;
    buffer->content = 0;
     */
}

void
buffer_copy(struct buffer_t *dest, struct buffer_t *src)
{
    dest->size = src->size;
    dest->content = memory_allocate(src->size);
    memcpy(dest->content, src->content, src->size);
}

void
buffer_from_string(struct buffer_t *dest, char *str)
{
    dest->size = strlen(str);
    dest->content = memory_allocate(dest->size);
    memcpy(dest->content, str, dest->size);
}

int
buffer_compare(struct buffer_t *a, struct buffer_t *b)
{
    if (a->size < b->size) return -1;
    if (a->size > b->size) return 1;
    for (size_t i = 0; i < a->size; ++i)
    {
        // the compiler optimizes these operations so we don't need to worry about iterating with pointers
        if (a->content[i] < b->content[i]) return -1;
        if (a->content[i] > b->content[i]) return 1;
    }
    return 0;
}
