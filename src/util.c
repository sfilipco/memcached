#include <inttypes.h>
#include <memory.h>
#include <netinet/in.h>

#include "util.h"

void
get_network_uint16(uint16_t *dest, uint8_t *src)
{
    memcpy(dest, src, 2);
    *dest = ntohs(*dest);
}

void
get_network_uint32(uint32_t *dest, uint8_t *src)
{
    memcpy(dest, src, 4);
    *dest = ntohl(*dest);
}

void
get_network_uint64(uint64_t *dest, uint8_t *src)
{
    memcpy(dest, src, 8);
    *dest = ntohll(*dest);
}


void
write_network_uint16(uint8_t *dest, uint16_t src)
{
    uint16_t nbytes = htons(src);
    memcpy(dest, &nbytes, 2);
}

void
write_network_uint32(uint8_t *dest, uint32_t src)
{
    uint32_t nbytes = htonl(src);
    memcpy(dest, &nbytes, 4);
}

void
write_network_uint64(uint8_t *dest, uint64_t src)
{
    uint64_t nbytes = htonll(src);
    memcpy(dest, &nbytes, 8);
}