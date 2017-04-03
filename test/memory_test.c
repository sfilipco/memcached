#include "../src/memory.h"

#include "minunit.h"

const size_t overhead = sizeof(size_t);

// THIS TEST ASSUMES WE DON'T HAVE CONCURRENCY
static char *
test_allocate()
{
    size_t size = 16, initial = get_allocated_memory();
    void *ptr = memory_allocate(size);
    mu_assert("memory allocated", ptr != NULL);
    mu_assert("allocate correct increase", get_allocated_memory() == initial + size + overhead);
    memory_free(ptr);
    mu_assert("allocate back to initial size", get_allocated_memory() == initial);
    return 0;
}

static char *
test_reallocate()
{
    size_t size = 16, resize = 48, initial = get_allocated_memory();
    void *ptr = memory_allocate(size);
    ptr = memory_reallocate(ptr, resize);
    mu_assert("memory reallocated", ptr != NULL);
    mu_assert("reallocate correct increase", get_allocated_memory() == initial + resize + overhead);
    memory_free(ptr);
    mu_assert("reallocate back to initial size", get_allocated_memory() == initial);
    return 0;
}

char *
memory_tests()
{
    mu_run_test(test_allocate);
    mu_run_test(test_reallocate);
    return 0;
}
