//
// Created by nanuns on 2021/7/8.
//

#include <thread>
#include "utils/util.h"
#include "g711_coder.h"
#include "gb_header.h"
#include "gb_demuxer.h"

GB28181Demuxer::GB28181Demuxer(UserArgs *arg) : args(arg) {
}

GB28181Demuxer::~GB28181Demuxer() {
    for (auto &it : servers) {
        Server *server = it.second;
        if (server->is_running == STATE_RUNNING) {
            server->is_running = STATE_STOPPED;
            server->udpServer.uninit();
            pthread_join(server->thread_voice, NULL);
        }
        delete server;
    }
}

int GB28181Demuxer::startVoice(int cid, payload_type pt, bool is_udp,
                               const char* remote_ip, int remote_port,
                               const char* local_ip, int local_port) {
    int ret = -1;
    const auto& it = servers.find(cid);
    if (it != servers.end()) {
        return ret;
    }

    LOGI("[demuxer]start voice %d, remote %s:%d, local %s:%d",
         cid, remote_ip, remote_port, local_ip, local_port);

    if (pt != payload_type::PT_PCMA && pt != payload_type::PT_PCMU && pt != payload_type::PT_PS) {
        LOGE("unsupported voice payload type %d", pt);
        return ret;
    }

    if (is_udp) {
        Server* server = new Server();
        ret = server->udpServer.init(local_ip, local_port);
        if (0 != ret) {
            delete server;
            return ret;
        }

        servers.emplace(cid, server);
        server->cid = cid;
        server->pt = pt;
        server->is_running = STATE_RUNNING;
        pthread_create(&server->thread_voice, NULL, processVoice, server);

        struct sockaddr_in remote_addr;
        bzero((char *) &remote_addr, SOCKIN_LEN);
        remote_addr.sin_family = AF_INET;
        inet_aton(remote_ip, &remote_addr.sin_addr);
        remote_addr.sin_port = htons(remote_port);

        UdpServer* udpServer = &server->udpServer;
        std::thread t([udpServer, remote_addr]() {
            int i = 0;
            while (udpServer->send("voice", 5, (sockaddr *) &remote_addr) > 0) {
                LOGD("send udp voice packet");
                if (i++ > 5) {
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                }
            }
            LOGI("send udp voice packet end");
        });
        t.detach();
    } else {
        // TODO
        return ret;
    }
    return 0;
}

int GB28181Demuxer::endVoice(int cid) {
    int ret = -1;
    const auto& it = servers.find(cid);
    if (it == servers.end()) {
        return ret;
    }
    LOGI("[demuxer]end voice %d", cid);
    Server* server = it->second;
    if (server->is_running == STATE_RUNNING) {
        server->is_running = STATE_STOPPED;
        server->udpServer.uninit();
        pthread_join(server->thread_voice, NULL);
    }
    delete server;
    servers.erase(it);
    LOGI("[demuxer]end voice %d ok", cid);
    return 0;
}

void* GB28181Demuxer::processVoice(void *obj) {
    int ret = -1, retries = 0;
    Server* server = (Server *) obj;

    LOGI("[demuxer]process voice");
    while (server->is_running) {
        ret = server->udpServer.recv(server->src_buf, sizeof(server->src_buf));
        if (ret <= 0) {
            LOGE("udp recv data error: %d", ret);
            retries++;
            if (retries >= 5) {
                break;
            }
            continue;
        }
        retries = 0;
        LOGD("udp recv size %d", ret);
        // 修复AudioTrack音频长度不对时出现: "control flow integrity check for type 'android::AMessage'"
        if (ret >= sizeof(server->src_buf)) {
            continue;
        }

        // 解析语音数据(pcma/pcmu 或者 ps)
        if (server->pt == payload_type::PT_PCMA || server->pt == payload_type::PT_PCMU) {
            // 去掉rtp头
            int rtp_h_len = gb_parse_rtp_packet((uint8_t *) server->src_buf, ret);
            if (rtp_h_len <= 0) {
                continue;
            }
            ret -= rtp_h_len;

            ret = g711_decode(&server->src_buf[rtp_h_len], server->dst_buf, ret,
                              server->pt == payload_type::PT_PCMA ? G711_A_LAW : G711_U_LAW);
        } else if (server->pt == payload_type::PT_PS) {
            // TODO
        }
        if (ret > 0) {
            ret = onCallbackStatus(EVENT_TALK_AUDIO_DATA, server->cid, server->dst_buf, ret);
        }
    }
    LOGI("[demuxer]process voice end");

    // 可能由于上级关闭了语音流传输通道,下级需要主动关闭语音并回收task
    if (server->is_running) {
        server->is_running = STATE_STOPPED;
        onCallStop(server->cid, true, true);
    }
    return NULL;
}
