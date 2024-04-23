//
// Created by nanuns on 2021/7/8 0008.
//

#include <linux/in.h>
#include "../utils/log.h"
#include "../utils/util.h"
#include "udp_server.h"

/** \brief Initialize a UDP server object.
 *
 * This function initializes a UDP server object making it ready to
 * receive messages.
 *
 * The server address and port are specified in the constructor so
 * if you need to receive messages from several different addresses
 * and/or port, you'll have to create a server for each.
 *
 * The address is a string and it can represent an IPv4 or IPv6
 * address.
 *
 * Note that this function calls connect() to connect the socket
 * to the specified address. To accept data on different UDP addresses
 * and ports, multiple UDP servers must be created.
 *
 * \note
 * The socket is open in this process. If you fork() or exec() then the
 * socket will be closed by the operating system.
 *
 * \warning
 * We only make use of the first address found by getaddrinfo(). All
 * the other addresses are ignored.
 *
 * \exception udp_client_server_runtime_error
 * The udp_client_server_runtime_error exception is raised when the address
 * and port combinaison cannot be resolved or if the socket cannot be
 * opened.
 *
 * \param[in] addr  The address we receive on.
 * \param[in] port  The port we receive from.
 */
UdpServer::UdpServer()
{
    LOGD("UdpServer");
}

/** \brief Clean up the UDP server.
 *
 * This function frees the address info structures and close the socket.
 */
UdpServer::~UdpServer()
{
    LOGD("~UdpServer");
    uninit();
}

int UdpServer::init(const char* addr, int port)
{
    int ret;
    char decimal_port[16];
    snprintf(decimal_port, sizeof(decimal_port), "%d", port);
    decimal_port[sizeof(decimal_port) / sizeof(decimal_port[0]) - 1] = '\0';
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    ret = getaddrinfo(addr, decimal_port, &hints, &f_addrinfo);
    if (ret != 0 || f_addrinfo == NULL) {
        LOGE("invalid address or port");
        return ret;
    }
    f_socket = socket(f_addrinfo->ai_family, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
    if (f_socket == -1) {
        freeaddrinfo(f_addrinfo);
        LOGE("could not create UDP socket");
        return -1;
    }
    struct timeval timeout = {10, 0};
    ret = setsockopt(f_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
    if (ret != 0) {
        LOGW("set rcvo (%d, %d)", f_socket, ret);
    }
    ret = ::bind(f_socket, f_addrinfo->ai_addr, f_addrinfo->ai_addrlen);
    if (ret != 0) {
        freeaddrinfo(f_addrinfo);
        close_socket(f_socket);
        LOGE("could not bind UDP socket");
    }
    LOGI("init udp server socket %d port %d ok", f_socket, port);
    return ret;
}

int UdpServer::send(const char *msg, int size, const struct sockaddr* remote_addr) {
    if (f_socket < 0) {
        return -1;
    }
    return ::sendto(f_socket, msg, size, MSG_NOSIGNAL, remote_addr, SOCKIN_LEN);
}

/** \brief Wait on a message.
 *
 * This function waits until a message is received on this UDP server.
 * There are no means to return from this function except by receiving
 * a message. Remember that UDP does not have a connect state so whether
 * another process quits does not change the status of this UDP server
 * and thus it continues to wait forever.
 *
 * Note that you may change the type of socket by making it non-blocking
 * (use the get_socket() to retrieve the socket identifier) in which
 * case this function will not block if no message is available. Instead
 * it returns immediately.
 *
 * \param[in] msg  The buffer where the message is saved.
 * \param[in] max_size  The maximum size the message (i.e. size of the \p msg buffer.)
 *
 * \return The number of bytes read or -1 if an error occurs.
 */
int UdpServer::recv(char *msg, size_t max_size)
{
    if (f_socket < 0) {
        return -1;
    }
    return ::recv(f_socket, msg, max_size, 0);
}

/** \brief Wait for data to come in.
 *
 * This function waits for a given amount of time for data to come in. If
 * no data comes in after max_wait_ms, the function returns with -1 and
 * errno set to EAGAIN.
 *
 * The socket is expected to be a blocking socket (the default,) although
 * it is possible to setup the socket as non-blocking if necessary for
 * some other reason.
 *
 * This function blocks for a maximum amount of time as defined by
 * max_wait_ms. It may return sooner with an error or a message.
 *
 * \param[in] msg  The buffer where the message will be saved.
 * \param[in] max_size  The size of the \p msg buffer in bytes.
 * \param[in] max_wait_ms  The maximum number of milliseconds to wait for a message.
 *
 * \return -1 if an error occurs or the function timed out, the number of bytes received otherwise.
 */
int UdpServer::recv(char *msg, size_t max_size, int max_wait_ms)
{
    if (f_socket < 0) {
        return -1;
    }
    fd_set s;
    FD_ZERO(&s);
    FD_SET(f_socket, &s);
    struct timeval timeout;
    timeout.tv_sec = max_wait_ms / 1000;
    timeout.tv_usec = (max_wait_ms % 1000) * 1000;
    int retval = select(f_socket + 1, &s, &s, &s, &timeout);
    if (retval == -1) {
        LOGE("recv data timeout, %d", f_socket);
        // select() set errno accordingly
        return -1;
    }
    if (retval > 0) {
        // socket has data
        return ::recv(f_socket, msg, max_size, 0);
    }

    // socket has no data
    errno = EAGAIN;
    return -1;
}

void UdpServer::uninit()
{
    if (NULL != f_addrinfo) {
        freeaddrinfo(f_addrinfo);
        f_addrinfo = NULL;
    }
    close_socket(f_socket);
}