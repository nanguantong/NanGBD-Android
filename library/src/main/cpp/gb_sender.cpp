//
// Created by nanuns on 2018/9/13.
//

#include <vector>
#include <thread>
#include "gb_header.h"
#include "gb_sender.h"

//#define TCP_SEND_PER_FRAME

GB28181Sender::GB28181Sender(UserArgs *arg) : args(arg) {
    is_clearing = false;
    is_running = STATE_RUNNING;
    pthread_create(&thread_send, NULL, GB28181Sender::processSend, this);
}

GB28181Sender::~GB28181Sender() {
    closeSender();
}

int GB28181Sender::initSender(StreamSender *sender) {
    int ret = 0;
    if (sender->sockfd > 0) {
        LOGE("[sender]init sender is opened");
        return -1;
    }
    switch (sender->transport) {
        case PROTO_UDP:
            LOGI("ip:%s, server_port:%d, protocol:%d", sender->server_ip, sender->server_port, sender->transport);
            ret = init_socket(SOCK_DGRAM, sender->server_ip, sender->server_port, sender->local_port,
                              sender->sockfd, sender->client_addr, sender->server_addr);
            break;
        case PROTO_TCP:
            LOGI("ip:%s, server_port:%d, local_port:%d, protocol:%d",
                 sender->server_ip, sender->server_port, sender->local_port, sender->transport);
            ret = init_socket(SOCK_STREAM, sender->server_ip, sender->server_port, sender->local_port,
                              sender->sockfd, sender->client_addr, sender->server_addr);
            break;
        default: break;
    }
    LOGI("[sender]init local port %d %s %d %s", sender->local_port, ret == 0 ? "ok" : "failed",
         sender->sockfd, ret == 0 ? "" : strerror(ex_errno));
    return ret;
}

int GB28181Sender::addPacket(uint8_t *pkt) {
    int size = pkt_queue.size();
    // 判断添加的包是否被及时取出发送
    if (size > args->queue_max) {
        bool ret;
        int i = 0;
        uint8_t *pkt_buf;
        uint8_t keyframe = 0;
        int remove_size = size - args->queue_max, total_remove_size;

        is_clearing = true;
        while (is_running) {
            /**
             * 两种方法（方法1好）：
             *
             * 1、跳帧清除
             *   从remove_size开始向后找到第一个I帧
             *     如果remove_size位置就是I帧，则从头删除到remove_size-1之间的所有pkt；
             *     如果remove_size位置不是I帧，则向后继续找第一个I帧，找到则累加，如果没找到I帧则等break等下一次
             *         IPPPPIPPPP .IPPPP
             *         IPPPPIPP . PPIPPPP
             *                  0 12
             *                  1 23
             *
             * 2、逐帧清除
             *      (1) 从前往后找到第一个关键帧位置，如果找到，则其之前的帧都clear掉；否则break
             *      (2) 从前往后找到关键帧位置，如果找到，则其之前的帧都clear掉；
             *          比较此时queue大小，如果还是超过queue_max，则继续前面步骤，否则break
             *      (3) 从后往前找到第一个关键帧位置，如果找到，则其之前的帧都clear掉；否则break
             *
             *     (1) 处理点 IPPPPIPPPPIPPPP  直接break
             *     (2) PPPP 处理点 IPPPP       直接clear处理点前的帧
             *     (3) PP 处理点 PP
             *          处理点前的都clear，但是后面还没到的P帧需要等到了再clear，直到I帧停止
             *          在等到I帧之前不能发送P帧，可能发生马赛克
             */
            ret = pkt_queue.at(remove_size + i, pkt_buf);
            if (!ret) {
                break;
            }
            keyframe = pkt_buf[TS_LEN];
            if (keyframe) {
                break;
            }
            i++;
        }

        LOGW("[sender]queue size %d is too bigger, clearing %d + %d (%d)...", size, remove_size, i, keyframe);

        // clear掉从头起total_remove_size的所有pkt
        if (keyframe) {
            total_remove_size = remove_size + i - 1;
            while (is_running && total_remove_size-- > 0) {
                pkt_buf = *pkt_queue.try_pop();
                delete pkt_buf;
            }
            size -= total_remove_size;
        }
        is_clearing = false;
    }
    pkt_queue.push(pkt);
    return size + 1;
}

