#ifndef SLKCACHED_BUFFER_H
#define SLKCACHED_BUFFER_H

#include <stdlib.h>

struct buffer_t {
    size_t size;
    char *content;
};

void
buffer_clear(struct buffer_t *buffer);

void
buffer_copy(struct buffer_t *dest, struct buffer_t *src);

void
buffer_from_string(struct buffer_t *dest, char *str);

int
buffer_compare(struct buffer_t *a, struct buffer_t *b);

#endif //SLKCACHED_BUFFER_H
