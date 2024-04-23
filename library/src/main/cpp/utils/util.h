//
// Created by nanuns on 2020/6/7.
//

#ifndef NANGBD_UTIL_H
#define NANGBD_UTIL_H

#ifdef WIN32
#include <windows.h>
#include <fileapi.h>
#define _access access
#else
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
// http://c.biancheng.net/cpp/html/238.html
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/file-constants
#include <fcntl.h>
#include "queue.cpp"
#include "log.h"

#if __ANDROID_API__ < 24
#include "ifaddrs.h"
#else
#include <ifaddrs.h>
#endif

#if defined(WIN32) || defined(_WIN32_WCE)
#define ex_errno WSAGetLastError()
#define is_wouldblock_error(r) ((r) == WSAEINTR || (r) == WSAEWOULDBLOCK)
#define is_connreset_error(r) ((r) == WSAECONNRESET || (r) == WSAECONNABORTED || (r) == WSAETIMEDOUT || (r) == WSAENETRESET || (r) == WSAENOTCONN)
#else
#define ex_errno errno
#endif
#ifndef is_wouldblock_error
#define is_wouldblock_error(r) ((r) == EINTR || (r) == EWOULDBLOCK || (r) == EAGAIN)
#define is_connreset_error(r) ((r) == ECONNRESET || (r) == ECONNABORTED || (r) == ETIMEDOUT || (r) == ENETRESET || (r) == ENOTCONN)
#endif

#ifndef SOCKET_TIMEOUT
/* when stream has sequence error: */
/* having SOCKET_TIMEOUT > 0 helps the system to recover */
#define SOCKET_TIMEOUT 0
#endif

#define STATE_STOPPED 0
#define STATE_RUNNING 1

#define null_if_empty(s) (((s) != NULL && (s)[0] !='\0') ? (s) : NULL)

using namespace std;

static int SOCKIN_LEN = sizeof(struct sockaddr_in);

static int random(int from, int to) {
    return rand() % (to - from + 1) + from;
}

static uint64_t bytes2long(uint8_t b[]) {
    uint64_t temp = 0;
    uint64_t res = 0;
    for (int i = 0; i < sizeof(uint64_t); i++) {
        res <<= 8;
        temp = b[i];
        temp = temp & 0xff;
        res |= temp;
    }
    return res;

    // return ((uint64_t)(ptr[0]) << 56) + ((uint64_t)(ptr[1]) << 48) + ((uint64_t)(ptr[2]) << 40) + ((uint64_t)(ptr[3]) << 32) +
    //        ((uint64_t)(ptr[4]) << 24) + ((uint64_t)(ptr[5]) << 16) + ((uint64_t)(ptr[6]) << 8)  + (uint64_t)(ptr[7]);
}

static uint8_t* long2bytes(uint64_t val) {
    uint8_t* bytes = (uint8_t *) malloc(sizeof(uint64_t));
    bytes[0] = (uint8_t)(val >> 56);
    bytes[1] = (uint8_t)(val >> 48);
    bytes[2] = (uint8_t)(val >> 40);
    bytes[3] = (uint8_t)(val >> 32);
    bytes[4] = (uint8_t)(val >> 24);
    bytes[5] = (uint8_t)(val >> 16);
    bytes[6] = (uint8_t)(val >> 8);
    bytes[7] = (uint8_t)val;
    return bytes;
}

static uint16_t bytes2ushort(uint8_t b[]) {
    uint16_t temp = 0;
    uint16_t res = 0;
    for (int i = 0; i < sizeof(uint16_t); i++) {
        res <<= 8;
        temp = b[i];
        temp = (uint16_t) (temp & 0xff);
        res |= temp;
    }
    return res;

    // return ((uint16_t)(b[0]) << 8) + (uint16_t)(b[1]);
}

static uint8_t* ushort2bytes(uint16_t val) {
    uint8_t* bytes = (uint8_t *) malloc(sizeof(uint16_t));
    bytes[0] = (uint8_t) ((val >> 8) & 0xff);
    bytes[1] = (uint8_t) (val & 0xff);
    return bytes;
}

