#ifndef SLKCACHED_BUFFER_H
#define SLKCACHED_BUFFER_H

#include <stdlib.h>

struct buffer
{
    char *content;
    size_t size;
};

void
buffer_clear(struct buffer *buffer);

void
buffer_copy(struct buffer *dest, struct buffer *src);

void
buffer_from_string(struct buffer *dest, char *str);

int
buffer_compare(struct buffer *a, struct buffer *b);

int
buffer_compare_string(struct buffer *buffer, char *string);

#endif //SLKCACHED_BUFFER_H
