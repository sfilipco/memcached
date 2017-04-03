#include <memory.h>

#include "hashmap.h"
#include "memory.h"

// We preallocate a hash_table as large as we can from the start
// This has poor memory usage when we have a few large key, values pairs
// If we have small key, value pairs than we have run into more collisions but since we
// are using a linked list for collisions we handle that scenario ok
// TODO: support resizing

struct hashmap_item
{
    struct hashmap_item *table_next, *lru_next, *lru_prev;
    uint64_t cas;
    // TODO: uint32_t flags;
    uint8_t *key, *value;
    size_t key_size, value_size;
};

static struct hashmap_item **hash_table, *lru_sentinel;
static uint64_t hash_table_size, cas_index, _items_in_hash;

int
hashmap_init(uint64_t _table_size)
{
    hash_table_size = _table_size;
    hash_table = memory_allocate(hash_table_size * sizeof(struct hashmap_item *));
    memset(hash_table, 0x00, hash_table_size * sizeof(struct hashmap_item *));

    cas_index = 0;
    _items_in_hash = 0;

    lru_sentinel = memory_allocate(sizeof(struct hashmap_item));
    lru_sentinel->key = lru_sentinel->value = NULL;
    lru_sentinel->key_size = lru_sentinel->value_size = 0;
    lru_sentinel->cas = 0;
    lru_sentinel->table_next = 0;
    lru_sentinel->lru_prev = lru_sentinel->lru_next = lru_sentinel;

    return SUCCESS;
}

int
hashmap_clear()
{
    size_t i;
    struct hashmap_item *it, *ptr;
    for (i = 0; i < hash_table_size; ++i) {
        it = hash_table[i];
        while (it != NULL) {
            ptr = it;
            it = it->table_next;
            memory_free(ptr->key);
            memory_free(ptr->value);
            memory_free(ptr);
            --_items_in_hash;
        }
    }
    memory_free(hash_table);
    memory_free(lru_sentinel);

    return SUCCESS;
}

static size_t
hash(uint8_t *buffer, size_t buffer_size, size_t table_size)
{
    // taken from http://stackoverflow.com/questions/7666509/hash-function-for-string
    // we could do better http://burtleburtle.net/bob/c/SpookyV2.h
    size_t response = 5381;
    for (size_t i = 0; i < buffer_size; ++i) {
        response = ((response << 5) + response) + buffer[i];
    }
    return response & (table_size-1);
}

static void
lru_hook_node_before_sentinel(struct hashmap_item *node)
{
    node->lru_next = lru_sentinel;
    node->lru_prev = lru_sentinel->lru_prev;
    lru_sentinel->lru_prev->lru_next = node;
    lru_sentinel->lru_prev = node;
}

static void
lru_remove_node_from_list(struct hashmap_item *node)
{
    node->lru_prev->lru_next = node->lru_next;
    node->lru_next->lru_prev = node->lru_prev;
}

int
hashmap_add(uint8_t *key, size_t key_size, uint8_t *value, size_t value_size)
{
    // TODO: Check that key does not exist
    struct hashmap_item *item = memory_allocate(sizeof(struct hashmap_item));
    if (item == NULL) goto error_item;

    item->key_size = key_size;
    item->key = memory_allocate(key_size);
    if (item->key == NULL) goto error_key;
    memcpy(item->key, key, key_size);

    item->value_size = value_size;
    item->value = memory_allocate(value_size);
    if (item->value == NULL) goto error_value;
    memcpy(item->value, value, value_size);

    item->cas = ++cas_index;

    size_t table_index = hash(key, key_size, hash_table_size);
    struct hashmap_item *next = hash_table[table_index];
    hash_table[table_index] = item;
    item->table_next = next;

    lru_hook_node_before_sentinel(item);

    ++_items_in_hash;

    return SUCCESS;

error_value:
    memory_free(item->key);
error_key:
    memory_free(item);
error_item:
    return INTERNAL_ERROR;
}

int
hashmap_check_and_set(uint8_t *key, size_t key_size, uint8_t *value, size_t value_size, uint64_t cas)
{
    size_t table_index = hash(key, key_size, hash_table_size);
    struct hashmap_item *item;
    for (item = hash_table[table_index]; item; item = item->table_next) {
        if (item->key_size == key_size && memcmp(item->key, key, key_size) == 0) {
            if (item->cas == cas) {
                uint8_t *ptr = memory_reallocate(item->value, value_size);
                if (!ptr) return INTERNAL_ERROR;
                item->value = ptr;
                memcpy(item->value, value, value_size);
                item->value_size = value_size;
                lru_remove_node_from_list(item);
                lru_hook_node_before_sentinel(item);
                item->cas = ++cas_index;
                return SUCCESS;
            } else {
                return ALREADY_UPDATED;
            }
        }
    }
    return NOT_FOUND;
}


int
hashmap_remove(uint8_t *key, size_t key_size)
{
    size_t table_index = hash(key, key_size, hash_table_size);
    struct hashmap_item *item = hash_table[table_index];
    if (item == NULL) return NOT_FOUND;
    if (item->key_size == key_size && memcmp(item->key, key, key_size) == 0) {
        hash_table[table_index] = item->table_next;
    } else {
        while (item->table_next != NULL && (item->table_next->key_size != key_size || memcmp(item->table_next->key, key, key_size) != 0)) {
            item = item->table_next;
        }
        if (item->table_next == NULL) return NOT_FOUND;
        struct hashmap_item *copy = item->table_next;
        item->table_next = item->table_next->table_next;
        item = copy;
    }

    lru_remove_node_from_list(item);
    memory_free(item->key);
    memory_free(item->value);
    memory_free(item);

    --_items_in_hash;

    return SUCCESS;
}

int
hashmap_find(uint8_t *key, size_t key_size, uint8_t **value, size_t *value_size, uint64_t *cas)
{
    size_t table_index = hash(key, key_size, hash_table_size);
    struct hashmap_item *item;
    for (item = hash_table[table_index]; item; item = item->table_next) {
        if (item->key_size == key_size && memcmp(item->key, key, key_size) == 0) {
            lru_remove_node_from_list(item);
            lru_hook_node_before_sentinel(item);
            *value = item->value;
            *value_size = item->value_size;
            if (cas != NULL) *cas = item->cas;
            return SUCCESS;
        }
    }

    *value = NULL;
    *value_size = 0;
    return NOT_FOUND;
}

int
hashmap_remove_lru()
{
    if (lru_sentinel->lru_next == lru_sentinel) return INTERNAL_ERROR;
    struct hashmap_item *ptr = lru_sentinel->lru_next;
    return hashmap_remove(ptr->key, ptr->key_size);
}
