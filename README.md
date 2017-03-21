Slkcached
---------

An implementation of some of the operations in the Memcached protocol.

HashImplementation
------------------
Single threaded hash, single threaded requests.
1M add -> 9M add->find->remove in under 10s without optimizations
we should see how optimizations work out.