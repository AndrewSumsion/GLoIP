#ifndef GLOIP_SERVER_H
#define GLOIP_SERVER_H 1

#include <cstdint>
using std::uint8_t;
using std::uint32_t;
using std::uint64_t;

#include <cstring>
using std::memcpy;

#include "TcpSocket.h"
using namespace tcplib;

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
};

struct PrimitiveArgument : public Argument {
    uint8_t typeSize;

    union {
        uint8_t data[8];
        uint64_t dataInteger;
        double dataDouble;
        float dataFloat;
    };

    PrimitiveArgument(uint8_t typeSize, const void* data);

    ArgumentType getType();
    uint32_t getSize();
};

struct BlobArgument : public Argument {
    uint32_t size;
    const uint8_t* data;

    BlobArgument(uint32_t size, const uint8_t* data);
    ~BlobArgument();

    ArgumentType getType();
    uint32_t getSize();
};

struct BlobReturnArgument : public Argument {
    uint32_t size;
    uint8_t* destination;

    BlobReturnArgument(uint32_t size, uint8_t* destination);
    ~BlobReturnArgument();

    ArgumentType getType();
    uint32_t getSize();
};

struct CustomArgument : public Argument {
    uint32_t size;
    uint8_t* data;

    CustomArgument(uint32_t size);
    ~CustomArgument();

    ArgumentType getType();
    uint32_t getSize();
};

typedef void(*GloipFunction)(bool* returnedSomething, uint8_t* returnSize, void* returnLocation, uint8_t numArgs, Argument** args);

GloipFunction* gloip_getGloipFunctionArray();

// returns the function corresponding to this hash. not the actual opengl function, a forwarding function that reads from a list of passed Arguments
inline GloipFunction gloip_getGloipFunction(uint32_t hash) {
    return gloip_getGloipFunctionArray()[hash];
}

GLOIP_EXPORT void gloip_startServer(const char* hostname, int port);

void gloip_mainLoop(TcpSocket connection);

// returns true until a shutdown packet has been read, then returns false
bool gloip_handleNextCommand(TcpSocket* io);

// reads the next argument from the execute function packet (heap allocated)
Argument* gloip_readArgument(TcpSocket* io);

PrimitiveArgument* gloip_createPrimitiveArgument(uint32_t size, const uint8_t* buffer);
BlobArgument* gloip_createBlobArgument(uint32_t size, const uint8_t* buffer);
BlobReturnArgument* gloip_createBlobReturnArgument(uint32_t size, const uint8_t* buffer);
CustomArgument* gloip_createCustomArgument(uint32_t size, const uint8_t* buffer);

inline bool gloip_checkArgTypes(int numExpectedTypes, ArgumentType* expectedTypes, uint8_t numArgs, Argument** args) {
    if(numArgs < numExpectedTypes) return false;
    for(int i = 0; i < numExpectedTypes; i++) {
        if(args[i]->getType() != expectedTypes[i]) {
            return false;
        }
    }
    return true;
}

#define GLOIP_CHECK_ARGS(...) do { \
        ArgumentType expectedTypes[] = {__VA_ARGS__}; \
        int numExpectedTypes = sizeof(expectedTypes) / sizeof(expectedTypes[0]); \
        if(!gloip_checkArgTypes(numExpectedTypes, expectedTypes, numArgs, args)) { \
            return; \
        } \
    } while(0)

#endif // GLOIP_SERVER_H