#include "gloip_client.h"

#include "TcpIOHandler.h"

#include <vector>
using std::vector;

#include <typeinfo>

#include <cstdio>

#define LOG(...) printf(__VA_ARGS__)

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
    if(data == nullptr) {
        size = 0;
    }
}

ArgumentType BlobArgument::getType() {
    return TYPE_POINTER_BLOB;
}

void BlobArgument::writeToBuffer(uint8_t* buffer) {
    writeHeaderToBuffer(buffer);
    buffer += 5;
    std::memcpy(buffer, &size, sizeof(uint32_t));
    buffer += 4;
    if(size > 0) {
        std::memcpy(buffer, data, size);
    }
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

void CustomArgument::reallocate(uint32_t newSize) {
    delete [] data;
    size = newSize;
    data = new uint8_t[size];
}

void gloip_cleanupHandler(void* arg) {
    LOG("cleaning up connection on thread exit\n");
    bool success;

    IOHandler* io = (IOHandler*) arg;

    if(io == nullptr) {
        return;
    }

    uint8_t shutdownBuffer[4] = {83, 84, 79, 80};
    success = io->writeAll(4, shutdownBuffer);
    if(!success) {
        LOG("IO Error sending thread shutdown request! Error: %d\n", io->getError());
        return;
    }

    success = io->readAll(4, shutdownBuffer);
    if(!success) {
        LOG("IO Error receiving thread shutdown response! Error: %d\n", io->getError());
        return;
    }
    if(shutdownBuffer[0] != 79 || shutdownBuffer[1] != 75 || shutdownBuffer[2] != 33 || shutdownBuffer[3] != 33) {
        LOG("Server sent invalid shutdown response!\n");
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
    if(connection != nullptr && !connection->isConnected()) {
        delete connection;
        connection = gloip_createConnection();
        pthread_setspecific(connectionKey, connection);
    }
    return connection;
}

IOHandler* gloip_createConnection() {
    bool success;

    IOHandler* handler = new TcpIOHandler(gloipHostname, gloipPort);
    success = handler->connect();
    if(!success) {
        LOG("IO Error connecting to server! Error: %d\n", handler->getError());
        return nullptr;
    }

    uint8_t handshakeBuffer[4] = {1, 8, 6, 7};

    success = handler->writeAll(4, handshakeBuffer);
    if(!success) {
        LOG("IO Error sending handshake to server! Error: %d\n", handler->getError());
        handler->disconnect();
        return nullptr;
    }

    success = handler->readAll(4, handshakeBuffer);
    if(!success) {
        LOG("IO Error receiving handshake from server! Error: %d\n", handler->getError());
        handler->disconnect();
        return nullptr;
    }

    if(handshakeBuffer[0] != 5 || handshakeBuffer[1] != 3 || handshakeBuffer[2] != 0 || handshakeBuffer[3] != 9) {
        LOG("Server sent invalid handshake!\n");
        handler->disconnect();
        return nullptr;
    }

    return handler;
}

void gloip_execute(uint32_t functionHash, bool waitForReturn, size_t returnSize, void* returnLocation, int numArgs, Argument** args) {
    IOHandler* io = gloip_getConnection();

    if(io == nullptr) {
        LOG("Unable to acquire connection to server. Ignoring this function call.\n");
        memset(returnLocation, 0, returnSize);
        return;
    }

    if(!gloip_sendRequest(io, functionHash, waitForReturn, numArgs, args)) {
        LOG("Error occurred sending request to server. Closing connection...\n");
        io->disconnect();
        memset(returnLocation, 0, returnSize);
        return;
    }

    if(!waitForReturn) {
        // if waitForReturn is false, we shouldn't wait, and the server will not send a response
        return;
    }

    if(!gloip_waitForResponse(io, returnSize, returnLocation, numArgs, args)) {
        LOG("Error occurred waiting for response from server. Closing connection...\n");
        io->disconnect();
        memset(returnLocation, 0, returnSize);
    }
}

bool gloip_sendRequest(IOHandler* io, uint32_t functionHash, bool sendResponse, int numArgs, Argument** args) {
    uint8_t headerPacket[] = {
        100, 6, 0, 0, 0, 0, sendResponse ? (uint8_t)1 : (uint8_t)0, (uint8_t)numArgs
    };
    std::memcpy(headerPacket + 2, &functionHash, sizeof(uint32_t));
    IO(io->writeAll(sizeof(headerPacket), headerPacket));

    for(int i = 0; i < numArgs; i++) {
        Argument* arg = args[i];
        IO(gloip_writeArgument(io, arg));
    }

    return true;
}

bool gloip_writeArgument(IOHandler* io, Argument* arg) {
    uint32_t packetSize = arg->getSize() + 4;
    uint8_t* packet = new uint8_t[packetSize];
    arg->writeToBuffer(packet);
    IO(io->writeAll(packetSize, packet));
    delete[] packet;

    return true;
}

bool gloip_waitForResponse(IOHandler* io, size_t returnSize, void* returnLocation, int numArgs, Argument** args) {
    uint8_t returnHeader[6];
    IO(io->readAll(6, returnHeader));
    // TODO: Assert everything is correct in returnHeader

    uint8_t numReturnArgs = returnHeader[5];
    int argIndex = 0;
    for(int i = 0; i < numReturnArgs; i++) {
        uint32_t argSize;
        IO(io->readAll(4, (uint8_t*)&argSize));
        uint8_t* argInBuffer = new uint8_t[argSize];
        IO(io->readAll(argSize, argInBuffer));
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
                // if the server sends back more data than we sent, reallocate enough room for the data
                if(providedSize > customArg->size) {
                    customArg->reallocate(providedSize);
                }
                if(providedSize > 0) {
                    memcpy(customArg->data, argBuffer + 4, providedSize);
                }
            }
        }

        delete[] argInBuffer;
    }

    return true;
}