static uint32_t bytes2uint(uint8_t b[]) {
    return ((uint32_t)(b[0]) << 24) + ((uint32_t)(b[1]) << 16) + ((uint32_t)(b[2]) << 8) + (uint32_t)(b[3]);
}

static uint8_t* uint2bytes(uint32_t val) {
    uint8_t* bytes = (uint8_t *) malloc(sizeof(uint32_t));
    bytes[0] = (uint8_t)(val >> 24);
    bytes[1] = (uint8_t)(val >> 16);
    bytes[2] = (uint8_t)(val >> 8);
    bytes[3] = (uint8_t)val;
    return bytes;
}

static uint32_t reverse_bytes(uint32_t value) {
    return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 |
           (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
}

static int64_t get_cur_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static char* get_time(char buf[], int size) {
    struct timeval tv;
    struct tm* tm;
    if (gettimeofday(&tv, NULL) == -1) {
        return NULL;
    }
    if ((tm = localtime((const time_t*)&tv.tv_sec)) == NULL) {
        return NULL;
    }
    snprintf(buf, size, "%d-%02d-%02d %02d:%02d:%02d.%03d",
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec,
             (int)(tv.tv_usec / 1000));
    buf[size - 1] = 0;
    return buf;
}

static char* get_time_with_t(char buf[], int size) {
    struct timeval tv;
    struct tm* tm;
    if (gettimeofday(&tv, NULL) == -1) {
        return NULL;
    }
    if ((tm = localtime((const time_t*)&tv.tv_sec)) == NULL) {
        return NULL;
    }
    snprintf(buf, size, "%d-%02d-%02dT%02d:%02d:%02d",
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec);
    buf[size - 1] = 0;
    return buf;
}

static char* get_ip() {
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char *host = NULL;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return NULL;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        if (!strcmp(ifa->ifa_name, "lo"))
            continue;
        if (family == AF_INET) {
            if ((host = (char*)malloc(NI_MAXHOST)) == NULL)
                return NULL;
            s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ?
                            sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                return NULL;
            }
            freeifaddrs(ifaddr);
            return host;
        }
    }
    return NULL;
}

static int close_socket(int& sockfd) {
    if (sockfd == -1) {
        return 0;
    }
    close(sockfd);
    sockfd = -1;
    return 0;
}

static int init_socket(int proto, const char* server_ip, int server_port,
                       int local_port, int& client_sockfd,
                       struct sockaddr_in& client_addr, struct sockaddr_in& server_addr) {
    int ret;
    int sockfd;

    switch (proto) {
        case SOCK_DGRAM:
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            break;
        case SOCK_STREAM:
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            break;
        default:
            sockfd = -1;
    }

    if (sockfd < 0) {
        LOGE("ERROR opening socket (%d)", sockfd);
        return -1;
    }

    bzero((char *) &server_addr, SOCKIN_LEN);
    server_addr.sin_family = AF_INET;
    inet_aton(server_ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(server_port);

    if (local_port > 0) {
        int flag = 1, max_send_buf = 2*1024*1024;
        struct timeval timeout = {3, 0};

        bzero((char *) &client_addr, SOCKIN_LEN);
        client_addr.sin_family = AF_INET;
        client_addr.sin_addr.s_addr = INADDR_ANY;
        client_addr.sin_port = htons(local_port);

#ifdef __linux__
        signal(SIGPIPE, SIG_IGN);
#endif
        ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&max_send_buf, sizeof(int));
        if (ret != 0) LOGW("set sbuf (%d, %d)", sockfd, ret);
        ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(int));
        if (ret != 0) LOGW("set reuseaddr (%d, %d)", sockfd, ret);

        if (proto == SOCK_STREAM) {
            ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,  (const char*)&flag, sizeof(int));
            if (ret != 0) LOGW("set tcp nodelay (%d, %d)", sockfd, ret);
        }
        ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
        if (ret != 0) LOGW("set sndo (%d, %d)", sockfd, ret);
        ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
        if (ret != 0) LOGW("set rcvo (%d, %d)", sockfd, ret);

        ret = ::bind(sockfd, (struct sockaddr *) &client_addr, SOCKIN_LEN);
        if (ret < 0) {
            LOGE("ERROR local bind (%d, %d)", sockfd, ret);
            close_socket(sockfd);
            return -1;
        }
    }
    if (proto == SOCK_STREAM) {
        ret = connect(sockfd, (const sockaddr *) &server_addr, SOCKIN_LEN);
        if (ret < 0) {
            LOGE("ERROR connect (%d, %d)", sockfd, ret);
            close_socket(sockfd);
            return -1;
        }
    }

    client_sockfd = sockfd;

    return 0;
}

