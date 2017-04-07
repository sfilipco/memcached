#include <netinet/in.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <inttypes.h>
#include <unistd.h>

#include "hashmap.h"
#include "memory.h"
#include "util.h"

#define HEADER_SIZE 24
#define UNKNOWN_COMMAND 0x81
#define OUT_OF_MEMORY 0x82

struct header {
    uint8_t op_code, extras_size;
    uint16_t key_size, status;
    uint32_t body_size, opaque;
    uint64_t cas;
};

int
parse_header(struct header *header, uint8_t *buffer)
{
    /*
     Header:

     Byte/     0       |       1       |       2       |       3       |
        /              |               |               |               |
       |0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|
       +---------------+---------------+---------------+---------------+
      0| Magic         | Opcode        | Key length                    |
       +---------------+---------------+---------------+---------------+
      4| Extras length | Data type     | Reserved/Status               |
       +---------------+---------------+---------------+---------------+
      8| Total body length                                             |
       +---------------+---------------+---------------+---------------+
     12| Opaque                                                        |
       +---------------+---------------+---------------+---------------+
     16| CAS                                                           |
       |                                                               |
       +---------------+---------------+---------------+---------------+
       Total 24 bytes
     */

    if (buffer[0] != 0x80) return -1;
    if (buffer[5] != 0x00) return -1;

    header->op_code = buffer[1];
    header->extras_size = buffer[4];
    header->status = 0;

    get_network_uint16(&header->key_size, buffer + 2);
    get_network_uint32(&header->body_size, buffer + 8);
    get_network_uint32(&header->opaque, buffer + 12);
    get_network_uint64(&header->cas, buffer + 16);

    return 0;
}

void
serialize_header(uint8_t *buffer, struct header *header)
{
    buffer[0] = 0x81;
    buffer[5] = 0x00;

    buffer[1] = header->op_code;
    buffer[4] = header->extras_size;
    write_network_uint16(buffer + 2, header->key_size);
    write_network_uint16(buffer + 6, header->status);
    write_network_uint32(buffer + 8, header->body_size);
    write_network_uint32(buffer + 12, header->opaque);
    write_network_uint64(buffer + 16, header->cas);
}

ssize_t
command_ready(struct evbuffer *input, struct header *header)
{
    if (evbuffer_get_length(input) < HEADER_SIZE) return 0;
    uint8_t *buffer;
    buffer = evbuffer_pullup(input, HEADER_SIZE);
    if (parse_header(header, buffer) < 0) {
        return -1;
    }

    size_t command_size = HEADER_SIZE + header->body_size;
    if (command_size > MB) return -1;
    if (evbuffer_get_length(input) < command_size) return 0;
    return command_size;
}

struct header
init_response_header(struct header *request_header)
{
    struct header response_header;
    memset(&response_header, 0x00, sizeof(struct header));
    response_header.op_code = request_header->op_code;
    response_header.opaque = request_header->opaque;
    return response_header;
}

