#ifndef SLKCACHED_HASHMAP_H
#define SLKCACHED_HASHMAP_H

#include <stdlib.h>
#include <stdint.h>

// consider making hashmap_item more private
struct hashmap_item_t {
    char *key;
    size_t key_size;

    char *value;
    size_t value_size;

    int64_t cas;

    struct hashmap_item_t *next;
    struct lru_node_t *lru_node;
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
hashmap_add(struct hashmap_t *hashmap, char *key, size_t key_size, char *value, size_t value_size);

int64_t
hashmap_check_and_set(struct hashmap_t *hashmap, char *key, size_t key_size, int64_t cas_value,
                      char *value, size_t value_size);

int
hashmap_remove(struct hashmap_t *hashmap, char *key, size_t key_size);

struct hashmap_item_t *
hashmap_find(struct hashmap_t *hashmap, char *key, size_t key_size);

#endif //SLKCACHED_HASHMAP_H
