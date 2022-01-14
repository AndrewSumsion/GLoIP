#ifndef TCPIOHANDLER_H
#define TCPIOHANDLER_H

#include "IOHandler.h"

#include <cstdint>
using std::uint8_t;

class TcpIOHandler : public IOHandler {
public:
    int error;
    int fd;
    const char* hostname;
    int port;

    TcpIOHandler(const char* hostname, int port);

    bool write(int size, const uint8_t* buffer, int* bytesWritten);
    bool read(int size, uint8_t* buffer, int* bytesRead);
    bool connect();
    bool disconnect();
    int getError();
};

#endif //TCPIOHANDLER_H
