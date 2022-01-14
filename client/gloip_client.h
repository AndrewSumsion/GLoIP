#ifndef GLOIP_CLIENT_H
#define GLOIP_CLIENT_H 1

#include "IOHandler.h"

#include <cstdint>
using std::uint8_t;
using std::uint32_t;

#include <cstring>

#define GLOIP_EXPORT extern "C"

enum ArgumentType : uint8_t {
    TYPE_PRIMITIVE = 0,
    TYPE_POINTER_BLOB = 1,
    TYPE_POINTER_RETURN = 2,
    TYPE_CUSTOM = 3
};

struct Argument {
    virtual ArgumentType getType() = 0;

    // returns the size of this argument in bytes, including the 1-byte type, but not the 4-byte size
    virtual uint32_t getSize() = 0;

    virtual void writeToBuffer(uint8_t* buffer) = 0;

    void writeHeaderToBuffer(uint8_t* buffer);
};

struct PrimitiveArgument : public Argument {
    uint8_t typeSize;
    uint8_t data[8];

    PrimitiveArgument(uint8_t typeSize, void* data);

    ArgumentType getType();
    void writeToBuffer(uint8_t* buffer);
    uint32_t getSize();
};

struct BlobArgument : public Argument {
    uint32_t size;
    const void* data;

    BlobArgument(uint32_t size, const void* data);

    ArgumentType getType();
    void writeToBuffer(uint8_t* buffer);
    uint32_t getSize();
};

struct BlobReturnArgument : public Argument {
    uint32_t size;
    void* destination;

    BlobReturnArgument(uint32_t size, void* destination);

    ArgumentType getType();
    void writeToBuffer(uint8_t* buffer);
    uint32_t getSize();
};

struct CustomArgument : public Argument {
    uint32_t size;

    // IMPORTANT: This pointer should not be stored outside this class, as it may be changed
    uint8_t* data;

    CustomArgument(uint32_t size);
    ~CustomArgument();

    ArgumentType getType();
    void writeToBuffer(uint8_t* buffer);
    uint32_t getSize();
    void reallocate(uint32_t newSize);
};

GLOIP_EXPORT void gloip_initialize(const char* hostname, int port);
GLOIP_EXPORT void gloip_shutdown();

IOHandler* gloip_getConnection();
IOHandler* gloip_createConnection();

void gloip_execute(uint32_t functionHash, bool waitForReturn, size_t returnSize, void* returnLocation, int numArgs, Argument** args);
bool gloip_sendRequest(IOHandler* io, uint32_t functionHash, bool sendResponse, int numArgs, Argument** args);
bool gloip_writeArgument(IOHandler* io, Argument* arg);
bool gloip_waitForResponse(IOHandler* io, size_t returnSize, void* returnLocation, int numArgs, Argument** args);

#endif // GLOIP_CLIENT_H