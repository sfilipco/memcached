// TODO: handle values with CRLF in them
// TODO: Check all buffer add responses
// TODO: check allocation failures in many places
// TODO: support resizing of hash hash_table
// NOTE: I wonder if we are effected by poor byte packing
// memory.c is probably the most important piece to optimize
// Buffer interface is probably second

/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "hashmap.h"
#include "memory.h"

#define DEBUG 0
#define MEGABYTE 1048576

void
print_buffer_t(struct buffer_t *buffer)
{
    if (buffer == NULL || buffer->size == 0 || buffer->content == NULL)
    {
        printf("NULL\n");
    } else {
        printf("%.*s\n", (int) buffer->size, buffer->content);
    }
}

void
str_add(char *key, char *value)
{
    struct buffer_t _key, _value;
    buffer_from_string(&_key, key);
    buffer_from_string(&_value, value);

    hashmap_add(&_key, &_value);

    buffer_clear(&_key);
    buffer_clear(&_value);
}

uint64_t
str_check_and_set(char *key, char *value, uint64_t cas)
{
    struct buffer_t _key, _value;
    buffer_from_string(&_key, key);
    buffer_from_string(&_value, value);

    uint64_t _cas = hashmap_check_and_set(&_key, &_value, cas);

    buffer_clear(&_key);
    buffer_clear(&_value);

    return _cas;
}

void
str_find(char *key, struct buffer_t* *value, uint64_t *cas)
{
    struct buffer_t _key;
    buffer_from_string(&_key, key);
    hashmap_find(&_key, value, cas);
    buffer_clear(&_key);
}

int
str_remove(char *key)
{
    struct buffer_t _key;
    buffer_from_string(&_key, key);
    int response = hashmap_remove(&_key);
    buffer_clear(&_key);
    return response;
}

void hash_test()
{
    void *ptr;
    ptr = memory_allocate(100);
    memory_free(ptr);
    ptr = memory_allocate(240);
    memory_free(ptr);
    ptr = memory_allocate(64);
    memory_free(ptr);

    hashmap_init(1*MEGABYTE); // 1MB * sizeof(*ptr) = 8MB on my machine

    printf("Hello, World!\n");

    const int M = 1000 * 1000;

    char buf[16];
    struct buffer_t key, *buffer_ptr;
    uint64_t cas;

    str_add("ana", "mere");
    str_add("ema", "pere");

    str_find("ana", &buffer_ptr, &cas); print_buffer_t(buffer_ptr);
    if (str_check_and_set("ana", "prune", 0) != -1)
    {
        printf("Check and set was supposed to error");
    }
    str_check_and_set("ana", "prune", cas);
    str_find("ana", &buffer_ptr, &cas); print_buffer_t(buffer_ptr);
    str_find("ema", &buffer_ptr, &cas); print_buffer_t(buffer_ptr);
    str_find("ioana", &buffer_ptr, &cas); print_buffer_t(buffer_ptr);

    str_remove("ana");
    str_find("ana", &buffer_ptr, &cas); print_buffer_t(buffer_ptr);

    for (int i = 1; i <= 2*M; ++i)
    {
        if (DEBUG && i % 1000 == 0) printf("with %d\n", i);
        if (i % 10000 == 0) printf("current memory usage = %zu\n", get_allocated_memory());
        sprintf(buf, "%d", i);

        buffer_from_string(&key, buf);
        hashmap_add(&key, &key);
        buffer_clear(&key);

        if (i > M) {
            sprintf(buf, "%d", i - M);

            buffer_from_string(&key, buf);
            hashmap_find(&key, &buffer_ptr, &cas);
            if (buffer_ptr != NULL && buffer_compare(buffer_ptr, &key) != 0) {
                printf("Difference for %d %s\n", i, buf);
            }
            if (buffer_ptr != NULL && hashmap_remove(&key) != 0) {
                printf("error while removing %d\n", i-M);
            }
            if (DEBUG) printf("find again %s\n", buf);
            hashmap_find(&key, &buffer_ptr, &cas);
            assert(buffer_ptr == NULL);
            buffer_clear(&key);
        }
    }
}

