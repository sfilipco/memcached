#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "hashmap.h"

#define DEBUG 0

const size_t MB = 1024 * 1024;

void
print_hashmap_item(struct hashmap_item_t *item)
{
    if (item != NULL)
    {
        printf("%.*s\n", item->value_size, item->value);
    } else {
        printf("NULL\n");
    }
}

int main()
{
    // TODO: check allocation failures in many places
    // TODO(stefan): support resizing of hash table
    printf("Hello, World!\n");

    struct hashmap_t *hashmap = hashmap_init(10 * MB);

    hashmap_add(hashmap, "ana", 3, "mere", 4);
    hashmap_add(hashmap, "ema", 3, "pere", 4);

    print_hashmap_item(hashmap_find(hashmap, "ana", 3));
    print_hashmap_item(hashmap_find(hashmap, "ema", 3));
    print_hashmap_item(hashmap_find(hashmap, "ioana", 5));

    hashmap_remove(hashmap, "ana", 3);
    print_hashmap_item(hashmap_find(hashmap, "ana", 3));

    const int M = 1000 * 1000;
    char buf[16];
    size_t len;
    struct hashmap_item_t *item;
    for (int i = 1; i <= 10*M; ++i)
    {
        if (DEBUG && i % 1000 == 0) printf("with %d\n", i);
        sprintf(buf, "%d", i);
        len = strlen(buf);
        hashmap_add(hashmap, buf, len, buf, len);
        if (i > M) {
            sprintf(buf, "%d", i - M);
            len = strlen(buf);
            item = hashmap_find(hashmap, buf, len);
            if (DEBUG) printf("found item %s\n", buf);
            if (item != NULL && memcmp(item->value, buf, len) != 0) {
                printf("Difference for %d %s %zu\n", i, buf, len);
            }
            if (DEBUG) printf("removing %s\n", buf);
            if (hashmap_remove(hashmap, buf, len) != 0) {
                printf("Error removing %d", i-M);
            }
            if (DEBUG) printf("find again %s\n", buf);
            item = hashmap_find(hashmap, buf, len);
            assert(item == NULL);
        }
    }

    return 0;
}
