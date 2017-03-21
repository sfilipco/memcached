#ifndef SLKCACHED_HASHMAP_H
#define SLKCACHED_HASHMAP_H

#include <stdlib.h>
#include <stdint.h>
#include "buffer.h"

// consider making hashmap_item more private
// ordering to optimize bit packing though we should verify if it's still a problem in these days
struct hashmap_item_t {
    struct hashmap_item_t *next;
    struct lru_node_t *lru_node;

    int64_t cas;

    struct buffer_t key, value;
};


struct lru_node_t {
    struct hashmap_item_t *hashmap_item;
    struct lru_node_t *next, *prev;
};

struct hashmap_t {
    size_t memory_size;

    struct hashmap_item_t* *table;
    size_t table_size;

    int64_t cas_index;

    struct lru_node_t *sentinel;
};


struct hashmap_t *
hashmap_init(size_t hash_size);

int64_t
hashmap_add(struct hashmap_t *hashmap, struct buffer_t *key, struct buffer_t *value);

int64_t
hashmap_check_and_set(struct hashmap_t *hashmap, struct buffer_t *key, int64_t cas_value, struct buffer_t *value);

int
hashmap_remove(struct hashmap_t *hashmap, struct buffer_t *key);

struct hashmap_item_t *
hashmap_find(struct hashmap_t *hashmap, struct buffer_t *key);

#endif //SLKCACHED_HASHMAP_H
