#ifndef SLKCACHED_HASHMAP_H
#define SLKCACHED_HASHMAP_H

#include <stdint.h>

#define SUCCESS 0
#define NOT_FOUND 1
#define ALREADY_UPDATED 2

#define INTERNAL_ERROR -1

int
hashmap_init(uint64_t table_size);

int
hashmap_clear();

int
hashmap_add(uint8_t *key, size_t key_size, uint8_t *value, size_t value_size);

int
hashmap_check_and_set(uint8_t *key, size_t key_size, uint8_t *value, size_t value_size, uint64_t cas);

int
hashmap_remove(uint8_t *key, size_t key_size);

int
hashmap_find(uint8_t *key, size_t key_size, uint8_t **value, size_t *value_size, uint64_t *cas);

int
hashmap_remove_lru();

#endif //SLKCACHED_HASHMAP_H
