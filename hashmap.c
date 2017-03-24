#include <memory.h>

#include "buffer.h"
#include "hashmap.h"
#include "memory.h"

// We preallocate a hash_table as large as we can from the start
// This has poor memory usage when we have a few large key, values pairs
// If we have small key, value pairs than we have run into more collisions but since we
// are using a linked list for collisions we handle that scenario ok
// TODO: support resizing

struct hashmap_item_t {
    struct hashmap_item_t *table_next, *lru_next, *lru_prev;

    uint64_t cas;
    // TODO: uint32_t flags;

    struct buffer_t key, value;
};


struct hashmap_item_t* *hash_table;
uint64_t hash_table_size;
uint64_t cas_index, _items_in_hash;
struct hashmap_item_t *lru_sentinel;

int
hashmap_init(uint64_t _table_size)
{
    hash_table_size = _table_size;
    hash_table = memory_allocate(hash_table_size * sizeof(struct hashmap_item_t *));

    cas_index = 0;
    _items_in_hash = 0;

    lru_sentinel = memory_allocate(sizeof(struct hashmap_item_t));
    lru_sentinel->lru_prev = lru_sentinel->lru_next = lru_sentinel;

    return 0;
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
lru_hook_node_before_sentinel(struct hashmap_item_t *node, struct hashmap_item_t *sentinel)
{
    node->lru_next = sentinel;
    node->lru_prev = sentinel->lru_prev;
    sentinel->lru_prev->lru_next = node;
    sentinel->lru_prev = node;
}

void
lru_remove_node_from_list(struct hashmap_item_t *node)
{
    node->lru_prev->lru_next = node->lru_next;
    node->lru_next->lru_prev = node->lru_prev;
}

int
hashmap_add(struct buffer_t *key, struct buffer_t *value)
{
    // TODO: Check that key does not exist
    struct hashmap_item_t *item = memory_allocate(sizeof(struct hashmap_item_t));

    buffer_copy(&item->key, key);
    buffer_copy(&item->value, value);

    item->cas = ++cas_index;

    size_t table_index = hash(key, hash_table_size);
    struct hashmap_item_t *next = hash_table[table_index];
    hash_table[table_index] = item;
    item->table_next = next;

    lru_hook_node_before_sentinel(item, lru_sentinel);

    ++_items_in_hash;

    return 0;
}

int
hashmap_check_and_set(struct buffer_t *key, struct buffer_t *value, uint64_t cas_value)
{
    size_t table_index = hash(key, hash_table_size);
    struct hashmap_item_t *item;
    for (item = hash_table[table_index]; item; item = item->table_next)
    {
        if (buffer_compare(&item->key, key) == 0) {
            if (item->cas == cas_value) {
                lru_remove_node_from_list(item);
                lru_hook_node_before_sentinel(item, lru_sentinel);
                item->cas = ++cas_index;
                buffer_clear(&item->value);
                buffer_copy(&item->value, value);
                return 0;
            } else {
                return 2    ;
            }
        }
    }
    return 1;
}


int
hashmap_remove(struct buffer_t *key)
{
    size_t table_index = hash(key, hash_table_size);
    struct hashmap_item_t *item = hash_table[table_index];
    if (buffer_compare(&item->key, key) == 0)
    {
        hash_table[table_index] = item->table_next;
    } else {
        while (item->table_next != NULL && buffer_compare(&item->table_next->key, key) != 0) {
            item = item->table_next;
        }
        if (item->table_next == NULL) {
            return -1;
        }
        struct hashmap_item_t *copy = item->table_next;
        item->table_next = item->table_next->table_next;
        item = copy;
    }

    lru_remove_node_from_list(item);

    buffer_clear(&item->key);
    buffer_clear(&item->value);

    memory_free(item);

    --_items_in_hash;

    return 0;
}

int
hashmap_find(struct buffer_t *key, struct buffer_t **value, uint64_t *cas_value)
{
    size_t table_index = hash(key, hash_table_size);
    struct hashmap_item_t *item;
    for (item = hash_table[table_index]; item; item = item->table_next)
    {
        if (buffer_compare(&item->key, key) == 0)
        {
            lru_remove_node_from_list(item);
            lru_hook_node_before_sentinel(item, lru_sentinel);
            *value = &item->value;
            *cas_value = item->cas;
            return 0;
        }
    }

    *value = NULL;
    *cas_value = 0;
    return 0;
}

int
hashmap_remove_lru()
{
    if (lru_sentinel->lru_next == lru_sentinel)
    {
        return -1;
    } else {
        return hashmap_remove(&lru_sentinel->lru_next->key);
    }
}