#ifdef WIN32
static int gettid() {
    return GetCurrentThreadId();
}
#endif

static int write_file(const char *file, char *buf, size_t len) {
    int fd = open(file, O_CREAT | O_RDWR, 0666);
    if (fd == -1)
        return -1;
    int offset = write(fd, buf, len);
    if (offset == 0)
        return -2;
    close(fd);
    return 0;
}

static int read_file(const char *file, char *buf, size_t len) {
    int fd = open(file, O_CREAT | O_RDWR, 0666);
    if (fd == -1)
        return -1;
    int offset = read(fd, buf, len);
    if (offset == 0)
        return -2;
    close(fd);
    return 0;
}

static int mkdir_r_port(const char *const dir)
{
#ifdef _WIN32
    if (!CreateDirectoryA(dir, NULL)) {
        switch (GetLastError()) {
            case ERROR_ALREADY_EXISTS:
                break;
            default:
                return -1;
        }
    }
#else
    if (!access(dir, 0)) {
        struct stat sb;

        if (stat(dir, &sb))
            return -1;
        if (!(sb.st_mode & S_IFDIR)) {
            errno = ENOTDIR;
            return -1;
        }
    } else if (mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        return -1;
#endif
    return 0;
}

static int is_dir_exist(const char *const dir) {
    if (access(dir, 0) != 0) {
        return -1;
    }

    struct stat s;
    if (stat(dir, &s) != 0)
        return -1;
    if (!S_ISDIR(s.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }
    return 0;
}

static int mkdir_r(const char *const path)
{
    int ret = -1;
    char *dup = NULL;
    char *c;
    const char *dir;
    int exit_fl = 0;
    size_t sz;

    if (!path || !*path) {
        errno = EINVAL;
        goto exited;
    }

    sz = strlen(path) + 1;
    dup = (char *)malloc(sz);
    if (!dup) {
        errno = ENOMEM;
        goto exited;
    }

    memcpy(dup, path, sz);
    exit_fl = 0;

    for (c = dup, dir = dup; !exit_fl; c++) {
        switch (*c) {
            case '\0':
                exit_fl = 1;
                /* Fall through. */
            case '/': {
                const char cb = *c;

                *c = '\0';

                if (!*dir)
                    /* Path starting with delimiter character. */
                    ;
                else if (mkdir_r_port(dir))
                    goto exited;

                dir = dup;
                *c = cb;
            }
            break;
        }
    }

    ret = 0;

exited:
    if (dup)
        free(dup);

    return ret;
}

/**
 * 获取路径下的所有文件
 *
 * @param file_path
 * @param secrecy
 * @param type
 * @return
 */
static int find_files(const char *file_path, int secrecy, const char *type)
{
    if (is_dir_exist(file_path) != 0) {
        return -1;
    }
    // TODO
}

/**
 * 获取路径下指定条件的文件
 *
 * @param file_path
 * @param start_time
 * @param end_time
 * @param secrecy
 * @param type
 * @return
 */
static int find_files(const char *file_path, const char *start_time, const char *end_time, int secrecy, const char *type)
{
    if (is_dir_exist(file_path) != 0) {
        return -1;
    }
    // TODO
}

static char* del_char(char *str, char ch)
{
    int i = 0, j = 0;
    while (str[i] != '\0') {
        if (str[i] != ch) {
            str[j++] = str[i];
        }
        i++;
    }
    str[j] = '\0';
    return str;
}

#endif // NANGBD_UTIL_H
