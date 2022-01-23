#include "IOHandler.h"

#include <cstdio>

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

bool IOHandler::writeAll(int size, const uint8_t* buffer) {
    //char* formattedString = formatArray(size, buffer);
    //printf("Write\t%d%s\n", size, formattedString);
    //delete [] formattedString;

    pthread_mutex_lock(&writeMutex);

    int bytesWritten = 0;
    int totalBytesWritten = 0;
    while(totalBytesWritten < size) {
        bool success = write(size - totalBytesWritten, buffer + totalBytesWritten, &bytesWritten);
        if(!success) {
            return false;
        }
        totalBytesWritten += bytesWritten;
    }
    pthread_mutex_unlock(&writeMutex);

    return true;
}

bool IOHandler::readAll(int size, uint8_t* buffer) {
    pthread_mutex_lock(&readMutex);
    int bytesRead = 0;
    int totalBytesRead = 0;
    while(totalBytesRead < size) {
        bool success = read(size - totalBytesRead, buffer + totalBytesRead, &bytesRead);
        if(!success) {
            return false;
        }
        totalBytesRead += bytesRead;
    }
    pthread_mutex_unlock(&readMutex);

    //char* formattedString = formatArray(size, buffer);
    //printf("Read\t%d%s\n", size, formattedString);
    //delete [] formattedString;
    return true;
}

bool IOHandler::isConnected() const {
    return connected;
}

IOHandler::IOHandler()
    : connected(false) {
    threadHandle = pthread_self();
    pthread_mutex_init(&readMutex, nullptr);
    pthread_mutex_init(&writeMutex, nullptr);
}

IOHandler::~IOHandler() {
    pthread_mutex_destroy(&readMutex);
    pthread_mutex_destroy(&writeMutex);
}