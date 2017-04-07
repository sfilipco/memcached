#ifndef MEMCACHED_HASHMAP_H
#define MEMCACHED_HASHMAP_H

#include <stdint.h>

#define SUCCESS 0
#define NOT_FOUND 1
#define ALREADY_UPDATED 2

#define INTERNAL_ERROR -1

struct hashmap_item {
    uint8_t *key, *value;
    uint16_t key_size;
    uint32_t value_size;
    // TODO: uint32_t flags;
    uint64_t cas;
    struct hashmap_item *table_next, *lru_next, *lru_prev;
};

int
hashmap_init(uint64_t table_size);

int
hashmap_clear();

int
hashmap_insert(uint8_t *key, uint16_t key_size, uint8_t *value, uint32_t value_size, uint64_t cas,
               struct hashmap_item **item);

int
hashmap_remove(uint8_t *key, uint16_t key_size);

int
hashmap_find(uint8_t *key, uint16_t key_size, struct hashmap_item **item);

int
hashmap_remove_lru();

#endif //MEMCACHED_HASHMAP_H
