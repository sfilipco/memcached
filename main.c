#include <assert.h>
#include <stdio.h>

#include "hashmap.h"
#include "memory.h"

#define DEBUG 0

const size_t MB = 1024 * 1024;

void
print_buffer_t(struct buffer_t *buffer)
{
    if (buffer == NULL || buffer->size == 0 || buffer->content == NULL)
    {
        printf("NULL\n");
    } else {
        printf("%.*s\n", (int) buffer->size, buffer->content);
    }
}

void
str_add(char *key, char *value)
{
    struct buffer_t _key, _value;
    buffer_from_string(&_key, key);
    buffer_from_string(&_value, value);

    hashmap_add(&_key, &_value);

    buffer_clear(&_key);
    buffer_clear(&_value);
}

uint64_t
str_check_and_set(char *key, char *value, uint64_t cas)
{
    struct buffer_t _key, _value;
    buffer_from_string(&_key, key);
    buffer_from_string(&_value, value);

    uint64_t _cas = hashmap_check_and_set(&_key, &_value, cas);

    buffer_clear(&_key);
    buffer_clear(&_value);

    return _cas;
}

void
str_find(char *key, struct buffer_t* *value, uint64_t *cas)
{
    struct buffer_t _key;
    buffer_from_string(&_key, key);
    hashmap_find(&_key, value, cas);
    buffer_clear(&_key);
}

int
str_remove(char *key)
{
    struct buffer_t _key;
    buffer_from_string(&_key, key);
    int response = hashmap_remove(&_key);
    buffer_clear(&_key);
    return response;
}

// TODO: check allocation failures in many places
// TODO: support resizing of hash hash_table
// NOTE: I wonder if we are effected by poor byte packing
// memory.c is probably the most important piece to optimize
// Buffer interface is probably second

int main(int argc, char **argv)
{
    set_memory_limit((size_t) 200 * MB);

    void *ptr;
    ptr = memory_allocate(100);
    memory_free(ptr);
    ptr = memory_allocate(240);
    memory_free(ptr);
    ptr = memory_allocate(64);
    memory_free(ptr);

    hashmap_init(1*MB); // 1MB * sizeof(*ptr) = 8MB on my machine

    printf("Hello, World!\n");

    const int M = 1000 * 1000;

    char buf[16];
    struct buffer_t key, *buffer_ptr;
    uint64_t cas;

    str_add("ana", "mere");
    str_add("ema", "pere");

    str_find("ana", &buffer_ptr, &cas); print_buffer_t(buffer_ptr);
    if (str_check_and_set("ana", "prune", 0) != -1)
    {
        printf("Check and set was supposed to error");
    }
    str_check_and_set("ana", "prune", cas);
    str_find("ana", &buffer_ptr, &cas); print_buffer_t(buffer_ptr);
    str_find("ema", &buffer_ptr, &cas); print_buffer_t(buffer_ptr);
    str_find("ioana", &buffer_ptr, &cas); print_buffer_t(buffer_ptr);

    str_remove("ana");
    str_find("ana", &buffer_ptr, &cas); print_buffer_t(buffer_ptr);

    for (int i = 1; i <= 2*M; ++i)
    {
        if (DEBUG && i % 1000 == 0) printf("with %d\n", i);
        if (i % 10000 == 0) printf("current memory usage = %zu\n", get_allocated_memory());
        sprintf(buf, "%d", i);

        buffer_from_string(&key, buf);
        hashmap_add(&key, &key);
        buffer_clear(&key);

        if (i > M) {
            sprintf(buf, "%d", i - M);

            buffer_from_string(&key, buf);
            hashmap_find(&key, &buffer_ptr, &cas);
            if (buffer_ptr != NULL && buffer_compare(buffer_ptr, &key) != 0) {
                printf("Difference for %d %s\n", i, buf);
            }
            if (buffer_ptr != NULL && hashmap_remove(&key) != 0) {
                printf("error while removing %d\n", i-M);
            }
            if (DEBUG) printf("find again %s\n", buf);
            hashmap_find(&key, &buffer_ptr, &cas);
            assert(buffer_ptr == NULL);
            buffer_clear(&key);
        }
    }

    return 0;
}
