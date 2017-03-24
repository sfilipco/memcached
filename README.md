Slkcached
---------

An implementation of some of the operations in the Memcached protocol.

Compile and Run
-------------
```
   mkdir bin; cd bin; cmake ../; make; ./slkcached
```

This will start a process on port 11311 that takes cache commands.

Supported Commands
------------------
```
    set key 0 0 0 value\r\n
    get key\r\n
    gets key\r\n
    cas key 0 0 0 cas value\r\n
```
Example:
```
    // Client 1 (telnet 127.0.0.1 11311)
    set ana 0 0 0 mere
    STORED
    get ana
    VALUE ana 0 4 1
    mere
    END
    cas ana 0 0 0 2 prune
    EXISTS
    cas ana 0 0 0 1 prune
    STORED

    // Client 2 (telnet 127.0.0.1 11311)
    cas ana 0 0 0 1 pere
    EXISTS
    get ana
    VALUE ana 0 5 2
    prune
    END
```

Implementation details
----------------------
The hash allocates a table of 1M entries from the start and uses linked lists for collision resolution.
Right now the heap allocated memory is capped at 200MB (rss memory should not be much higher).
If memory is required for an allocation then we remove the least recently used items from the heap until
we have enough memory to do our allocation.
With every allocation of memory we store an extra `size_t` to remember the size of the current allocation
to help with tracking memory at deallocation and reallocation.

Concurrency is handled using libevent routines. We use the buffered event API. We have one thread
in our process. No parallelism at this point.

For the insert commands the `value` must be specified before the first `CRLF` (and can't contain the `CRLF`
bytes), `flags` are not supported and `bytes` is not checked.
There is a branch called `compliant-get-implementation` where the proper protocol implementation is
attempted but somewhere in there some memory gets corrupted... typical. The approach was to peek at
`set` and `cas` operations then extract the bytes value and check the buffer if enough bytes are
buffered and only then extract all they bytes and process the request.

Discussion
----------
* Overall this should be a very fast cache. Because we don't implement the proper protocol we take a
 hit of performance caching large values. I expect most of the time to be spent searching for
 `CRLF` in buffers in environments with more throughput required.
* There are a ton of places in the code where error are not checked and handled. A bad command will
 crash the server right now.
* I tested the hash with a methods but the server I only tested manually using telnet. Moving forward
 I would have used a memcached client implementation to connect to test the connection to the server.
* Why did I choose C? That was a tough decision. Considering that raison d'etre of memcached is
 performance I narrowed the languages to the ones that give most performance: C, C++, Rust. C++ has
 evolved a lot more than C since I last used it and with Rust I have not worked enough (it might
 have been hard to compile). That leaves me with C.
