#ifndef MEMCACHED_MINUNIT_H
#define MEMCACHED_MINUNIT_H

#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++; \
                                if (message) return message; } while (0)

#define mu_run_test_collection(tests) do { char *message = tests(); \
                                            if (message) return message; } while (0)
#define hash_test(test) do { hashmap_init(1024); char *message = test(); hashmap_clear(); tests_run++; \
                                if (message) return message; } while (0)

extern int tests_run;

#endif //MEMCACHED_MINUNIT_H
