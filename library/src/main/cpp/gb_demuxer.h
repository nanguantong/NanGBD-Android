//
// Created by nanuns on 2021/7/8.
//

#ifndef NANGBD_GB_DEMUXER_H
#define NANGBD_GB_DEMUXER_H

#include "user_args.h"
#include "net/udp_server.h"

using namespace std;

class GB28181Demuxer {
public:
    GB28181Demuxer(UserArgs *arg);
    ~GB28181Demuxer();

public:
    /**
     * 开始音频解码
     * @return 0:成功
     */
    int startVoice(int cid, payload_type pt, bool is_udp, const char* remote_ip, int remote_port,
                   const char* local_ip, int local_port);

    /**
     * 结束音频解码
     */
    int endVoice(int cid);

private:
    static void* processVoice(void *obj);

private:
    class Server {
    public:
        int cid;
        payload_type pt;
        UdpServer udpServer;
        pthread_t thread_voice;
        volatile int is_running = STATE_STOPPED;
        char src_buf[64 * 1024];
        char dst_buf[64 * 1024];
    };

    UserArgs *args = NULL;
    std::map<int, Server*> servers;
};

#endif // NANGBD_GB_DEMUXER_H
