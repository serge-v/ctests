#ifndef TESTS_SOCKETS_H_
#define TESTS_SOCKETS_H_

#include <sys/types.h>

#ifdef _MSC_VER
#include <ws2tcpip.h>
//#include <winsock.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

int create_tcp_socket(const char* address, int port, struct sockaddr_in* si, int* s);
int create_udp_socket(const char* address, int port, struct sockaddr_in* si, int* s);
int set_reuse_addr(int socket);
int set_multicast_if(int socket, const char* address, int port);
int set_rcv_timeout(int socket, int seconds, int microseconds);
int bind_address(int socket, const char* address, int port);
int add_membership(int socket, const char* address);
int set_nonblock(int socket);
int send_command(const char* address, int port, const char* command, int clients);

#endif // TESTS_SOCKETS_H_