int
handle_request(uint8_t *request, struct header *request_header, struct evbuffer *output)
{
    struct header response_header = init_response_header(request_header);
    uint8_t *response = NULL;
    uint32_t flags, expiration;
    struct hashmap_item *item;

    int result;
    switch (request_header->op_code) {
        case 0x00: // get
            if (request_header->extras_size > 0) goto bad_request;
            result = hashmap_find(request + 24, request_header->key_size, &item);
            if (result == SUCCESS) {
                response_header.cas = item->cas;
                response_header.extras_size = 4;
                response_header.body_size = 4 + item->value_size;
                response = memory_allocate(HEADER_SIZE + response_header.body_size);
                if (response == NULL) goto internal_error;
                serialize_header(response, &response_header);
                memcpy(response + HEADER_SIZE + 4, item->value, item->value_size);
                evbuffer_add(output, response, HEADER_SIZE + response_header.body_size);
                memory_free(response);
                break;
            }
            if (result == NOT_FOUND) {
                response_header.status = NOT_FOUND;
                response = memory_allocate(HEADER_SIZE + response_header.body_size);
                if (response == NULL) goto internal_error;
                serialize_header(response, &response_header);
                memcpy(response + HEADER_SIZE + 4, item->value, item->value_size);
                evbuffer_add(output, response, HEADER_SIZE + response_header.body_size);
                memory_free(response);
                break;
            }
            goto internal_error;
        case 0x01: // set
            get_network_uint32(&flags, request + HEADER_SIZE);
            get_network_uint32(&expiration, request + HEADER_SIZE + 4);
            size_t key_start = HEADER_SIZE + request_header->extras_size;
            size_t value_start = key_start + request_header->key_size;
            size_t value_size = request_header->body_size + HEADER_SIZE - value_start;
            // TODO check value_size is smaller than max uint32_t
            result = hashmap_insert(request + key_start, request_header->key_size,
                                    request + value_start, (uint32_t) value_size,
                                    request_header->cas, &item);
            if (result == INTERNAL_ERROR) goto internal_error;
            response_header.status = (uint16_t) result;
            response_header.cas = item->cas;
            response = memory_allocate(HEADER_SIZE);
            if (response == NULL) goto internal_error;
            serialize_header(response, &response_header);
            evbuffer_add(output, response, HEADER_SIZE);
            memory_free(response);
            break;
//        case 0x04: // delete
//            break;
//        case 0x07: // quit
//            break;
        default:
            fprintf(stderr, "operation not supported %" PRIx8 "\n", request_header->op_code);
            response_header.status = UNKNOWN_COMMAND;
            response = memory_allocate(HEADER_SIZE);
            if (response == NULL) goto internal_error;
            serialize_header(response, &response_header);
            evbuffer_add(output, response, HEADER_SIZE);
            memory_free(response);
            break;
    }

    return 0;
bad_request:
    return 1;
internal_error:
    return 2;
}

void
readcb(struct bufferevent *bev, void *ctx)
{
    struct evbuffer *input, *output;
    struct header request_header;
    uint8_t *request;
    input = bufferevent_get_input(bev);
    output = bufferevent_get_output(bev);
    ssize_t total_length, bytes_read;

    while ((total_length = command_ready(input, &request_header)) > 0) {
        request = memory_allocate((size_t) total_length);
        bytes_read = evbuffer_remove(input, request, (size_t) total_length);
        if (bytes_read < total_length) {
            memory_free(request);
            perror("Read fewer bytes than we wanted");
            goto close_connection;
        }
        handle_request(request, &request_header, output);
    }
    if (total_length == -1) {
        perror("Invalid command received");
        goto close_connection;
    }
    return;

close_connection:
    {
        evutil_socket_t fd = bufferevent_getfd(bev);
        evutil_closesocket(fd);
    }
    return;
}

void
errorcb(struct bufferevent *bev, short error, void *ctx)
{
    if (error & BEV_EVENT_EOF) {
        /* connection has been closed, do any clean up here */
        /* ... */
    } else if (error & BEV_EVENT_ERROR) {
        /* check errno to see what error occurred */
        /* ... */
    } else if (error & BEV_EVENT_TIMEOUT) {
        /* must be a timeout event handle, handle it */
        /* ... */
    }
    bufferevent_free(bev);
}

void
do_accept(evutil_socket_t listener, short event, void *arg)
{
    struct event_base *base = arg;
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listener, (struct sockaddr*)&ss, &slen);
    if (fd < 0) {
        perror("accept");
    } else if (fd > FD_SETSIZE) {
        close(fd);
    } else {
        struct bufferevent *bev;
        evutil_make_socket_nonblocking(fd);
        bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(bev, readcb, NULL, errorcb, NULL);
        bufferevent_setwatermark(bev, EV_READ, 0, MB);
        bufferevent_enable(bev, EV_READ|EV_WRITE);
    }
}

int main(int argc, char **argv)
{
    set_memory_limit((size_t) 200 * MB);
    hashmap_init(1*MB);

    // We may not always want to count the IO buffers in our memory consumption but it seems fine to so by default
    event_set_mem_functions(memory_allocate, memory_reallocate, memory_free);

    evutil_socket_t listener;
    struct sockaddr_in sin;
    struct event_base *base;
    struct event *listener_event;

    base = event_base_new();
    if (!base) {
        perror("Did not init base");
        return -1;
    }

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(11311);

    listener = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(listener);

    int one = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(listener, 16)<0) {
        perror("listen");
        return -1;
    }

    listener_event = event_new(base, listener, EV_READ|EV_PERSIST, do_accept, (void*)base);
    /*XXX check it */
    event_add(listener_event, NULL);

    event_base_dispatch(base);


    return 0;
}
