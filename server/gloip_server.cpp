#include "gloip_server.h"

#include "TcpServer.h"

#include <thread>
using std::thread;

PrimitiveArgument::PrimitiveArgument(uint8_t typeSize, const void* primitive)
    : typeSize(typeSize)
    {
    memcpy(this->data, primitive, typeSize);
}

ArgumentType PrimitiveArgument::getType() {
    return TYPE_PRIMITIVE;
}

uint32_t PrimitiveArgument::getSize() {
    return 1 + 1 + 8;
}

BlobArgument::BlobArgument(uint32_t size, const uint8_t* data)
    : size(size),
      data(data)
    {

}

BlobArgument::~BlobArgument() {
    delete [] data;
}

ArgumentType BlobArgument::getType() {
    return TYPE_POINTER_BLOB;
}

uint32_t BlobArgument::getSize() {
    return 1 + sizeof(uint32_t) + size;
}

ArgumentType BlobReturnArgument::getType() {
    return TYPE_POINTER_RETURN;
}

BlobReturnArgument::BlobReturnArgument(uint32_t size, uint8_t* destination)
    : size(size),
      destination(destination)
    {

}

BlobReturnArgument::~BlobReturnArgument() {
    delete [] destination;
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

uint32_t CustomArgument::getSize() {
    return 1 + sizeof(uint32_t) + size;
}

void gloip_startServer(const char* hostname, int port) {
    TcpServer server = TcpServer();
    server.startServer(hostname, port);

    // TODO: add an exit condition
    while(true) {
        TcpSocket socket;
        server.accept(&socket);

        uint8_t handshakeBuffer[4];
        
        socket.readAll(4, handshakeBuffer);
        if(handshakeBuffer[0] != 1 || handshakeBuffer[1] != 8 || handshakeBuffer[2] != 6 || handshakeBuffer[3] != 7) {
            // invalid connection
            socket.close();
            continue;
        }

        handshakeBuffer[0] = 5;
        handshakeBuffer[1] = 3;
        handshakeBuffer[2] = 0;
        handshakeBuffer[3] = 9;

        socket.writeAll(4, handshakeBuffer);

        thread t = thread(gloip_mainLoop, socket);
        t.detach();
    }
}

void gloip_mainLoop(TcpSocket connection) {
    while(gloip_handleNextCommand(&connection));
    connection.close();
}

bool gloip_handleNextCommand(TcpSocket* io) {
    uint8_t packetId;
    io->readAll(1, &packetId);

    // handle shutdown
    if(packetId == 83) {
        uint8_t shutdownBuffer[4];
        shutdownBuffer[0] = packetId;
        io->readAll(3, shutdownBuffer + 1);
        if(shutdownBuffer[0] != 83 || shutdownBuffer[1] != 84 || shutdownBuffer[2] != 79 || shutdownBuffer[3] != 80) {
            // something's really wrong, shut down anyway
            return false;
        }

        shutdownBuffer[0] = 79;
        shutdownBuffer[1] = 75;
        shutdownBuffer[2] = 33;
        shutdownBuffer[3] = 33;

        io->writeAll(4, shutdownBuffer);
        return false;
    }

    if(packetId != 100) {
        // invalid data
        return false;
    }

    // read function execution request
    uint8_t headerLength;
    io->readAll(1, &headerLength);

    uint8_t* headerBuffer = new uint8_t[headerLength];
    io->readAll(headerLength, headerBuffer);
    uint32_t functionHash; // = *((uint32_t*)headerBuffer);
    memcpy(&functionHash, headerBuffer, sizeof(uint32_t));
    bool sendResponse = (bool) headerBuffer[4];
    uint8_t numArgs = headerBuffer[5];
    delete[] headerBuffer;

    // Note: these are heap allocated
    Argument* args[numArgs];

    for(int i = 0; i < numArgs; i++) {
        args[i] = gloip_readArgument(io);
    }

    // execute function and get info about return type
    bool nonVoidReturnType;
    uint8_t returnSize;
    uint8_t returnValue[8];

    GloipFunction gloipFunction = gloip_getGloipFunction(functionHash);
    gloipFunction(&nonVoidReturnType, &returnSize, returnValue, numArgs, args);

    if(!sendResponse) {
        // exit here if client is not expecting a response
        return true;
    }

    uint8_t responseHeader[6];
    responseHeader[0] = 101;
    memcpy(responseHeader + 1, &functionHash, 1);

    // figure out how many arguments we're going to return (including primitive return)
    uint8_t numReturnArgs = 0;

    if(nonVoidReturnType)
        numReturnArgs++;
    
    for(int i = 0; i < numArgs; i++) {
        if(args[0]->getType() == TYPE_POINTER_RETURN || args[0]->getType() == TYPE_CUSTOM) {
            numReturnArgs++;
        }
    }

    responseHeader[5] = numReturnArgs;

    io->writeAll(6, responseHeader);

    if(nonVoidReturnType) {
        uint32_t returnBufferLength = 14;
        uint8_t returnBuffer[returnBufferLength];

        uint32_t argLength = returnBufferLength - sizeof(uint32_t);
        memcpy(returnBuffer, &argLength, sizeof(uint32_t));
        returnBuffer[4] = TYPE_PRIMITIVE;
        returnBuffer[5] = returnSize;
        memcpy(returnBuffer + 6, returnValue, 8);

        io->writeAll(returnBufferLength, returnBuffer);
    }

    for(int i = 0; i < numArgs; i++) {
        if(args[i]->getType() != TYPE_POINTER_RETURN) {
            continue;
        }
        BlobReturnArgument* arg = (BlobReturnArgument*) args[i];
        uint32_t argSize = arg->size;

        uint32_t returnBufferLength = 9 + argSize;
        uint8_t* returnBuffer = new uint8_t[returnBufferLength];
        memcpy(returnBuffer, &returnBufferLength, sizeof(uint32_t));
        returnBuffer[4] = arg->getType();
        memcpy(returnBuffer + 5, &argSize, sizeof(uint32_t));
        memcpy(returnBuffer + 9, arg->destination, argSize);

        io->writeAll(returnBufferLength, returnBuffer);

        delete [] returnBuffer;
    }

    // find the custom argument if one exists and return the modified data
    // TODO: refactor this to use the same code as a pointer return because they are basically identical
    for(int i = 0; i < numArgs; i++) {
        if(args[i]->getType() != TYPE_CUSTOM) {
            continue;
        }
        CustomArgument* arg = (CustomArgument*) args[i];
        uint32_t argSize = arg->size;

        uint32_t returnBufferLength = 9 + argSize;
        uint8_t* returnBuffer = new uint8_t[returnBufferLength];
        memcpy(returnBuffer, &returnBufferLength, sizeof(uint32_t));
        returnBuffer[4] = arg->getType();
        memcpy(returnBuffer + 5, &argSize, sizeof(uint32_t));
        memcpy(returnBuffer + 9, arg->data, argSize);

        io->writeAll(returnBufferLength, returnBuffer);

        delete [] returnBuffer;
    }

    for(int i = 0; i < numArgs; i++) {
        // delete all the Argument objects because they're dynamically allocated
        delete args[i];
    }

    return true;
}

Argument* gloip_readArgument(TcpSocket* io) {
    uint32_t argumentLength;
    io->readAll(4, (uint8_t*)&argumentLength);
    uint8_t* argumentBuffer = new uint8_t[argumentLength];
    io->readAll(argumentLength, argumentBuffer);
    ArgumentType argType = (ArgumentType)argumentBuffer[0];

    Argument* result = nullptr;

    switch(argType) {
        case TYPE_PRIMITIVE:
            result = gloip_createPrimitiveArgument(argumentLength, argumentBuffer);
            break;
        case TYPE_POINTER_BLOB:
            result = gloip_createBlobArgument(argumentLength, argumentBuffer);
            break;
        case TYPE_POINTER_RETURN:
            result = gloip_createBlobReturnArgument(argumentLength, argumentBuffer);
            break;
        case TYPE_CUSTOM:
            result = gloip_createCustomArgument(argumentLength, argumentBuffer);
            break;
    }

    delete [] argumentBuffer;
    return result;
}

PrimitiveArgument* gloip_createPrimitiveArgument(uint32_t size, const uint8_t* buffer) {
    uint8_t sizeOfDataType = buffer[1];
    return new PrimitiveArgument(sizeOfDataType, buffer + 2);
}

BlobArgument* gloip_createBlobArgument(uint32_t size, const uint8_t* buffer) {
    uint32_t dataSize; // = *((uint32_t*)(buffer + 1));
    memcpy(&dataSize, buffer + 1, sizeof(uint32_t));
    uint8_t* blobData = new uint8_t[dataSize];
    memcpy(blobData, buffer + 5, dataSize);
    return new BlobArgument(dataSize, blobData);
}

BlobReturnArgument* gloip_createBlobReturnArgument(uint32_t size, const uint8_t* buffer) {
    uint32_t dataSize; // = *((uint32_t*)(buffer + 1));
    memcpy(&dataSize, buffer + 1, sizeof(uint32_t));
    uint8_t* blobData = new uint8_t[dataSize];
    return new BlobReturnArgument(dataSize, blobData);
}

CustomArgument* gloip_createCustomArgument(uint32_t size, const uint8_t* buffer) {
    uint32_t dataSize; // = *((uint32_t*)(buffer + 1));
    memcpy(&dataSize, buffer + 1, sizeof(uint32_t));
    CustomArgument* arg = new CustomArgument(dataSize);
    memcpy(arg->data, buffer + 5, dataSize);
    return arg;
}