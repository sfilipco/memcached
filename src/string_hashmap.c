#include <stdint.h>
#include <string.h>

#include "hashmap.h"
#include "memory.h"

int
string_hashmap_add(char *key, char *value)
{
    return hashmap_add((uint8_t*) key, strlen(key), (uint8_t*) value, strlen(value));
}

int
string_hashmap_check_and_set(char *key, char *value, uint64_t cas)
{
    return hashmap_check_and_set((uint8_t*) key, strlen(key), (uint8_t*) value, strlen(value), cas);
}

int
string_hashmap_remove(char *key)
{
    return hashmap_remove((uint8_t*) key, strlen(key));
}

int
string_hashmap_find(char *key, char **value, uint64_t *cas)
{
    uint8_t *ptr;
    size_t ptr_size;
    int response = hashmap_find((uint8_t*) key, strlen(key), &ptr, &ptr_size, cas);
    if (response < 0) return response;
    if (response == NOT_FOUND) {
        *value = NULL;
        return NOT_FOUND;
    }

    *value = memory_allocate(ptr_size+1);
    if (*value == NULL) return -1;

    memcpy(*value, ptr, ptr_size);
    (*value)[ptr_size] = '\0';
    return 0;
}
