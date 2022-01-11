#ifndef TCPLIB_TCPSERVER_H
#define TCPLIB_TCPSERVER_H 1

#include "Socket.h"
#include "TcpSocket.h"

namespace tcplib {

class TcpServer {
private:
    SOCKET socketHandle;
    int error;

public:
    TcpServer();
    
    bool startServer(const char* hostname, int port);
    bool accept(TcpSocket* socket);
    bool close();

    int getError() const;
};

}

#endif // TCPLIB_TCPSERVER_H