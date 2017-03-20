//
// Created by Stefan Filip on 3/20/17.
//

#include <memory.h>
#include "hashmap.h"

struct hashmap_t *
hashmap_init(size_t memory_size)
{
    if (memory_size == 0) return NULL;

    struct hashmap_t *hashmap = malloc(sizeof(struct hashmap_t));
    hashmap->memory_size = memory_size;

    // We preallocate a table as large as we can from the start
    // This has poor memory usage when we have a few large key, values pairs
    // If we have small key, value pairs than we have run into more collisions but since we
    // are using a linked list for collisions we handle that scenario ok
    // TODO(stefan): support resizing
    hashmap->table_size = memory_size >> 2;

    hashmap->table = malloc(hashmap->table_size * sizeof(struct hashmap_item_t *));

    hashmap->cas_index = 1;

    hashmap->lru_first = hashmap->lru_last = NULL;

    return hashmap;
}

size_t
hash(char *key, size_t key_size, size_t table_size)
{
    // taken from http://stackoverflow.com/questions/7666509/hash-function-for-string
    // we could do better http://burtleburtle.net/bob/c/SpookyV2.h
    size_t response = 5381;
    for (size_t i = 0; i < key_size; ++i)
    {
        response = ((response << 5) + response) + key[i];
    }
    return response % table_size;
}

int64_t
hashmap_add(struct hashmap_t *hashmap, char *key, size_t key_size, char *value, size_t value_size)
{
    struct hashmap_item_t *item = malloc(sizeof(struct hashmap_item_t));

    item->key = malloc(key_size);
    memcpy(item->key, key, key_size);
    item->key_size = key_size;

    item->value = malloc(value_size);
    memcpy(item->value, value, value_size);
    item->value_size = value_size;

    item->cas = hashmap->cas_index++;

    size_t table_position = hash(key, key_size, hashmap->table_size);
    struct hashmap_item_t *next = hashmap->table[table_position];
    hashmap->table[table_position] = item;
    item->next = next;

    struct lru_node_t *lru_node = malloc(sizeof(struct lru_node_t));
    lru_node->hashmap_item = item;
    item->lru_node = lru_node;
    lru_node->next = NULL;
    if (hashmap->lru_last == NULL)
    {
        lru_node->prev = NULL;
        hashmap->lru_first = lru_node;
    } else {
        hashmap->lru_last->next = lru_node;
        lru_node->prev = hashmap->lru_last;
    }
    hashmap->lru_last = lru_node;

    return item->cas;
}

int
hashmap_remove(struct hashmap_t *hashmap, char *key, size_t key_size)
{
    return 0;
}

struct hashmap_item_t *
hashmap_find(struct hashmap_t *hashmap, char *key, size_t key_size)
{
    size_t table_position = hash(key, key_size, hashmap->table_size);
    struct hashmap_item_t *iterator;
    for (iterator = hashmap->table[table_position]; iterator; iterator = iterator->next) {
        if (key_size == iterator->key_size && memcmp(key, iterator->key, key_size) == 0) {
            return iterator;
        }
    }
    return NULL;
}
