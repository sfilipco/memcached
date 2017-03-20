//
// Created by Stefan Filip on 3/20/17.
//

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

    struct lru_node_t *lru_first, *lru_last;
};

/**
 * Creates a new hashmap
 *
 * @param hash_size
 * @return
 */
struct hashmap_t *
hashmap_init(size_t hash_size);

/**
 *
 * @param parent
 * @param key
 * @param key_size
 * @param value
 * @param value_size
 * @return cas value for this item
 */
int64_t
hashmap_add(struct hashmap_t *hashmap, char *key, size_t key_size, char *value, size_t value_size);

int
hashmap_remove(struct hashmap_t *hashmap, char *key, size_t key_size);

struct hashmap_item_t *
hashmap_find(struct hashmap_t *hashmap, char *key, size_t key_size);

#endif //SLKCACHED_HASHMAP_H