int GB28181Sender::sendRtpPacket(StreamSender *sender, const uint8_t *buf, uint32_t buf_len, int64_t ts, bool keyframe, bool is_tcp) {
    static uint8_t rtp_header[RTP_HDR_LEN];

    static uint32_t payload_len_udp = RTP_MAX_PACKET_BUFF - RTP_HDR_LEN;
    static uint8_t send_buf_udp[RTP_MAX_PACKET_BUFF];

#ifdef TCP_SEND_PER_FRAME
    static int payload_len_tcp = 1024*1024;
    static uint8_t send_buf_tcp[payload_len_tcp];
#else
    static uint32_t payload_len_tcp = RTP_MAX_PACKET_BUFF - RTP_TCP_HDR_LEN - RTP_HDR_LEN;
    static uint8_t send_buf_tcp[RTP_MAX_PACKET_BUFF];
#endif

    uint8_t* send_buf = is_tcp ? send_buf_tcp : send_buf_udp;
    uint32_t payload_len = is_tcp ? payload_len_tcp : payload_len_udp;
    ssize_t sent_size;
    uint32_t send_len;
    uint32_t total_payload_len = 0, total_sent_len = 0;

    if (is_tcp) {
#ifdef TCP_SEND_PER_FRAME
        gb_make_rtp_header(rtp_header, sender->payload_type, RTP_SEQ(sender->seq_num++), ts, sender->ssrc, RTP_PKT_END);
        memcpy(send_buf, short2Bytes(RTP_HDR_LEN + buf_len), RTP_TCP_HDR_LEN);
        memcpy(send_buf + RTP_TCP_HDR_LEN, rtp_header, RTP_HDR_LEN);
        memcpy(send_buf + RTP_TCP_HDR_LEN + RTP_HDR_LEN, buf, buf_len);
        send_len = RTP_TCP_HDR_LEN + RTP_HDR_LEN + buf_len;
        sent_size = sendData(sender, send_buf, send_len, keyframe);
        if (sent_size != send_len) {
            LOGE("tcp send error (%d, %d)!", sent_size, buf_len);
        }
        return sent_size;
#else
        if (buf_len <= payload_len) {
            gb_make_rtp_header(rtp_header, sender->payload_type, RTP_SEQ(sender->seq_num), ts, sender->ssrc, RTP_PKT_END);
            memcpy(send_buf, ushort2bytes(RTP_HDR_LEN + (uint16_t) buf_len), RTP_TCP_HDR_LEN);
            memcpy(send_buf + RTP_TCP_HDR_LEN, rtp_header, RTP_HDR_LEN);
            memcpy(send_buf + RTP_TCP_HDR_LEN + RTP_HDR_LEN, buf, buf_len);
            send_len = RTP_TCP_HDR_LEN + RTP_HDR_LEN + buf_len;
            sent_size = sendData(sender, send_buf, send_len, keyframe);
            if (sent_size != send_len) {
                LOGE("tcp send error (%zd, %d) seq %u", sent_size, buf_len, sender->seq_num);
            }
            sender->seq_num++;
            return sent_size;
        } else {
            uint32_t cur_payload_len = 0;
            int8_t marker;

            while (buf_len > 0) {
                if (buf_len <= payload_len) {
                    cur_payload_len = buf_len;
                    marker = RTP_PKT_END;
                } else {
                    cur_payload_len = payload_len;
                    marker = 0;
                }
                memcpy(send_buf, ushort2bytes(RTP_HDR_LEN + (uint16_t) cur_payload_len), RTP_TCP_HDR_LEN);
                gb_make_rtp_header(send_buf + RTP_TCP_HDR_LEN, sender->payload_type, RTP_SEQ(sender->seq_num), ts, sender->ssrc, marker);
                memcpy(send_buf + RTP_TCP_HDR_LEN + RTP_HDR_LEN, buf + total_payload_len, cur_payload_len);

                send_len = RTP_TCP_HDR_LEN + RTP_HDR_LEN + cur_payload_len;
                sent_size = sendData(sender, send_buf, send_len, keyframe);
                if (sent_size != send_len) {
                    LOGE("tcp send error (%zd, %d), marker %d seq %u", sent_size, cur_payload_len, marker, sender->seq_num);
                    if (sent_size < 0) {
                        return sent_size;
                    }
                }
                buf_len -= cur_payload_len;
                total_payload_len += cur_payload_len;
                total_sent_len += sent_size;
                sender->seq_num++;
            }
            return total_sent_len;
        }
#endif
    }

    total_payload_len = total_sent_len = 0;
    if (buf_len <= payload_len) {
        gb_make_rtp_header(rtp_header, sender->payload_type, RTP_SEQ(sender->seq_num), ts, sender->ssrc, RTP_PKT_END);
        memcpy(send_buf, rtp_header, RTP_HDR_LEN);
        memcpy(send_buf + RTP_HDR_LEN, buf, buf_len);
        send_len = RTP_HDR_LEN + buf_len;
        sent_size = sendData(sender, send_buf, send_len, keyframe);
        if (sent_size != send_len) {
            LOGE("udp send error (%zd, %d) seq %u", sent_size, buf_len, sender->seq_num);
        }
        sender->seq_num++;
        return sent_size;
    } else {
        int cur_payload_len;
        int8_t marker;

        while (buf_len > 0) {
            if (buf_len <= payload_len) {
                cur_payload_len = buf_len;
                marker = RTP_PKT_END;
            } else {
                cur_payload_len = payload_len;
                marker = 0;
            }
            gb_make_rtp_header(send_buf, sender->payload_type, RTP_SEQ(sender->seq_num), ts, sender->ssrc, marker);
            memcpy(send_buf + RTP_HDR_LEN, buf + total_payload_len, cur_payload_len);

            send_len = RTP_HDR_LEN + cur_payload_len;
            sent_size = sendData(sender, send_buf, send_len, keyframe);
            if (sent_size != send_len) {
                LOGE("udp send error (%zd, %d), marker %d seq %u", sent_size, cur_payload_len, marker, sender->seq_num);
                if (sent_size < 0) {
                    return sent_size;
                }
            }
            buf_len -= cur_payload_len;
            total_payload_len += cur_payload_len;
            total_sent_len += sent_size;
            sender->seq_num++;
        }
        return total_sent_len;
    }
}

