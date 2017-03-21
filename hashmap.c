//
// Created by Stefan Filip on 3/20/17.
//

#include <memory.h>

#include "buffer.h"
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

    hashmap->sentinel = malloc(sizeof(struct lru_node_t));
    hashmap->sentinel->prev = hashmap->sentinel;
    hashmap->sentinel->next = hashmap->sentinel;

    return hashmap;
}

size_t
hash(struct buffer_t *buffer, size_t table_size)
{
    // taken from http://stackoverflow.com/questions/7666509/hash-function-for-string
    // we could do better http://burtleburtle.net/bob/c/SpookyV2.h
    size_t response = 5381;
    for (size_t i = 0; i < buffer->size; ++i)
    {
        response = ((response << 5) + response) + buffer->content[i];
    }
    return response & (table_size-1);
}

void
lru_hook_node_before_sentinel(struct lru_node_t *node, struct lru_node_t *sentinel)
{
    node->next = sentinel;
    node->prev = sentinel->prev;
    sentinel->prev->next = node;
    sentinel->prev = node;
}

void
lru_remove_node_from_list(struct lru_node_t *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

int64_t
hashmap_add(struct hashmap_t *hashmap, struct buffer_t *key, struct buffer_t *value)
{
    // TODO: Check that key does not exist
    struct hashmap_item_t *item = malloc(sizeof(struct hashmap_item_t));

    buffer_copy(&item->key, key);
    buffer_copy(&item->value, value);

    item->cas = hashmap->cas_index++;

    size_t table_index = hash(key, hashmap->table_size);
    struct hashmap_item_t *next = hashmap->table[table_index];
    hashmap->table[table_index] = item;
    item->next = next;

    struct lru_node_t *lru_node = malloc(sizeof(struct lru_node_t));
    lru_hook_node_before_sentinel(lru_node, hashmap->sentinel);
    item->lru_node = lru_node;
    lru_node->hashmap_item = item;

    return item->cas;
}

int64_t
hashmap_check_and_set(struct hashmap_t *hashmap, struct buffer_t *key, int64_t cas_value, struct buffer_t *value)
{
    size_t table_index = hash(key, hashmap->table_size);
    struct hashmap_item_t *item;
    for (item = hashmap->table[table_index]; item; item = item->next)
    {
        if (item->cas == cas_value)
        {
            lru_remove_node_from_list(item->lru_node);
            lru_hook_node_before_sentinel(item->lru_node, hashmap->sentinel);
            item->cas = hashmap->cas_index++;
            buffer_clear(&item->value);
            buffer_copy(&item->value, value);
            return item->cas;
        }
    }
    return -1;
}


int
hashmap_remove(struct hashmap_t *hashmap, struct buffer_t *key)
{
    size_t table_index = hash(key, hashmap->table_size);
    struct hashmap_item_t *item = hashmap->table[table_index];
    if (buffer_compare(&item->key, key) == 0)
    {
        hashmap->table[table_index] = item->next;
    } else {
        while (item->next != NULL && buffer_compare(&item->next->key, key) != 0) {
            item = item->next;
        }
        if (item->next == NULL) {
            return -1;
        }
        struct hashmap_item_t *copy = item->next;
        item->next = item->next->next;
        item = copy;
    }

    lru_remove_node_from_list(item->lru_node);
    free(item->lru_node);

    buffer_clear(&item->key);
    buffer_clear(&item->value);

    free(item);

    return 0;
}

struct hashmap_item_t *
hashmap_find(struct hashmap_t *hashmap, struct buffer_t *key)
{
    size_t table_index = hash(key, hashmap->table_size);
    struct hashmap_item_t *item;
    for (item = hashmap->table[table_index]; item; item = item->next)
    {
        if (buffer_compare(&item->key, key) == 0)
        {
            lru_remove_node_from_list(item->lru_node);
            lru_hook_node_before_sentinel(item->lru_node, hashmap->sentinel);
            return item;
        }
    }
    return NULL;
}
