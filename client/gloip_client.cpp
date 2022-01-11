#include "gloip_client.h"

#include "TcpIOHandler.h"

#include <vector>
using std::vector;

#include <typeinfo>

#include <cstdio>

void Argument::writeHeaderToBuffer(uint8_t* buffer) {
    uint32_t size = getSize();
    std::memcpy(buffer, &size, sizeof(uint32_t));
    buffer[4] = getType();
}

PrimitiveArgument::PrimitiveArgument(uint8_t typeSize, void* primitive)
    : typeSize(typeSize)
    {
    std::memcpy(this->data, primitive, typeSize);
}

ArgumentType PrimitiveArgument::getType() {
    return TYPE_PRIMITIVE;
}

void PrimitiveArgument::writeToBuffer(uint8_t* buffer) {
    writeHeaderToBuffer(buffer);
    buffer += 5;
    buffer[0] = typeSize;
    buffer += 1;
    std::memcpy(buffer, data, typeSize);
}

uint32_t PrimitiveArgument::getSize() {
    return 1 + 1 + 8;
}

BlobArgument::BlobArgument(uint32_t size, const void* data)
    : size(size),
      data(data)
    {

}

ArgumentType BlobArgument::getType() {
    return TYPE_POINTER_BLOB;
}

void BlobArgument::writeToBuffer(uint8_t* buffer) {
    writeHeaderToBuffer(buffer);
    buffer += 5;
    std::memcpy(buffer, &size, sizeof(uint32_t));
    buffer += 4;
    std::memcpy(buffer, data, size);
}

uint32_t BlobArgument::getSize() {
    return 1 + sizeof(uint32_t) + size;
}

ArgumentType BlobReturnArgument::getType() {
    return TYPE_POINTER_RETURN;
}

BlobReturnArgument::BlobReturnArgument(uint32_t size, void* destination)
    : size(size),
      destination(destination)
    {

}

void BlobReturnArgument::writeToBuffer(uint8_t* buffer) {
    writeHeaderToBuffer(buffer);
    buffer += 5;
    std::memcpy(buffer, &size, sizeof(uint32_t));
}

uint32_t BlobReturnArgument::getSize() {
    return 1 + sizeof(uint32_t);
}

CustomArgument::CustomArgument(uint32_t size)
    : size(size) {
    data = new uint8_t[size];
}

CustomArgument::~CustomArgument() {
    delete [] data;
}

ArgumentType CustomArgument::getType() {
    return TYPE_CUSTOM;
}

void CustomArgument::writeToBuffer(uint8_t* buffer) {
    writeHeaderToBuffer(buffer);
    buffer += 5;
    memcpy(buffer, &size, sizeof(uint32_t));
    buffer += sizeof(uint32_t);
    memcpy(buffer, data, size);
}

uint32_t CustomArgument::getSize() {
    return 1 + sizeof(uint32_t) + size;
}

void gloip_cleanupHandler(void* arg) {
    printf("cleaning up connection on thread exit\n");

    IOHandler* io = (IOHandler*) arg;

    if(io == nullptr) {
        return;
    }

    uint8_t shutdownBuffer[4] = {83, 84, 79, 80};
    io->writeAll(4, shutdownBuffer);
    io->readAll(4, shutdownBuffer);
    if(shutdownBuffer[0] != 79 || shutdownBuffer[1] != 75 || shutdownBuffer[2] != 33 || shutdownBuffer[3] != 33) {
        // TODO: Do something
    }

    io->disconnect();
}

static char* gloipHostname;
static int gloipPort;
static pthread_key_t connectionKey;

void gloip_initialize(const char* hostname, int port) {
    size_t size = strlen(hostname) + 1;
    gloipHostname = new char[size];
    memcpy(gloipHostname, hostname, size);
    gloipPort = port;

    pthread_key_create(&connectionKey, gloip_cleanupHandler);
}

void gloip_shutdown() {
    delete [] gloipHostname;
    pthread_key_delete(connectionKey);
}

IOHandler* gloip_getConnection() {
    IOHandler* connection = (IOHandler*) pthread_getspecific(connectionKey);
    if(connection == nullptr) {
        connection = gloip_createConnection();
        pthread_setspecific(connectionKey, connection);
    }
    return connection;
}

