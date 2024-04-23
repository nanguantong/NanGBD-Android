//
// Created by nanuns on 2018/9/13.
//

#ifndef NANGBD_GB_SENDER_H
#define NANGBD_GB_SENDER_H

#include "utils/util.h"
#include "user_args.h"

class GB28181Sender {
public:
    GB28181Sender(UserArgs *arg);
    ~GB28181Sender();

    int initSender(StreamSender *sender);
    int addPacket(uint8_t *pkt);
    int closeSender();

private:
    static void* processSend(void *obj);

    int sendRtpPacket(StreamSender *sender, const uint8_t *buf, uint32_t buf_len, int64_t ts, bool keyframe, bool is_tcp);
    ssize_t sendData(StreamSender *sender, const uint8_t *buf, int len, bool keyframe);

    int closeSenderInternal();

private:
    UserArgs* args = NULL;
    AVQueue<uint8_t *> pkt_queue;
    volatile int is_running = STATE_STOPPED;
    pthread_t thread_send;
    std::atomic_bool is_clearing;

    uint64_t total_sent_size = 0;
};

#endif // NANGBD_GB_SENDER_H