void *GB28181Sender::processSend(void *obj) {
    GB28181Sender *gb_sender = (GB28181Sender *) obj;
    int64_t st = 0, t1 = 0, t2 = 0, st_upflow = 0, et_upflow = 0;
    ssize_t sent_size = 0;
    int count = 0;
    bool is_empty;
    std::vector<int> esenders;

    while (gb_sender->is_running) {
        if (DEBUG) {
            st = get_cur_time();
        }
        if (gb_sender->is_clearing) {
            continue;
        }
        uint8_t *pkt_buf = *gb_sender->pkt_queue.wait_and_pop();
        if (!gb_sender->is_running || pkt_buf == NULL) {
            delete pkt_buf;
            break;
        }

        int64_t pts = bytes2long(pkt_buf);
        uint8_t keyframe = pkt_buf[TS_LEN];
        uint8_t *buf = pkt_buf + TS_LEN + FRAME_FLAG_LEN;
        uint32_t buf_len = bytes2uint(buf);
        //pts = (count++) * gb_sender->scr_per_frame;

        if (DEBUG) {
            t1 = get_cur_time();
        }

        if (st_upflow == 0) {
            st_upflow = get_cur_time();
        }

        gb_sender->args->mtx_sender->lock();
        for (auto it = gb_sender->args->senders->begin(); it != gb_sender->args->senders->end();) {
            StreamSender *sender = &it->second;
            if (sender->drop_frame && !keyframe) {
                LOGW("drop seq %d, pts %lld, len %d, %d", sender->seq_num, pts, buf_len, sender->sockfd);
                it++;
                continue;
            }
            sender->drop_frame = false;
            switch (sender->transport) {
                case PROTO_UDP:
                    sent_size = gb_sender->sendRtpPacket(sender, buf + DATA_LEN, buf_len, pts, keyframe, false);
                    LOGD("[sender][udp]get pkt buf len %d, sent %ld", buf_len, sent_size);
                    break;
                case PROTO_TCP:
                    sent_size = gb_sender->sendRtpPacket(sender, buf + DATA_LEN, buf_len, pts, keyframe, true);
                    LOGD("[sender][tcp]get pkt buf len %d, sent %ld", buf_len, sent_size);
                    break;
                default: break;
            }
            if (sent_size < 0) {
                esenders.push_back(it->first);
                gb_sender->args->senders->erase(it++);
            }
            else {
                it++;
            }
        }
        is_empty = gb_sender->args->senders->empty();
        gb_sender->args->mtx_sender->unlock();

        // 这里放到线程队列执行, 防止阻塞此线程
        if (!gb_sender->is_running) {
            delete pkt_buf;
            break;
        }

        if (gb_sender->args->stream_stat_period > 0 && gb_sender->total_sent_size > 0) {
            et_upflow = get_cur_time();
            if (et_upflow > st_upflow + gb_sender->args->stream_stat_period * 1000) {
                uint64_t up_kbps = gb_sender->total_sent_size * 1000 / (et_upflow - st_upflow);
                char str[128];
                LOGD("[sender]up flow %llu kB/s", up_kbps);
                snprintf(str, sizeof(str) - 1, "{\"upFlow\":%llu}", up_kbps);
                onCallbackStatus(EVENT_STATISTICS, 0, str, strlen(str));
                st_upflow = et_upflow;
                gb_sender->total_sent_size = 0;
            }
        }

        if (!esenders.empty()) {
            std::thread t([esenders, is_empty]() {
                onCallsStop(esenders, false, is_empty);
            });
            t.detach();
            esenders.clear();
        }

        delete pkt_buf;

        if (DEBUG) {
            t2 = get_cur_time();
            LOGD("[sender]read from queue:%d I/O wait time: %lld, send time: %lld",
                 gb_sender->pkt_queue.size(), t1 - st, t2 - t1);
        }
    }
    LOGI("[sender]send is over");
    return NULL;
}

