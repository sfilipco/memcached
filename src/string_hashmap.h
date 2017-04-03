//
// Created by Stefan Filip on 4/1/17.
//

#ifndef MEMCACHED_STRING_HASHMAP_H
#define MEMCACHED_STRING_HASHMAP_H

#include <stdint.h>

#include "hashmap.h"

int
string_hashmap_add(char *key, char *value);

int
string_hashmap_check_and_set(char *key, char *value, uint64_t cas);

int
string_hashmap_remove(char *key);

int
string_hashmap_find(char *key, char **value, uint64_t *cas);

#endif //MEMCACHED_STRING_HASHMAP_H
