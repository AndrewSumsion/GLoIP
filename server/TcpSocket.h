#ifndef TCPLIB_TCPSOCKET_H
#define TCPLIB_TCPSOCKET_H 1

#include "Socket.h"

#include <cstdint>
using std::uint8_t;

#define IO(expr) if(!(expr)) return false

namespace tcplib {

class TcpSocket {
private:
    SOCKET socketHandle;
    int error;
    bool closed;

public:
    TcpSocket(SOCKET socketHandle);
    TcpSocket();

    void setHandle(SOCKET socketHandle);

    bool read(int size, uint8_t* buffer, int* bytesRead);
    bool write(int size, const uint8_t* buffer, int* bytesWritten);

    bool readAll(int size, uint8_t* buffer);
    bool writeAll(int size, const uint8_t* buffer);

    bool close();

    bool isClosed() const;
    int getError() const;
};

}

#endif // TCPLIB_TCPSOCKET_H