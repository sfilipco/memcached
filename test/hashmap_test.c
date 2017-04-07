#include <string.h>

#include "../src/hashmap.h"
#include "../src/memory.h"

#include "minunit.h"

#define test_and_clear(test) do { hashmap_init(1024); char *response = test(); hashmap_clear(); \
                                  if (response != NULL) return response; } while (0)

static char *
test_add_find()
{
    uint8_t key[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t value[] = {0x05, 0x06, 0x07, 0x08, 0x09};
    uint16_t ksize = 4;
    uint32_t vsize = 5;
    struct hashmap_item *item;

    mu_assert("add_find: miss key", hashmap_find(key, ksize, &item) == NOT_FOUND);
    mu_assert("add_find: insert key->value", hashmap_insert(key, ksize, value, vsize, 0, &item) == SUCCESS);
    mu_assert("add_find: get key", hashmap_find(key, ksize, &item) == SUCCESS);
    mu_assert("add_find: key was found in hash", item != NULL);
    mu_assert("add_find: value was correctly stored", memcmp(item->value, value, vsize) == 0);
    return 0;
}

static char *
test_remove()
{
    uint8_t key[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t value[] = {0x05, 0x06, 0x07, 0x08, 0x09};
    uint16_t ksize = 4;
    uint32_t vsize = 5;
    struct hashmap_item *item;

    mu_assert("remove: miss key", hashmap_remove(key, ksize) == NOT_FOUND);
    mu_assert("remove: setup key", hashmap_insert(key, ksize, value, vsize, 0, &item) == SUCCESS);
    mu_assert("remove: hit key", hashmap_find(key, ksize, &item) == SUCCESS);
    mu_assert("remove: remove key", hashmap_remove(key, ksize) == SUCCESS);
    mu_assert("remove: miss key again", hashmap_find(key, ksize, &item) == NOT_FOUND);
    mu_assert("remove: item pointer set to empty", item == NULL);
    return 0;
}


static char *
test_cas()
{
    uint8_t key[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t not_found_key[] = {0xFF, 0xFF};
    uint8_t value1[] = {0x05, 0x06, 0x07, 0x08, 0x09};
    uint8_t value2[] = {0x0A, 0x0B, 0x0C};
    uint8_t value3[] = {0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13};
    uint16_t ksize = 4, not_found_ksize = 2;
    uint32_t vsize1 = 5, vsize2 = 3, vsize3 = 6;
    struct hashmap_item *item;
    uint64_t value1_cas, value2_cas;

    mu_assert("cas: add key", hashmap_insert(key, ksize, value1, vsize1, 0, &item) == SUCCESS);
    value1_cas = item->cas;
    mu_assert("cas: check and set", hashmap_insert(key, ksize, value2, vsize2, item->cas, &item) == SUCCESS);
    value2_cas = item->cas;
    mu_assert("cas: get key", hashmap_find(key, ksize, &item) == SUCCESS);
    mu_assert("cas: check value size", item->value_size == vsize2);
    mu_assert("cas: check value", memcmp(item->value, value2, vsize2) == 0);
    mu_assert("cas: check and set miss",
              hashmap_insert(key, ksize, value3, vsize3, value1_cas, &item) == ALREADY_UPDATED);
    mu_assert("cas: check and set no value",
              hashmap_insert(not_found_key, not_found_ksize, value3, vsize3, value2_cas, &item) == NOT_FOUND);
    mu_assert("cas: check and set again", hashmap_insert(key, ksize, value3, vsize3, value2_cas, &item) == SUCCESS);
    return 0;
}

static void
int2buf(uint8_t *buf, uint16_t *bsize, int v)
{
    for (*bsize = 0; v; ++(*bsize), v >>= 8) {
        buf[*bsize] = v & 0xFF;
    }
}

static char *
test_integration()
{
    size_t memory_limit = get_memory_limit();
    set_memory_limit(200*MB);

    hashmap_init(10*MB);

    uint8_t buf[4];
    uint16_t bsize;
    struct hashmap_item *item;

    // With how memory is set up we keep a bit over 1M entries in the hash
    for (size_t i = 1; i <= 4 * MILLION; ++i)
    {
        // if (i % 10000 == 0) printf("current memory usage = %zu\n", get_allocated_memory());
        int2buf(buf, &bsize, i);
        mu_assert("integration: add", hashmap_insert(buf, bsize, buf, bsize, 0, &item) == SUCCESS);

        if (i > 2 * MILLION) {
            int2buf(buf, &bsize, i - MILLION);

            mu_assert("integration: find", hashmap_find(buf, bsize, &item) == SUCCESS);
            mu_assert("integration: value_size", item->value_size == bsize);
            mu_assert("integration: value", memcmp(item->value, buf, bsize) == 0);
            mu_assert("integration: remove", hashmap_remove(buf, bsize) == SUCCESS);
            mu_assert("integration: miss", hashmap_find(buf, bsize, &item) == NOT_FOUND);
        }
    }

    hashmap_clear();
    set_memory_limit(memory_limit);

    return 0;
}

char *
hashmap_tests()
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