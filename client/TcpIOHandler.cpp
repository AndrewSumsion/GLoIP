#include "TcpIOHandler.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <netdb.h>

ssize_t readImpl(int fd, void* buf, size_t count) {
    ssize_t result = recv(fd, buf, count, 0);
    return result;
}

ssize_t writeImpl(int fd, const void* buf, size_t count) {
    ssize_t result = send(fd, buf, count, 0);
    return result;
}

int connectImpl(int fd, const sockaddr* addr, socklen_t addrlen) {
    int result = connect(fd, addr, addrlen);
    return result;
}

TcpIOHandler::TcpIOHandler(const char* hostname, int port)
    : hostname(hostname),
      port(port),
      fd(0),
      error(0) {

}

#define CHECK_RETURN(ret) if((ret) < 0) { error = errno; return false; }

bool TcpIOHandler::connect() {
    int success;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_RETURN(fd);
    sockaddr_in server = {};
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    success = inet_pton(AF_INET, hostname, &server.sin_addr);
    if(success == 0) {
        return false;
    }
    else CHECK_RETURN(success);

    success = connectImpl(fd, (sockaddr*)&server, sizeof(server));
    CHECK_RETURN(success);

    connected = true;
    return true;
}

bool TcpIOHandler::disconnect() {
    if(!connected) {
        return false;
    }
    int success = close(fd);
    CHECK_RETURN(success);

    connected = false;
    return true;
}

int TcpIOHandler::getError() {
    return error;
}

bool TcpIOHandler::read(int size, uint8_t *buffer, int* bytesRead) {
    if(!connected) {
        error = ENOTCONN;
        return false;
    }
    ssize_t result = readImpl(fd, buffer, size);
    CHECK_RETURN(result)
    if(result == 0) {
        // other end disconnected
        error = ENOTCONN;
        return false;
    }
    *bytesRead = result;
    return true;
}

bool TcpIOHandler::write(int size, const uint8_t *buffer, int* bytesWritten) {
    if(!connected) {
        error = ENOTCONN;
        return false;
    }
    ssize_t result = writeImpl(fd, buffer, size);
    CHECK_RETURN(result);
    *bytesWritten = result;
    return true;
}

