# GLoIP
GLoIP stands for GL over IP (technically Graphics Library over Internet Protocol).
This project is designed for transporting OpenGL ES calls over the network and executing them on another device.
It is specifically designed for transporting OpenGL ES calls from an Android virtual machine to the host which translates using ANGLE,
and it's part of an in-progress project called LEMVR which aims to run Oculus Quest exclusive content on desktop VR headsets.

## How it works
In GLoIP there is a server and a client. The client is a shared library with the same API as some version of OpenGL ES.
Whenever a function is called in the client library, it communicates with the server to execute the same function on the server and return the results.
The server is a shared library with a single function that starts up the TCP server and continually accepts connections and requests from the client.

The server and client libraries are automatically generated given the API header (glXX.h) and a corresponding metadata file describing the function's arguments.
For more information on how this metadata file is specified, see the example in `protocols/test/test.h.meta`.

GLoIP supports 3 main ways of transporting data.
- Primitives: When a primitive argument is passed, or a primitive value is returned from a function, its value is simply sent over the stream 
along with a byte specifying the size of its data type
- Blob Arguments: This method is used when a pointer is passed to a function along with some specification of the size of the data stored at that pointer.
GLoIP determines the size of the data passed, and sends the data to the server along with an integer specifying the size in bytes.
- Blob Return Arguments: This method is used when a function returns a blob of data by assigning to a pointer argument.
In most cases, the size of the data that will be returned can be determined by the values of other arguments.
The client sends the amount of bytes it has allocated for the result, the server runs the function, and sends back the data that was stored in the pointer.

## Client
Before any forwarded functions are called, GLoIP needs to be initialized using `gloip_initialize` to specify the hostname and port of the server.
At the end of the program, `gloip_shutdown` should be called.

As soon as GLoIP is initialized, any time a forwarded function is called, it sends a request to the server to execute the function
along with data from all the given arguments. It also specifies whether the server should send a packet in return,
and only does so if the function has a non-void return type or if the function has any blob return arguments.
If the client requested a response, it then waits for a response from the server, updates any blob return arguments, and returns the function's return value.

Every thread GLoIP is used in gets a separate connection to the server for concurrency, and each thread on the client corresponds to a thread on the server
to preserve thread-specific aspects of the GL library like contexts.

For more information on how GLoIP works, see `GLoIP-protocol.txt`

## Server
The server is intended to be used as part of another program, running in a separate thread accepting connections. It spawns a new thread for each connection.
When the client requests a function to be called, the server calls that function in the original shared library and sends back any return data.
The server should never send a response if the client did not request one to preserve the state of the protocol.
The server depends on the original shared library, and so should be run in an environment where it is available.

## Instructions
To use GLoIP, you should define a protocol. A protocol consists of the header of the GL library, a metadata file describing functions in the header,
custom code for unusual functions for both the client and the server, and the shared library of the original GL library for the server to link against.
The naming of these files is important; here is an example of how the folder should be laid out if your header is called `test.h`:
```
protocols
└───test
    ├───libtest.so
    ├───test_client_custom.cpp
    ├───test_server_custom.cpp
    ├───test.h
    └───test.h.meta
```
To understand the metadata format, see the documentation in `protocols/test/test.h.meta`
You can generate a template metadata file for your protocol with the `create_meta_template.py` script.

If the metadata declares that a function needs a custom implementation, it MUST be implemented in both of the `*_custom.cpp` files.
The implementations should be declared like so:

`*_client_custom.cpp`:
```
GL_APICALL <return type> GL_APIENTRY functionName(<original function signature>) {
    uint32_t functionHash = HASH_functionName;
    // ...
}
```

`*_server_custom.cpp`:
```
void gloipRedirect_functionName(bool* returnedSomething, uint8_t* returnSize, void* returnLocation, uint8_t numArgs, Argument** args) {
    *returnedSomething = ...; // true if the original function has a non-void return type, otherwise false
    *returnSize = ...; // the size in bytes of the data type the original function returns
    // ...
    memcpy(returnLocation, ..., *returnSize); // to return something if non-void
}
```
See generated source files for non-custom functions for a reference implementation. Server and client implementations can send arbitrary data to one another using
`CustomArgument` arguments. If the server wants to send back more data than the client sent, it should use `CustomArgument::reallocate` to send back 
a larger buffer.

## Compiling
After the above steps are complete, the program can be compiled like so:

To build the client, run `python3 build_client.py <protocol>` where `protocol` is a protocol defined above.
The resulting library can be found at `out/libgloipclient.so`

Building the server is similar. Run `python3 build_server.py <protocol>`. The resulting library can be found at `out/libgloipserver.so`.

## To-Do
- Finish metadata for all of OpenGL ES 3.2
- Create some kind of solution for functions like eglGetProcAddress
- Include multiple protocols in one library
- Implement a page-based, aligned protocol for communicating
