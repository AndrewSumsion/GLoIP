#ifndef IO_HANDLER_H
#define IO_HANDLER_H 1

#include <pthread.h>

#include <cstdint>
using std::uint8_t;
using std::uint16_t;

#define IO(expr) if(!(expr)) return false;
#define IOHANDLER(expr, handler) do { if(!(expr)) { handler; return false; } } while(0)

class IOHandler {
private:
    const static bool isBigEndian = false;
    pthread_mutex_t readMutex;
    pthread_mutex_t writeMutex;
public:
    virtual bool write(int size, const uint8_t* buffer, int* bytesWritten) = 0;
    virtual bool read(int size, uint8_t* buffer, int* bytesRead) = 0;
    virtual bool connect() = 0;
    virtual bool disconnect() = 0;
    virtual int getError() = 0;

    IOHandler();
    ~IOHandler();

    bool writeAll(int size, const uint8_t* buffer);
    bool readAll(int size, uint8_t* buffer);
};

#endif // IO_HANDLER_H