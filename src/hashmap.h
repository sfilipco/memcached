#ifndef SLKCACHED_HASHMAP_H
#define SLKCACHED_HASHMAP_H

#include <stdlib.h>
#include <stdint.h>

#include "buffer.h"

int
hashmap_init(uint64_t table_size);

int
hashmap_add(struct buffer *key, struct buffer *value);

int
hashmap_check_and_set(struct buffer *key, struct buffer *value, uint64_t cas_value);

int
hashmap_remove(struct buffer *key);

int
hashmap_find(struct buffer *key, struct buffer **value, uint64_t *cas_value);

int
hashmap_remove_lru();

#endif //SLKCACHED_HASHMAP_H