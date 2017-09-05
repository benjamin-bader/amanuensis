Amanuensis
----------

A faithful assistant to your various network applications, scrupulously copying all that they say and preserving it for the ages.

This will, if I find the time to persevere, become a cross-platform HTTP proxy and web-traffic inspector - like Fiddler, but native and perpetually open-source.


### Build requirements

- QT 5.9+

On Windows:

- Visual Studio 2017

On Linux:

- TBD

On Mac:

- Xcode 8.x+ and command-line tools

### Testing

Qt Creator is adequate for running the unit tests.  If you decide that you'd like to run them on the command-line, good luck - qmake documentation _may_ or _may not_ work on your platform.  On Windows 10, at least, the following has worked:

0. Open a command prompt
0. Source vcvarsall.bat
0. Put qmake on your PATH
0. cd path/to/source
0. `qmake -r "M=2"`
0. `nmake check`


### Design

The project is divided into `app` and `core` modules.  The former contains the QT application logic and UI, while `core` is a more-or-less vanilla C++ implementation of a cross-platform HTTP proxy.  As `app` is currently a non-entity, it will not be described here.

`core` implements a proxy server in terms of the following abstractions:

- `Proxy`
  An object provides an transparent-HTTP-proxy service on a given port, emitting Transaction objects to interested consumers of `core` as traffic is intercepted. 

- `Transaction`
  An abstract interface that consumers of `core` can use to listen for events in the lifecycle of a proxied request/response pair.  Events emitted include relevant HTTP data, such as request lines, headers, bodies, response codes, and errors.

Implementations 

- `ProxyTransaction`
  A concrete implementation of an RFC 7230-compliant(-ish) proxy Transaction.

How It Works

When a `Proxy` is initialized, it opens a listening socket and sets up a pool of ASIO `io_service` workers.  When a client connection is accepted, a `Connection` is obtained from the `ConnectionPool` and given to a new `ProxyTransaction`.  The `ProxyTransaction` is owned by the `Proxy`, and is shared with any `TransactionListener` objects registered on the `Proxy`.  From here, processing is driven directly by the `ProxyTransaction`.

`ProxyTransaction` initializes (possibly inherits?) an `HttpMessageParser`, then begins to read data from the client `Connection`.  As soon as a complete HTTP request line is parsed and validated, the `ProxyTransaction` asynchronously obtains a remote `Connection` from the `ConnectionPool`.  Simultaneously, the `ProxyTransaction` emits lifecycle events to any `TransactionListener` objects registered with itself; this continues for the duration of the transaction.  As soon as all HTTP request headers are read _AND_ the remote `Connection` is resolved and open, the `ProxyTransaction` begins to forward the request to the remote `Connection`.  When the request is completely forwarded, the `ProxyTransaction` begins to read response data from the remote.  As response data is parsed, events are emitted.  Response data is relayed to the client as it is received.

When all response data is completely relayed to the client, the transaction is finished, and the `ProxyTransaction` cleans itself up.  This involves deciding whether either `Connection` objects should remain open, and returning them to the pool (or destroying them) as appropriate.

TODO:
- Refactor HttpMessageParser to expose parse events necessary to implement TransactionListener

----------------------
Copyright (C) 2017 Benjamin Bader
