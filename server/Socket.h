#ifndef TCPLIB_SOCKET_H
#define TCPLIB_SOCKET_H 1

#ifdef _WIN32
  #include <winsock2.h>
  #include <Ws2tcpip.h>
  #include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#else
  typedef int SOCKET;
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
#endif

#endif // TCPLIB_SOCKET_H