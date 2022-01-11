#include "TcpServer.h"

#include <errno.h>

namespace tcplib {

namespace {
    inline int closeImpl(SOCKET handle) {
        int status = 0;

        #ifdef _WIN32
            status = shutdown(handle, SD_BOTH);
            if (status == 0) { status = closesocket(handle); }
        #else
            status = shutdown(handle, SHUT_RDWR);
            if (status == 0) { status = close(handle); }
        #endif

        return status;
    }

    inline int acceptImpl(SOCKET handle, struct sockaddr* address, socklen_t* addrlen) {
        return accept(handle, address, addrlen);
    }
}

TcpServer::TcpServer()
    : socketHandle(0),
      error(0) {

}

int TcpServer::getError() const {
    return error;
}

bool TcpServer::startServer(const char* ip, int port) {
    socketHandle = socket(AF_INET, SOCK_STREAM, 0);
    if(socketHandle < 0) {
        error = errno;
        return false;
    }

    int opt = 1;
    if (setsockopt(socketHandle, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        error = errno;
        return false;
    }

    struct sockaddr_in address;
    unsigned int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);

    if(bind(socketHandle, (struct sockaddr *)&address, addrlen) < 0) {
        error = errno;
        return false;
    }

    if(listen(socketHandle, 3) < 0) {
        error = errno;
        return false;
    }

    return true;
}

bool TcpServer::accept(TcpSocket* socket) {
    sockaddr addr;
    socklen_t addrlen;
    SOCKET handle = acceptImpl(socketHandle, &addr, &addrlen);
    if(handle < 0) {
        error = errno;
        return false;
    }
    *socket = TcpSocket(handle);
    return true;
}

bool TcpServer::close() {
    int success = closeImpl(socketHandle);
    if(success < 0) {
        error = errno;
        return false;
    }

    return true;
}

}