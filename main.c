#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "hashmap.h"

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
print_hashmap_item_t(struct hashmap_item_t *item)
{
    if (item == NULL)
    {
        printf("NULL\n");
    } else {
        print_buffer_t(&item->value);
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
// TODO: support resizing of hash table
// NOTE: I wonder if we are effected by poor byte packing
// NOTE: I just had to go and add the buffer struct... this rough benchmark suggests that we lost 25% throughput :(
// Maybe there are more aggressive compiler flags that we can use.
int main(int argc, char **argv)
{
    printf("Hello, World!\n");
    const int M = 1000 * 1000;
    char buf[16];
    struct buffer_t buffer;
    size_t len;
    struct hashmap_item_t *item;
    int64_t cas;

    struct hashmap_t *hashmap = hashmap_init(10 * MB);

    // NOTE: Yes, yes, losing some memory
    hashmap_add(hashmap, str2buf("ana"), str2buf("mere"));
    hashmap_add(hashmap, str2buf("ema"), str2buf("pere"));

    item = hashmap_find(hashmap, str2buf("ana"));
    if (hashmap_check_and_set(hashmap, str2buf("ana"), 0, str2buf("prune")) != -1)
    {
        printf("Check and set was supposed to error");
    }
    hashmap_check_and_set(hashmap, str2buf("ana"), item->cas, str2buf("prune"));
    print_hashmap_item_t(hashmap_find(hashmap, str2buf("ana")));
    print_hashmap_item_t(hashmap_find(hashmap, str2buf("ema")));
    print_hashmap_item_t(hashmap_find(hashmap, str2buf("ioana")));

    hashmap_remove(hashmap, str2buf("ana"));
    print_hashmap_item_t(hashmap_find(hashmap, str2buf("ana")));

    for (int i = 1; i <= 10*M; ++i)
    {
        if (DEBUG && i % 1000 == 0) printf("with %d\n", i);
        sprintf(buf, "%d", i);
        buffer_from_string(&buffer, buf);

        hashmap_add(hashmap, &buffer, &buffer);
        if (i > M) {
            sprintf(buf, "%d", i - M);
            buffer_from_string(&buffer, buf);
            item = hashmap_find(hashmap, &buffer);
            if (DEBUG) printf("found item %s\n", buf);
            if (item != NULL && buffer_compare(&item->value, &buffer) != 0) {
                printf("Difference for %d %s\n", i, buf);
            }
            if (DEBUG) printf("removing %s\n", buf);
            if (hashmap_remove(hashmap, &buffer) != 0) {
                printf("Error removing %d", i-M);
            }
            if (DEBUG) printf("find again %s\n", buf);
            item = hashmap_find(hashmap, &buffer);
            assert(item == NULL);
        }
    }

    return 0;
}
