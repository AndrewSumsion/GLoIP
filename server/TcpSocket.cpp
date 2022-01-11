#include "TcpSocket.h"

#include "Socket.h"
#include <errno.h>

#include <cstdio>

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
}

TcpSocket::TcpSocket()
    : socketHandle(0),
      error(0) {

}

TcpSocket::TcpSocket(SOCKET socketHandle)
    : socketHandle(socketHandle),
      error(0) {

}

bool TcpSocket::read(int size, uint8_t* buffer, int* bytesRead) {
    ssize_t result = recv(socketHandle, buffer, size, 0);
    if(result < 0) {
        error = errno;
        return false;
    }
    *bytesRead = result;
    return true;
}

bool TcpSocket::write(int size, const uint8_t* buffer, int* bytesWritten) {
    ssize_t result = send(socketHandle, buffer, size, 0);
    if(result < 0) {
        error = errno;
        return false;
    }
    *bytesWritten = result;
    return true;
}

char* formatArray(int size, const uint8_t* buffer) {
    char* result = new char[size * 5 + 3]; // enough room for size 3-digit numbers separated by ", " surrounded by brackets
    int p = 0;
    result[p++] = '[';
    for(int i = 0; i < size; i++) {
        if(i == size - 1) {
            p += sprintf(result + p, "%i", buffer[i]);
        }
        else {
            p += sprintf(result + p, "%i, ", buffer[i]);
        }
    }
    result[p++] = ']';
    result[p] = '\0';
    return result;
}

bool TcpSocket::readAll(int size, uint8_t* buffer) {
    int totalBytesRead = 0;
    int bytesRead = 0;
    bool success = false;
    while(totalBytesRead < size) {
        success = read(size - totalBytesRead, buffer + totalBytesRead, &bytesRead);
        if(!success) {
            return false;
        }
        totalBytesRead += bytesRead;
    }

    char* formattedString = formatArray(size, buffer);
    printf("Read\t%d%s\n", size, formattedString);
    delete [] formattedString;

    return true;
}

bool TcpSocket::writeAll(int size, const uint8_t* buffer) {
    char* formattedString = formatArray(size, buffer);
    printf("Write\t%d%s\n", size, formattedString);
    delete [] formattedString;

    int totalBytesWritten = 0;
    int bytesWritten = 0;
    bool success = false;
    while(totalBytesWritten < size) {
        success = write(size - totalBytesWritten, buffer + totalBytesWritten, &bytesWritten);
        if(!success) {
            return false;
        }
        totalBytesWritten += bytesWritten;
    }

    return true;
}

bool TcpSocket::close() {
    int success = closeImpl(socketHandle);
    if(success < 0) {
        error = errno;
        return false;
    }

    return true;
}

int TcpSocket::getError() const {
    return error;
}

}