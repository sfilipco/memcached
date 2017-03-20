#include <stdio.h>
#include "hashmap.h"

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
    printf("Hello, World!\n");

    struct hashmap_t *hashmap = hashmap_init(10 * MB);

    hashmap_add(hashmap, "ana", 3, "mere", 4);
    hashmap_add(hashmap, "ema", 3, "pere", 4);

    print_hashmap_item(hashmap_find(hashmap, "ana", 3));
    print_hashmap_item(hashmap_find(hashmap, "ema", 3));
    print_hashmap_item(hashmap_find(hashmap, "ioana", 5));

    return 0;
}