IOHandler* gloip_createConnection() {
    IOHandler* handler = new TcpIOHandler(gloipHostname, gloipPort);
    handler->connect();
    uint8_t handshakeBuffer[4] = {1, 8, 6, 7};
    handler->writeAll(4, handshakeBuffer);
    handler->readAll(4, handshakeBuffer);
    if(handshakeBuffer[0] != 5 || handshakeBuffer[1] != 3 || handshakeBuffer[2] != 0 || handshakeBuffer[3] != 9) {
        // TODO: Do some kind of error handling here
        return nullptr;
    }

    return handler;
}

void gloip_execute(uint32_t functionHash, bool waitForReturn, size_t returnSize, void* returnLocation, int numArgs, Argument** args) {
    IOHandler* io = gloip_getConnection();

    gloip_sendRequest(io, functionHash, waitForReturn, numArgs, args);

    if(!waitForReturn) {
        // if waitForReturn is false, we shouldn't wait, and the server will not send a response
        return;
    }

    gloip_waitForResponse(io, returnSize, returnLocation, numArgs, args);
}

void gloip_sendRequest(IOHandler* io, uint32_t functionHash, bool sendResponse, int numArgs, Argument** args) {
    uint8_t headerPacket[] = {
        100, 6, 0, 0, 0, 0, sendResponse ? 1 : 0, numArgs
    };
    std::memcpy(headerPacket + 2, &functionHash, sizeof(uint32_t));
    io->writeAll(sizeof(headerPacket), headerPacket);

    for(int i = 0; i < numArgs; i++) {
        Argument* arg = args[i];
        gloip_writeArgument(io, arg);
    }
}

void gloip_writeArgument(IOHandler* io, Argument* arg) {
    uint32_t packetSize = arg->getSize() + 4;
    uint8_t* packet = new uint8_t[packetSize];
    arg->writeToBuffer(packet);
    io->writeAll(packetSize, packet);
    delete[] packet;
}

void gloip_waitForResponse(IOHandler* io, size_t returnSize, void* returnLocation, int numArgs, Argument** args) {
    uint8_t returnHeader[6];
    io->readAll(6, returnHeader);
    // TODO: Assert everything is correct in returnHeader

    uint8_t numReturnArgs = returnHeader[5];
    int argIndex = 0;
    for(int i = 0; i < numReturnArgs; i++) {
        uint32_t argSize;
        io->readAll(4, (uint8_t*)&argSize);
        uint8_t* argInBuffer = new uint8_t[argSize];
        io->readAll(argSize, argInBuffer);
        ArgumentType argType = (ArgumentType) argInBuffer[0];
        
        const uint8_t* argBuffer = argInBuffer + 1;

        if(argType == TYPE_PRIMITIVE) {
            // ignore the type sent back by the server, just copy as much as the caller specified.
            // this assumes the caller is putting in the correct sizes and that the server is sending back the right amount of data
            std::memcpy(returnLocation, argBuffer + 1, returnSize);
        }
        else if(argType == TYPE_POINTER_RETURN) {
            // move forward to the next return argument
            while(argIndex < numArgs && args[argIndex]->getType() != TYPE_POINTER_RETURN) {
                argIndex++;
            }

            // note: this is a safe cast, as we manually checked type above.
            BlobReturnArgument* arg = (BlobReturnArgument*)(args[argIndex]);
            
            // also ignoring the size value sent by the server
            // TODO: use the minimum of the client and server-side size values for robustness
            std::memcpy(arg->destination, argBuffer + 4, arg->size);
        }
        else if(argType == TYPE_CUSTOM) {
            uint32_t providedSize;
            memcpy(&providedSize, argBuffer, sizeof(uint32_t));

            // find the custom argument in the list of arguments
            CustomArgument* customArg = nullptr;
            for(int i = 0; i < numArgs; i++) {
                if(args[i]->getType() == TYPE_CUSTOM) {
                    customArg = (CustomArgument*) args[i];
                    break;
                }
            }
            // if custom argument is present, copy any data
            if(customArg != nullptr) {
                // copy the minimum of how much data was provided and how much data our CustomArgument can hold
                uint32_t dataToCopy = customArg->size < providedSize ? customArg->size : providedSize;
                memcpy(customArg->data, argBuffer + 4, dataToCopy);
            }
        }

        delete[] argInBuffer;
    }
}