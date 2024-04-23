//
// Created by nanuns on 2021/7/8 0008.
//

#ifndef NANGBD_UDP_SERVER_H
#define NANGBD_UDP_SERVER_H

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

using namespace std;

class UdpServer
{
public:
    UdpServer();
    ~UdpServer();

    int init(const char* addr, int port);
    int send(const char *msg, int size, const struct sockaddr* remote_addr);
    int recv(char *msg, size_t max_size);
    int recv(char *msg, size_t max_size, int max_wait_ms);
    void uninit();

private:
    int              f_socket = -1;
    struct addrinfo* f_addrinfo = NULL;
};

#endif // NANGBD_UDP_SERVER_H
