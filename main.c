#include <assert.h>
#include <stdio.h>
#include <string.h>

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

struct buffer_t *
str2buf(char *str)
{
    struct buffer_t *response = malloc(sizeof(struct buffer_t));
    buffer_from_string(response, str);
    return response;
}

// TODO: check allocation failures in many places
// TODO: support resizing of hash hash_table
// NOTE: I wonder if we are effected by poor byte packing
// NOTE: I just had to go and add the buffer struct... this rough benchmark suggests that we lost 25% throughput :(
// Maybe there are more aggressive compiler flags that we can use.
int main(int argc, char **argv)
{
    set_memory_limit((size_t) 10 * MB);
    hashmap_init(1*MB); // 1MB * sizeof(*ptr)

    printf("Hello, World!\n");

    const int M = 1000 * 1000;

    char buf[16];
    struct buffer_t *key, *value;
    int64_t cas;

    key = malloc(sizeof(struct buffer_t *));

    // NOTE: Yes, yes, losing some memory
    printf("ana are mere\n");
    hashmap_add(str2buf("ana"), str2buf("mere"));
    printf("ana are mere\n");
    hashmap_add(str2buf("ema"), str2buf("pere"));

    hashmap_find(str2buf("ana"), &value, &cas);
    if (hashmap_check_and_set(str2buf("ana"), str2buf("prune"), 0) != -1)
    {
        printf("Check and set was supposed to error");
    }
    hashmap_check_and_set(str2buf("ana"), str2buf("prune"), cas);
    hashmap_find(str2buf("ana"), &value, &cas); print_buffer_t(value);
    hashmap_find(str2buf("ema"), &value, &cas); print_buffer_t(value);
    hashmap_find(str2buf("ioana"), &value, &cas); print_buffer_t(value);

    hashmap_remove(str2buf("ana"));
    hashmap_find(str2buf("ana"), &value, &cas); print_buffer_t(value);

    for (int i = 1; i <= 2*M; ++i)
    {
        if (DEBUG && i % 1000 == 0) printf("with %d\n", i);
        sprintf(buf, "%d", i);
        buffer_from_string(key, buf);

        hashmap_add(key, key);
        if (i > M) {
            sprintf(buf, "%d", i - M);
            buffer_from_string(key, buf);
            hashmap_find(key, &value, &cas);
            if (DEBUG) printf("found item %s\n", buf);
            if (value != NULL && buffer_compare(value, key) != 0) {
                printf("Difference for %d %s\n", i, buf);
            }
            if (DEBUG) printf("removing %s\n", buf);
            if (hashmap_remove(key) != 0) {
                printf("Error removing %d", i-M);
            }
            if (DEBUG) printf("find again %s\n", buf);
            hashmap_find(key, &value, &cas);
            assert(value == NULL);
        }
    }

    return 0;
}
