#include <stdio.h>

#include "../src/hashmap.h"
#include "../src/util.h"

#include "minunit.h"

#include "memory_test.c"
#include "string_hashmap_test.c"

int tests_run = 0;

static char * all_tests() {
    mu_run_test_collection(string_hashmap_tests);
    mu_run_test_collection(memory_tests);
    return 0;
}

int main(int argc, char **argv)
{
    set_memory_limit(20 * MB);

    char *result = all_tests();
    if (result != NULL) {
        printf("%s\n", result);
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}