int
read_until_delimiter(struct buffer_t *input, struct buffer_t *output, char delimiter)
{
    size_t output_size;
    for (output_size = 0; output_size < input->size && input->content[output_size] != delimiter; ++output_size);
    if (output_size == input->size) return -1;
    output->size = output_size;
    // DANGER ZONE;
    output->content = input->content;
    return 0;
}

void
offset_buffer(struct buffer_t *buffer, size_t offset)
{
    buffer->size -= offset;
    buffer->content += offset;
}

int
handle_set(struct buffer_t *cmd_content, struct evbuffer *output)
{
    struct buffer_t key, flags, exptime, bytes, value;
    read_until_delimiter(cmd_content, &key, ' ');
    offset_buffer(cmd_content, key.size + 1);
    read_until_delimiter(cmd_content, &flags, ' ');
    offset_buffer(cmd_content, flags.size + 1);
    read_until_delimiter(cmd_content, &exptime, ' ');
    offset_buffer(cmd_content, exptime.size + 1);
    read_until_delimiter(cmd_content, &bytes, ' ');
    offset_buffer(cmd_content, bytes.size + 1);
    value.content = cmd_content->content;
    value.size = cmd_content->size;

    if (hashmap_add(&key, &value) != 0)
    {
        return -1;
    }
    evbuffer_add(output, "STORED\r\n", 8);

    return 0;
}

int
handle_get(struct buffer_t *cmd_content, struct evbuffer *output)
{
    struct buffer_t key, *buffer_ptr;
    uint64_t cas_value;

    key.size = cmd_content->size;
    key.content = cmd_content->content;

    hashmap_find(&key, &buffer_ptr, &cas_value);
    if (buffer_ptr != NULL)
    {
        evbuffer_add(output, "VALUE", 5);
        evbuffer_add(output, " ", 1);
        evbuffer_add(output, key.content, key.size);
        evbuffer_add(output, " 0 ", 3); // flags not supported
        evbuffer_add_printf(output, "%zu %llu\r\n", buffer_ptr->size, cas_value);
        evbuffer_add(output, buffer_ptr->content, buffer_ptr->size);
        evbuffer_add(output, "\r\n", 2);
    }
    evbuffer_add(output, "END\r\n", 5);
    return 0;
}

void
readcb(struct bufferevent *bev, void *ctx)
{
    struct evbuffer *input, *output;
    struct buffer_t *line, *command, *cmd_content;
    input = bufferevent_get_input(bev);
    output = bufferevent_get_output(bev);

    line = memory_allocate(sizeof(struct buffer_t));
    command = memory_allocate(sizeof(struct buffer_t));
    cmd_content = memory_allocate(sizeof(struct buffer_t));
    while ((line->content = evbuffer_readln(input, &line->size, EVBUFFER_EOL_CRLF_STRICT))) {
        if (read_until_delimiter(line, command, ' ') != 0)
        {
            // error
        }

        cmd_content->size = line->size - command->size - 1;
        cmd_content->content = line->content + command->size + 1;

        if (buffer_compare_string(command, "SET") == 0)
        {

            if (handle_set(cmd_content, output) != 0)
            {
                // error
            }
        } else if (buffer_compare_string(command, "GET") == 0 || buffer_compare_string(command, "GETS") == 0)
        {
            if (handle_get(cmd_content, output) != 0)
            {
                // error
            }
        }

        buffer_clear(line);
    }
    memory_free(line);
    memory_free(command);
    memory_free(cmd_content);

    if (evbuffer_get_length(input) >= MEGABYTE)
    {
        // TODO: later

    }
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
        bufferevent_setwatermark(bev, EV_READ, 0, MEGABYTE);
        bufferevent_enable(bev, EV_READ|EV_WRITE);
    }
}

int main(int argc, char **argv)
{
    set_memory_limit((size_t) 200 * MEGABYTE);
    hashmap_init(1*MEGABYTE);

    // hash_test(); return 0;

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
