jsonrpc-c
=========

JSON-RPC in C (server only for now)

What?
-----
A library for a C program to receive JSON-RPC requests on tcp sockets (no HTTP).

Why?
----
I needed a way for an application written in C, running on an embedded Linux system to be configured by
a Java/Swing configuration tool running on a connected laptop. Wanted something simple, human readable,
and saw no need for HTTP.

How?
----
It depends on libev (was already use on the embedded app) and includes cJSON (with a small patch). No further dependencies.
libev isn't that important for a server, a single select() would suffice, but it might help with portability,
and might be more useful later when the library is also a JSON-RPC _client_.

###Testing

Run `autoreconf -i`  before `./configure` and `make`

Test the example server by running it and typing: 

`echo "{\"method\":\"sayHello\"}" | nc localhost 1234`

or

`echo "{\"method\":\"exit\"}" | nc localhost 1234`

Who?
----

@hmngomes