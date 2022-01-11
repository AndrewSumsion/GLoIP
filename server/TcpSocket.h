#ifndef TCPLIB_TCPSOCKET_H
#define TCPLIB_TCPSOCKET_H 1

#include "Socket.h"

#include <cstdint>
using std::uint8_t;

namespace tcplib {

class TcpSocket {
private:
    SOCKET socketHandle;
    int error;

public:
    TcpSocket(SOCKET socketHandle);
    TcpSocket();

    void setHandle(SOCKET socketHandle);

    bool read(int size, uint8_t* buffer, int* bytesRead);
    bool write(int size, const uint8_t* buffer, int* bytesWritten);

    bool readAll(int size, uint8_t* buffer);
    bool writeAll(int size, const uint8_t* buffer);

    bool close();

    int getError() const;
};

}

#endif // TCPLIB_TCPSOCKET_H