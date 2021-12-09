Amanuensis
----------
[![Build Status](https://travis-ci.org/benjamin-bader/amanuensis.svg?branch=master)](https://travis-ci.org/benjamin-bader/amanuensis)

A faithful assistant to your various network applications, scrupulously copying all that they say and preserving it for the ages.

This will, if I find the time to persevere, become a cross-platform HTTP proxy and web-traffic inspector - like Fiddler, but native and perpetually open-source.


### Build requirements

- QT 6.0+

On Windows:

- Visual Studio 2017

On Linux:

- TBD

On Mac:

- Xcode and command-line tools

### Testing

Qt Creator is adequate for running the unit tests.  If you decide that you'd like to run them on the command-line, good luck - qmake documentation _may_ or _may not_ work on your platform.  On Windows 10, at least, the following has worked:

0. Open a command prompt
0. Source vcvarsall.bat
0. Put qmake on your PATH
0. cd path/to/source
0. `qmake -r "M=2"`
0. `nmake check`

### Code Signing

On macOS, we make use of a launchd "Privileged Helper" to effect system changes - namely, to enable or disable a system-wide HTTP proxy service.  Currently, this requires both the helper and the main application to be cryptographically signed.  You _do not_ need an Apple Developer ID, at least not on Sierra, contrary to at least some of Apple's developer documentation.  A self-signed certificate will suffice; we provide tools to generate and install such a certificate in the `keygen` directory.  To install a suitable code-signing certificate:

```bash
cd keygen
./install_codesigning_cert.sh
```

Be sure to enter a non-empty passcode, otherwise Keychain Access will not be able to import the cert.

This script installs a signing identity named "Amanuensis Developers"; you can use this identity to sign arbitrary things, and as of 22 September 2017, launchd will accept privileged helpers signed with it.

Note that you may need to do a dance with SMJobBlessUtil.py, in the form of:
0. build
0. SMJobBlessUtil.py setreq path/to/packaged/app path/to/app/info.plist path/to/helper/info.plist
0. rebuild
0. SMJobBlessUtil.py check path/to/packaged/app

SMJobBlessUtil.py is available as a download from Apple; we haven't yet implemented packaging up a distribution.  **These steps are both speculative and aspirational**.

It is our intention that this process will fulfill our obligations under GPLv3 § 6.

### Design

The project is divided into `app` and `core` modules.  The former contains the QT application logic and UI, while `core` is a more-or-less headless implementation of a cross-platform HTTP proxy.  As `app` is currently a non-entity, it will not be described here.

`core` implements a proxy server in terms of the following abstractions:

- `Proxy`
  An object provides an transparent-HTTP-proxy service on a given port, emitting Transaction objects to interested consumers of `core` as traffic is intercepted. 

- `Transaction`
  An object that consumers of `core` can use to listen for events in the lifecycle of a proxied request/response pair.  Events emitted include relevant HTTP data, such as request lines, headers, bodies, response codes, and errors.  It is a concrete implementation of an RFC 7230-compliant(-ish) proxy Transaction.

How It Works

When a `Proxy` is initialized, it opens a listening socket and sets up a pool of ASIO `io_context` workers.  When a client connection is accepted, a `Connection` is obtained from the `ConnectionPool` and given to a new `Transaction`.  The `Transaction` is owned by the `Proxy`.  From here, processing is driven directly by the `Transaction`.

`Transaction` initializes an `HttpMessageParser`, then begins to read data from the client `Connection`.  As soon as a complete HTTP request line is parsed and validated, the `Transaction` asynchronously obtains a remote `Connection` from the `ConnectionPool`.  Simultaneously, the `Transaction` emits lifecycle events; this continues for the duration of the transaction.  As soon as all HTTP request headers are read _AND_ the remote `Connection` is resolved and open, the `Transaction` begins to forward the request to the remote `Connection`.  When the request is completely forwarded, the `Transaction` begins to read response data from the remote.  As response data is parsed, events are emitted.  Response data is relayed to the client as it is received.

When all response data is completely relayed to the client, the transaction is finished, and the `Transaction` cleans itself up.  This involves deciding whether either `Connection` objects should remain open, and returning them to the pool (or destroying them) as appropriate, and queuing itself for deletion.

----------------------
Copyright (C) 2017-2021 Benjamin Bader
