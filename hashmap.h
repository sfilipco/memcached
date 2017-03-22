#ifndef SLKCACHED_HASHMAP_H
#define SLKCACHED_HASHMAP_H

#include <stdlib.h>
#include <stdint.h>

#include "buffer.h"

int
hashmap_init(uint64_t table_size);

uint64_t
hashmap_add(struct buffer_t *key, struct buffer_t *value);

uint64_t
hashmap_check_and_set(struct buffer_t *key, struct buffer_t *value, uint64_t cas_value);

int
hashmap_remove(struct buffer_t *key);

int
hashmap_find(struct buffer_t *key, struct buffer_t **value, uint64_t *cas_value);

int
hashmap_remove_lru();

#endif //SLKCACHED_HASHMAP_H