int GB28181Sender::closeSender() {
    if (is_running == STATE_STOPPED) {
        return 0;
    }
    is_running = STATE_STOPPED;
    return closeSenderInternal();
}

int GB28181Sender::closeSenderInternal() {
    LOGI("[sender]start close sender, queue size: %d", pkt_queue.size());

    while (!pkt_queue.empty()) {
        uint8_t *buf = *pkt_queue.try_pop();
        delete buf;
    }
    pkt_queue.push(NULL);
    pthread_join(thread_send, NULL);
    pkt_queue.clear();

    LOGI("[sender]close sender over");
    return 0;
}

ssize_t GB28181Sender::sendData(StreamSender *sender, const uint8_t *buf, int len, bool keyframe) {
    int ret, sent_len = 0, remaining_len = len;

    while (sent_len < len) {
        if (sender->sockfd < 0) {
            return -1;
        }
        switch (sender->transport) {
            case PROTO_UDP:
                ret = sendto(sender->sockfd, buf + sent_len, remaining_len, MSG_NOSIGNAL, (const sockaddr *) &sender->server_addr, SOCKIN_LEN);
                break;
            case PROTO_TCP:
                ret = send(sender->sockfd, buf + sent_len, remaining_len, MSG_NOSIGNAL);
                break;
            default: return -1;
        }

        if (ret == 0) {
            LOGE("socket remote eof (%d, %d, %d, %d, %s)", sender->sockfd, ret, len, keyframe, strerror(ex_errno));
            //close_socket(sender->sockfd); ret = -1;
            break;
        } else if (ret < 0) {
            int val = ex_errno;
            LOGE("send error (%d, %d, %d, %d, %s)", sender->sockfd, ret, len, keyframe, strerror(val));

            // wait or drop frame
            if (is_wouldblock_error(val)) {
                struct timeval tv;
                fd_set wrset;

                tv.tv_sec = SOCKET_TIMEOUT / 1000;
                tv.tv_usec = (SOCKET_TIMEOUT % 1000) * 1000;
                if (tv.tv_usec == 0)
                    tv.tv_usec += 10000;

                FD_ZERO(&wrset);
                FD_SET(sender->sockfd, &wrset);
                ret = select(sender->sockfd + 1, NULL, &wrset, NULL, &tv);
                if (ret > 0) {
                    continue;
                } else if (ret < 0) {
                    LOGE("select %d error: %s", sender->sockfd, strerror(ex_errno));
                    return -1;
                }
                LOGE("select %d timeout: %d ms\n", sender->sockfd, SOCKET_TIMEOUT);
                continue;
            }

            /* NETWORK_ERROR; */
            return -1;
#if 0
            // tcp可能被对端关闭了需重连, 或者发送失败重试
            while (++sender->retries <= MAX_RESEND_COUNT) {
                close_socket(sender->sockfd);
                ret = init_socket(sender->transport == PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM,
                                  sender->server_ip, sender->server_port, sender->local_port,
                                  sender->sockfd, sender->client_addr, sender->server_addr);
                if (ret == 0) {
                    // 丢弃非关键帧
                    if (!keyframe) {
                        ret = len;
                        sender->drop_frame = true;
                        break;
                    }
                    ret = sendData(sender, buf, len, keyframe);
                    if (ret > 0) {
                        break;
                    }
                }
                LOGI("send retry %d %s, %d %s", sender->retries, ret < 0 ? "failed" : "ok", ret, strerror(ex_errno));
                if (ret >= 0) {
                    break;
                }
            }
            LOGI("connect %d retry %d, drop frame %d, ret %d", sender->sockfd, sender->retries, sender->drop_frame, ret);
            if (sender->retries > MAX_RESEND_COUNT) {
                ret = -1;
            }
#endif
        } else {
            sent_len += ret;
            if (ret < remaining_len) {
                remaining_len -= ret;
                continue;
            }
        }
        break;
    }
    if (sent_len > 0) {
        total_sent_size += sent_len;
    }
    return sent_len;
}
