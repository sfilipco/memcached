#ifndef MEMCACHED_UTIL_H
#define MEMCACHED_UTIL_H

#define KB 1024
#define MB 1048576
#define MILLION 1000000


void
get_network_uint16(uint16_t *dest, uint8_t *src);

void
get_network_uint32(uint32_t *dest, uint8_t *src);

void
get_network_uint64(uint64_t *dest, uint8_t *src);

void
write_network_uint16(uint8_t *dest, uint16_t src);

void
write_network_uint32(uint8_t *dest, uint32_t src);

void
write_network_uint64(uint8_t *dest, uint64_t src);

#endif //MEMCACHED_UTIL_H
