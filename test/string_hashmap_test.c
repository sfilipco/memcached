#include <string.h>

#include "../src/string_hashmap.h"
#include "../src/memory.h"

#include "minunit.h"

#define test_and_clear(test) do { string_hashmap_init(1024); char *response = test(); string_hashmap_clear(); \
                                  if (response != NULL) return response; } while (0)

static char *
test_add_find()
{
    char *value;
    mu_assert("add_find: miss my_key", string_hashmap_find("my_key", &value, NULL) == NOT_FOUND);
    mu_assert("add_find: my_key->my_value", string_hashmap_add("my_key", "my_value") == SUCCESS);
    mu_assert("add_find: get my_key", string_hashmap_find("my_key", &value, NULL) == SUCCESS);
    mu_assert("add_find: my_key was found in hash", value != NULL);
    memory_free(value);
    mu_assert("add_find: my_value was correctly stored", strcmp(value, "my_value") == 0);
    return 0;
}

static char *
test_remove()
{
    mu_assert("remove: miss my_key", string_hashmap_remove("my_key") == NOT_FOUND);
    mu_assert("remove: setup my_key", string_hashmap_add("my_key", "my_value") == SUCCESS);
    char *value;
    mu_assert("remove: delete my_key", string_hashmap_remove("my_key") == SUCCESS);
    mu_assert("remove: find my_key", string_hashmap_find("my_key", &value, NULL) == NOT_FOUND);
    mu_assert("remove: my_key was not found in hash", value == NULL);
    return 0;
}

static char *
test_cas()
{
    uint64_t cas;
    char *value;
    mu_assert("cas: add my_key", string_hashmap_add("my_key", "my_value") == SUCCESS);
    mu_assert("cas: get my_key", string_hashmap_find("my_key", &value, &cas) == SUCCESS);
    memory_free(value);
    mu_assert("cas: check and set", string_hashmap_check_and_set("my_key", "my_other_value", cas) == SUCCESS);
    mu_assert("cas: get my_key", string_hashmap_find("my_key", &value, NULL) == SUCCESS);
    mu_assert("cas: check value", strcmp(value, "my_other_key"));
    memory_free(value);
    mu_assert("cas: check and set", string_hashmap_check_and_set("my_key", "my_third_value", cas) == ALREADY_UPDATED);
    mu_assert("cas: check and set", string_hashmap_check_and_set("not_found_key", "my_third_value", cas) == NOT_FOUND);
    return 0;
}

static char *
test_integration()
{
    size_t memory_limit = get_memory_limit();
    set_memory_limit(200*MB);

    hashmap_init(10*MB);

    char buf[16], *value;

    // With how memory is set up we keep a bit over 1M entries in the hash
    for (size_t i = 1; i <= 4*Million; ++i)
    {
        // if (i % 10000 == 0) printf("current memory usage = %zu\n", get_allocated_memory());
        sprintf(buf, "%zu", i);

        mu_assert("integration: add", string_hashmap_add(buf, buf) == SUCCESS);

        if (i > 2*Million) {
            sprintf(buf, "%zu", i - Million);

            mu_assert("integration: find", string_hashmap_find(buf, &value, NULL) == SUCCESS);
            memory_free(value);
            mu_assert("integration: value", strcmp(buf, value) == 0);
            mu_assert("integration: remove", string_hashmap_remove(buf) == SUCCESS);
            mu_assert("integration: miss", string_hashmap_find(buf, &value, NULL) == NOT_FOUND);
        }
    }

    hashmap_clear();
    set_memory_limit(memory_limit);

    return 0;
}

char *
string_hashmap_tests()
{
    size_t size = get_allocated_memory();
    hash_test(test_add_find);
    mu_assert("hash memory leak check 1", get_allocated_memory() == size);
    hash_test(test_remove);
    mu_assert("hash memory leak check 2", get_allocated_memory() == size);
    hash_test(test_cas);
    mu_assert("hash memory leak check 3", get_allocated_memory() == size);
    mu_run_test(test_integration);
    mu_assert("hash memory leak check 4", get_allocated_memory() == size);
    return 0;
}
