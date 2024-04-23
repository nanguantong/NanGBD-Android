//
// Created by nanuns on 2018/8/1.
//

#include <pthread.h>
#include <libyuv.h>
#include "utils/log.h"
#include "utils/bits.h"
#include "osd/osd.h"
#include "ua/gm_ssl.h"
#include "ua/gm_codec.h"
#include "g711_coder.h"
#include "gb_header.h"
#include "gb_muxer.h"

using namespace libyuv;

#define TEST_VBR 0

static AVRational device_time_base = { 1, AV_TIME_BASE };

extern ua_config_t* g_config_ua;
extern gm_t* g_gm;
extern osd_info_t g_osd_info;

#define LOG_BUF_SIZE	1024
void ff_logoutput(void* ptr, int level, const char* fmt, va_list vl) {
    static char buf[LOG_BUF_SIZE];
    //vsnprintf(buf, LOG_BUF_SIZE, fmt, vl);
    //LOGI("==== %s", buf);
}

/**
 * https://blog.csdn.net/changshulin0906/article/details/111664001 rtp荷载ps流
 * https://www.jianshu.com/p/2665a83f0cf2 谈谈关于Android视频编码的那些坑
 * @param arg
 */
GB28181Muxer::GB28181Muxer(UserArgs *arg) : args(arg) {
    av_register_all();
    av_log_set_level(AV_LOG_ERROR);
    av_log_set_callback(ff_logoutput);
    srand(time(0));
}

GB28181Muxer::~GB28181Muxer() {
    setArgs(NULL);
}

UserArgs* GB28181Muxer::getArgs() {
    return args;
}

void GB28181Muxer::setArgs(UserArgs* args) {
    if (this->args != NULL) {
        this->args->mtx_sender->lock();
        for (auto &it : *this->args->senders) {
            StreamSender *sender = &it.second;
            close_socket(sender->sockfd);
        }
        delete this->args->senders;
        this->args->mtx_sender->unlock();
        delete this->args->mtx_sender;
        if (this->args->fout != NULL) {
            fclose(this->args->fout);
            this->args->fout = NULL;
        }
        free(this->args->media_path);
        free(this->args);
        this->args = NULL;
    }
    if (args != NULL) {
        this->args = args;
    }
}

int GB28181Muxer::initMuxer() {
    int ret = 0;
    int64_t bitrate;
    AVDictionary *param = nullptr;
    LOGI("[muxer]init starting, tid:%d", gettid());

    if (args->v_encodec != VideoCodec::NONE) {
        fmt_ctx = avformat_alloc_context();
        video_st = avformat_new_stream(fmt_ctx, 0);

        if (video_st == NULL) {
            LOGE("video stream null");
            ret = -1;
            goto err;
        }

        g711a_frame_len = args->a_frame_len / 2;
        scr_per_frame = 90000 / args->frame_rate;
        max_rate = args->v_bitrate * (1 + args->v_bitrate_factor);
        min_rate = args->v_bitrate * (1 - args->v_bitrate_factor);

        bitrate = args->v_bitrate + (args->enable_audio ? g711a_frame_len : 0);
        bitrate += bitrate / 20 + 10000;
        mux_rate = (bitrate + (8 * 50) - 1) / (8 * 50);
        if (mux_rate >= (1<<22)) {
            LOGW("mux rate %d is too large", mux_rate);
            mux_rate = (1<<22) - 1;
        }

        codec_ctx = video_st->codec;
        switch (args->v_encodec) {
            case VideoCodec::H264: codec_ctx->codec_id = AV_CODEC_ID_H264; break;
            case VideoCodec::H265: codec_ctx->codec_id = AV_CODEC_ID_HEVC; break;
        }
        codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
        codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
        if (args->v_rotate ==  kRotate0 || args->v_rotate == kRotate180) {
            codec_ctx->width = args->out_width;
            codec_ctx->height = args->out_height;
        } else {
            codec_ctx->width = args->out_height;
            codec_ctx->height = args->out_width;
        }

        LOGI("in %dx%d out %dx%d codec %dx%d vid:%d, profile:%d, bitrate:%lld mode %d mrate:%d, "
             "framerate:%d, gop:%d, tnum:%d, rotate:%d, enable audio %d, max queue %d",
             args->in_width, args->in_height, args->out_width, args->out_height,
             codec_ctx->width, codec_ctx->height, codec_ctx->codec_id,
             args->v_profile, args->v_bitrate, args->v_bitrate_mode, mux_rate,
             args->frame_rate, args->gop, args->thread_num, args->v_rotate, args->enable_audio, args->queue_max
        )

        // https://blog.csdn.net/shiqian1022/article/details/88390108 CRF指南（x264 和 x265 中的固定码率因子）
        // https://blog.csdn.net/keen_zuxwang/article/details/71439387 x264 ffmpeg编解码参数笔记
        // https://blog.csdn.net/timor_lxl/article/details/109670499 FFmpeg编码的码率控制
        codec_ctx->bit_rate = args->v_bitrate;
        codec_ctx->gop_size = args->gop;
        codec_ctx->framerate.num = args->frame_rate;
        codec_ctx->framerate.den = 1;
        codec_ctx->time_base.num = 1;
        codec_ctx->time_base.den = args->frame_rate;
        codec_ctx->max_b_frames = 0;
        codec_ctx->qmin = 10;
        codec_ctx->qmax = 30; //51;

        if (codec_ctx->codec_id == AV_CODEC_ID_H264 || codec_ctx->codec_id == AV_CODEC_ID_HEVC) {
            char *profile = (char *) "baseline";
            int hw_profile = 1, hw_level = 0x200;
            if (codec_ctx->codec_id == AV_CODEC_ID_HEVC) {
                profile = (char *) "main";
            } else {
                switch (args->v_profile) {
                    case VideoProfile::PROFILE_BASELINE:
                        codec_ctx->profile = 66; hw_profile = 1; hw_level = 0x100; profile = (char *) "baseline";
                        break;
                    case VideoProfile::PROFILE_MAIN:
                        codec_ctx->profile = 67; hw_profile = 2; profile = (char *) "main";
                        break;
                    case VideoProfile::PROFILE_HIGH:
                        codec_ctx->profile = 100; hw_profile = 8; hw_level = 0x1000; profile = (char *) "high";
                        break;
                }
            }

            if (args->thread_num >= 0) {
                ret = av_dict_set(&param, "profile", profile, 0);
                ret = av_dict_set(&param, "tune", "zerolatency", 0); // "animation"
                // ultrafast,superfast,veryfast,faster,fast,medium,slow,slower,veryslow,placebo
                ret = av_dict_set(&param, "preset", "superfast", 0);
                //ret = av_opt_set(codec_ctx->priv_data, "preset", "superfast", 0);
                //ret = av_opt_set(codec_ctx->priv_data, "tune", "zerolatency", 0);
                //ret = av_opt_set(codec_ctx->priv_data, "qp", mode, 0);
                //ret = av_opt_set_double(codec_ctx->priv_data, "crf", 23.f, 0);

                codec_ctx->thread_count = args->thread_num;
                codec_ctx->thread_type = FF_THREAD_FRAME;
                //Optional Param
                //codec_ctx->me_pre_cmp = 1;
                //codec_ctx->me_range = 16;
                //codec_ctx->max_qdiff = 4;
                //codec_ctx->qcompress = 0.6;

                switch (args->v_bitrate_mode) {
                    case VideoBitrateMode::BITRATE_MODE_CQ:
                        break;
                    case VideoBitrateMode::BITRATE_MODE_VBR:
                        codec_ctx->bit_rate_tolerance = args->v_bitrate * args->v_bitrate_factor;
                        codec_ctx->rc_buffer_size = max_rate;
                        codec_ctx->rc_max_rate = max_rate;
                        codec_ctx->rc_min_rate = min_rate;
                        //调节清晰度和编码速度,调节编码后输出数据量越大输出数据量越小,越大编码速度越快,清晰度越差
                        codec_ctx->flags |= AV_CODEC_FLAG_QSCALE;
                        //codec_ctx->i_quant_factor = 0.7;
                        break;
                    case VideoBitrateMode::BITRATE_MODE_CBR:
                        max_rate = codec_ctx->rc_buffer_size =
                        codec_ctx->rc_max_rate = codec_ctx->rc_min_rate = args->v_bitrate;
                        break;
                }

                video_codec = avcodec_find_encoder(codec_ctx->codec_id);
            } else {
                ret = av_dict_set_int(&param, "profile", hw_profile, 0);
                ret = av_dict_set_int(&param, "level", hw_level, 0);
                ret = av_dict_set_int(&param, "fr", args->frame_rate, 0);
                ret = av_dict_set_int(&param, "i_frame_interval", args->gop, 0);
                ret = av_dict_set_int(&param, "color_format", args->v_color_format, 0);

                const char *bitrate_mode = "cq";
                switch (args->v_bitrate_mode) {
                    case VideoBitrateMode::BITRATE_MODE_CQ:
                        bitrate_mode = "cq";
                        break;
                    case VideoBitrateMode::BITRATE_MODE_VBR:
                        bitrate_mode = "vbr";
                        break;
                    case VideoBitrateMode::BITRATE_MODE_CBR:
                        bitrate_mode = "cbr";
                        max_rate = args->v_bitrate;
                        break;
                }
                ret = av_dict_set(&param, "bitrate_mode", bitrate_mode, 0);

                if (codec_ctx->codec_id == AV_CODEC_ID_H264) {
                    video_codec = avcodec_find_encoder_by_name("h264_mediacodec_enc");
                } else if (codec_ctx->codec_id == AV_CODEC_ID_HEVC) {
                    video_codec = avcodec_find_encoder_by_name("hevc_mediacodec_enc");
                }
            }
        }

        if (!video_codec) {
            ret = -1;
            LOGE("can not find encoder!");
            goto err;
        }
        ret = avcodec_open2(codec_ctx, video_codec, &param);
        if (ret < 0) {
            LOGE("failed to open encoder! %d", ret);
            goto err;
        }

        {
            AVCPBProperties *cpb = av_cpb_properties_alloc(NULL);
            cpb->avg_bitrate = args->v_bitrate;
            cpb->max_bitrate = max_rate;
            cpb->min_bitrate = min_rate;
            codec_ctx->opaque = cpb;
        }

        in_y_size = args->in_width * args->in_height;
        out_y_size = args->out_width * args->out_height;
        in_size = in_y_size * 3 / 2;
        out_size = out_y_size * 3 / 2;
        pic_size = av_image_get_buffer_size(codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, 1);
        frame = av_frame_alloc();
        if (args->thread_num >= 0) {
            frame->format = codec_ctx->pix_fmt;
        } else {
            frame->format = AV_PIX_FMT_NV12;
            frame420 = av_frame_alloc();
            frame420->format = AV_PIX_FMT_YUV420P;
            pic_buf420 = (uint8_t *) av_malloc(pic_size * sizeof(uint8_t));
            ret = av_image_fill_arrays(frame420->data, frame420->linesize, pic_buf420,
                                       static_cast<AVPixelFormat>(frame420->format),
                                       codec_ctx->width, codec_ctx->height, 1);
            if (ret < 0) {
                LOGE("failed to fill image %d", ret);
                goto err;
            }
        }
        pic_buf = (uint8_t *) av_malloc(pic_size * sizeof(uint8_t));
        ret = av_image_fill_arrays(frame->data, frame->linesize, pic_buf,
                                   static_cast<AVPixelFormat>(frame->format),
                                   codec_ctx->width, codec_ctx->height, 1);
        if (ret < 0) {
            LOGE("failed to fill image %d", ret);
            goto err;
        }
        av_dict_free(&param);
        av_new_packet(&pkt, pic_size);
        ret = 0;
    }

    audio_frame_cnt = 0;
    video_frame_cnt = 0;
    audio_start_time = 0;
    video_start_time = 0;
    last_pts = -1;
    enc_video_frame_cnt = 0;
    enc_audio_frame_cnt = 0;
    last_check_send_ts = 0;
    mux_audio_frame_cnt = 0;
    mux_video_frame_cnt = 0;
    wait_iframe = true;

    // test
    m_flag = false;
    m_last_fps = 0;
    m_count = 0;
    m_start_time = 0;

    return ret;
err:
    if (fmt_ctx != NULL) {
        if (codec_ctx != NULL) {
            if (codec_ctx->opaque != NULL) {
                av_freep(&codec_ctx->opaque);
            }
            avcodec_close(codec_ctx);
            codec_ctx = NULL;
        }
        avformat_free_context(fmt_ctx);
        fmt_ctx = NULL;
    }
    if (frame != NULL) {
        av_free(pic_buf); pic_buf = NULL;
        av_frame_free(&frame);
    }
    if (frame420 != NULL) {
        av_free(pic_buf420); pic_buf420 = NULL;
        av_frame_free(&frame420);
    }
    av_dict_free(&param);

    return ret;
}

int GB28181Muxer::initSender(StreamSender *sender) {
    int ret;
    LOGI("[muxer]init sender, local port %d, tid:%d", sender->local_port, gettid());
    if (gb_sender == NULL) {
        gb_sender = new GB28181Sender(args);
    }
    ret = gb_sender->initSender(sender);
    return ret;
}

int GB28181Muxer::endSender(StreamSender *sender) {
    if (sender == NULL) {
        return -1;
    }
    LOGI("[muxer]end sender sock %d local port %d, tid:%d", sender->sockfd, sender->local_port, gettid());
    return close_socket(sender->sockfd);
}

int GB28181Muxer::startMuxer() {
    if (is_running) {
        LOGI("[muxer]start mux is running");
        return 0;
    }
    if (codec_ctx == NULL) {
        return -1;
    }
    LOGI("[muxer]starting mux, tid:%d", gettid());
    is_running = STATE_RUNNING;
    if (args->v_encodec != VideoCodec::NONE) {
        pthread_create(&thread_encode, NULL, startEncode, this);
        pthread_create(&thread_mux, NULL, startMux, this);
    } else {
        pthread_create(&thread_mux, NULL, startMuxDirect, this);
    }
    LOGI("[muxer]started mux");
    return 0;
}

void GB28181Muxer::checkFps()
{
    if (m_start_time == 0) {
        m_start_time = time(NULL);
    }

    time_t current_time = time(NULL);
    double diff = difftime(current_time, m_start_time);

    if (diff >= 5 && !m_flag) {
        m_last_fps = m_count / diff;
        m_start_time = current_time;
        m_count = 0;
        m_flag = true;
        LOGI("fps=== %.2f", m_last_fps);
    }
    m_flag = false;
    m_count++;
}

int GB28181Muxer::pushVideoFrame(uint8_t *buf, int len, int64_t pts, bool keyframe) {
    if (!is_running) {
        return 0;
    }
    if (buf == NULL || len <= 0 || (in_size > 0 && len < in_size)) {
        return -1;
    }
    static int64_t st = 0, et = 0;
    if (DEBUG) {
        st = pts;
    }

    uint8_t *new_buf;
    if (args->v_encodec != VideoCodec::NONE) {
        new_buf = (uint8_t *) malloc(TS_LEN + len);
        *(int64_t*)new_buf = pts;
        memcpy(new_buf + TS_LEN, buf, len);
    } else {
        int header_len = DATA_LEN + TS_LEN + sizeof(char);
        int total_len = header_len + len;
        new_buf = (uint8_t *) malloc(total_len);
        *(int*)new_buf = len;
        *(int64_t*)(new_buf + DATA_LEN) = pts;
        *(char*)(new_buf + DATA_LEN + TS_LEN) = keyframe ? 1 : 0;
        memcpy(new_buf + header_len, buf, len);
    }
    video_queue.push(new_buf);
    video_frame_cnt++;
    if (video_start_time == 0) {
        video_start_time = pts;
    }
    if (DEBUG) {
        et = get_cur_time();
        checkFps();
        LOGD("[muxer]push video frame to queue time：%lld, frames:%lld", et - st, video_frame_cnt);
    }
    return 0;
}

int GB28181Muxer::pushAudioFrame(uint8_t *buf, int len, int64_t pts) {
    if (!is_running) {
        return -1;
    }
    if (buf == NULL || len <= 0 || args->a_frame_len <= 0) {
        return -1;
    }
    static int64_t st = 0, et = 0;
    if (DEBUG) {
        st = get_cur_time();
    }
    uint8_t *encoded_audio = static_cast<uint8_t *>(malloc(TS_LEN + args->a_frame_len / 2));
    int ret = g711_encode(reinterpret_cast<char *>(buf),
                          reinterpret_cast<char *>(encoded_audio + TS_LEN),
                          args->a_frame_len, G711_A_LAW);
    if (ret < 0) {
        LOGE("[muxer]encode audio failed! len: %d, data: %d %d %d %d %d",
                args->a_frame_len, buf[0], buf[1], buf[2], buf[3], buf[4]);
        return -1;
    }
    *(int64_t*)encoded_audio = pts;
    audio_queue.push(encoded_audio);
    audio_frame_cnt++;
    if (audio_start_time == 0) {
        audio_start_time = pts;
    }
    if (DEBUG) {
        et = get_cur_time();
    }
    LOGD("[muxer]push audio frame to queue time：%lld, frames:%lld", et - st, audio_frame_cnt);
    return 0;
}

void* GB28181Muxer::startEncode(void *obj) {
    LOGI("[muxer][encode]start encode");
    GB28181Muxer *gb_muxer = (GB28181Muxer *) obj;
    const int skip_num = 10;
    int ret;
    uint64_t st = 0, et = 0;
    uint8_t *frame_buf;
    int64_t pts, frame_pts;
    int input_retry = 0;

    while (gb_muxer->is_running) {
        if (gb_muxer->video_queue.size() > gb_muxer->args->queue_max) {
            LOGW("[muxer][encode]queue size %d is too bigger, clearing...", gb_muxer->video_queue.size())
            for (int i = 0; i < skip_num; ++i) {
                if (!gb_muxer->is_running) {
                    break;
                }
                frame_buf = *gb_muxer->video_queue.try_pop();
                delete frame_buf;
            }
        }
        if (DEBUG) {
            st = get_cur_time();
        }
        frame_buf = *gb_muxer->video_queue.wait_and_pop();
        if (!gb_muxer->is_running || frame_buf == NULL) {
            delete frame_buf;
            break;
        }

        gb_muxer->mtx_encode.lock();
        pts = *(int64_t*)frame_buf;
        frame_pts = pts - gb_muxer->video_start_time;
        gb_muxer->customFilter(frame_buf + TS_LEN, pts, gb_muxer->frame);
        delete frame_buf;

        if (gb_muxer->args->thread_num < 0) {
            frame_pts *= 1000;
        } else {
            frame_pts *= 90;
        }
        gb_muxer->frame->width = gb_muxer->codec_ctx->width;
        gb_muxer->frame->height = gb_muxer->codec_ctx->height;
        gb_muxer->frame->pts = frame_pts;
        ret = avcodec_send_frame(gb_muxer->codec_ctx, gb_muxer->frame);
        gb_muxer->mtx_encode.unlock();
        if (ret < 0) {
            input_retry = 0;
            while (gb_muxer->is_running && ret == AVERROR(EAGAIN) && input_retry++ < 5) {
                usleep(10000);
                gb_muxer->mtx_encode.lock();
                ret = avcodec_send_frame(gb_muxer->codec_ctx, gb_muxer->frame);
                gb_muxer->mtx_encode.unlock();
                LOGW("[muxer][encode]retry %d send frame (pts: %lld)", input_retry, frame_pts);
            }
            if (ret != 0) {
                LOGW("[muxer][encode]send frame (pts: %lld) error %d", frame_pts, ret);
            }
        } else {
            gb_muxer->enc_video_frame_cnt++;
        }
        if (DEBUG) {
            et = get_cur_time();
            LOGD("[muxer][encode]get frame(pts: %lld / %lld) from vqueue(left: %d) time: %lld, in time: %lld, encode time: %lld",
                 frame_pts, pts, gb_muxer->video_queue.size(), pts - st, et - pts, et - st);
        }
    }
    LOGI("[muxer][encode]encode is over");
    gb_muxer->mtx_encode.lock();
    if (gb_muxer->codec_ctx != NULL) {
        avcodec_send_frame(gb_muxer->codec_ctx, NULL);
    }
    gb_muxer->mtx_encode.unlock();
    return NULL;
}

void* GB28181Muxer::startMux(void *obj) {
    LOGI("[muxer][mux]start mux");
    int ret;
    int64_t st = 0, et = 0;
    GB28181Muxer *gb_muxer = (GB28181Muxer *) obj;

    while (gb_muxer->is_running) {
        if (DEBUG) {
            LOGD("[muxer][mux]vqueue: %d, aqueue: %d)", gb_muxer->video_queue.size(), gb_muxer->audio_queue.size());
            st = get_cur_time();
        }

        gb_muxer->mtx_encode.lock();
        ret = avcodec_receive_packet(gb_muxer->codec_ctx, &gb_muxer->pkt);
        gb_muxer->mtx_encode.unlock();
        if (ret < 0) {
            if (ret != AVERROR(EAGAIN)) {
                LOGE("receive_packet error %d", ret);
            }
            av_packet_unref(&gb_muxer->pkt);
            usleep(10);
            continue;
        }
        if (gb_muxer->pkt.data == NULL || gb_muxer->pkt.size <= 0) {
            LOGE("[muxer][mux]recv packet (pts: %lld, %d, %d) data is null",
                    gb_muxer->pkt.pts, gb_muxer->pkt.size, gb_muxer->pkt.side_data_elems);
            continue;
        }

        if (gb_muxer->args->thread_num < 0) {
            gb_muxer->pkt.pts = (gb_muxer->pkt.pts / 1000) * 90;
        }
        if (gb_muxer->last_pts == -1) { // 首帧
            if (gb_muxer->wait_iframe) {
                if (gb_muxer->pkt.flags == 1) {
                    gb_muxer->wait_iframe = false;
                } else {
                    av_packet_unref(&gb_muxer->pkt);
                    continue;
                }
            }
            LOGD("[muxer][mux]got first encoded pkt (pts: %lld)\n", gb_muxer->pkt.pts);
        } else {
            LOGD("[muxer][mux]got encoded pkt (pts: %lld)\n", gb_muxer->pkt.pts);
        }

        LOGD("frame num %d", gb_muxer->codec_ctx->frame_number);
        // TODO: 对视频帧 NALU 做 35114 签名和加密处理，B/C级，这里有需要的朋友可以联系我们
        if (/*g_config_ua->gm_level > 1*/0) {

        }

        ret = gb_muxer->mux(gb_muxer->pkt.data, gb_muxer->pkt.size, gb_muxer->pkt.pts, gb_muxer->pkt.flags == 1);
        gb_muxer->last_pts = gb_muxer->pkt.pts;
        av_packet_unref(&gb_muxer->pkt);
        if (ret >= 0) {
            if (DEBUG) {
                et = get_cur_time();
                LOGD("[muxer][mux]mux one pkt over (vqueue: %d, aqueue: %d), time used: %lld",
                     gb_muxer->video_queue.size(), gb_muxer->audio_queue.size(), et - st);
            }
#if TEST_VBR
            ret = random(0, gb_muxer->args->queue_max + 10);
#endif
            if (gb_muxer->args->v_bitrate_adjust_period > 0 &&
                (ret >= gb_muxer->args->queue_max || ret < gb_muxer->args->queue_max / 4)) {
                et = get_cur_time();
                if (gb_muxer->last_check_send_ts == 0 || et > gb_muxer->last_check_send_ts + gb_muxer->args->v_bitrate_adjust_period * 1000) {
                    LOGD("[muxer][mux] pending send queue size: %d", ret);
                    if (gb_muxer->last_check_send_ts != 0) {
                        gb_muxer->adjustBitrate(ret < gb_muxer->args->queue_max);
                    }
                    gb_muxer->last_check_send_ts = et;
                }
            }
        }
    }
    LOGI("[muxer][mux]mux is over");
    return NULL;
}

void* GB28181Muxer::startMuxDirect(void *obj) {
    LOGI("[muxer][mux]start mux direct");
    int ret;
    GB28181Muxer *gb_muxer = (GB28181Muxer *) obj;
    uint8_t *frame_buf;

    while (gb_muxer->is_running) {
        frame_buf = *gb_muxer->video_queue.wait_and_pop();
        if (!gb_muxer->is_running || frame_buf == NULL) {
            delete frame_buf;
            break;
        }

        int flags = frame_buf[DATA_LEN + TS_LEN];
        if (gb_muxer->wait_iframe) {
            if (flags == 1) {
                gb_muxer->wait_iframe = false;
            } else {
                delete frame_buf;
                continue;
            }
        }

        int len = *(int*)frame_buf; // bytes2int
        int64_t pts = *(int64_t*)(frame_buf + DATA_LEN);// bytes2long
        pts = (pts - gb_muxer->video_start_time) * 90;
        if (pts < gb_muxer->last_pts) {
            LOGW("[muxer][mux]out of order pts: %lld, last: %lld", pts, gb_muxer->last_pts);
            pts = gb_muxer->last_pts + 1;
        }
        else {
            LOGD("[muxer][mux]pts: %lld, last pts: %lld", pts, gb_muxer->last_pts);
        }

        gb_muxer->mux(frame_buf + DATA_LEN + TS_LEN + sizeof(char), len, pts, flags);
        gb_muxer->last_pts = pts;
        delete frame_buf;
    }
    LOGI("[muxer][mux]mux is over");
    return NULL;
}

/**
 * PS包=PS头|PES(video)|PES(audio)
 * @param data
 * @param data_len
 * @param pts
 * @param keyframe
 * @return
 */
int GB28181Muxer::mux(uint8_t* data, int data_len, int64_t pts, bool keyframe) {
    uint8_t header_buf[512];
    uint32_t header_pos = 0;
    uint32_t audio_cnt = 0, video_cnt = 1, i = 0;
    int pes_h_len = PES_HDR_LEN, pes_pl_len, h_len, len;
    int pending_send_que_size = 0;
    uint8_t* v_buf;

    // 有音频并需要封装发送
    if (args->enable_audio) {
        audio_cnt = 0;
        int a_size = audio_queue.size();
        for (i = 0; i < a_size; i++) {
            if (!is_running) {
                break;
            }
            uint8_t *aframe;
            bool r = audio_queue.at(i, aframe);
            if (!r) {
                break;
            }
            int64_t a_pts = *(int64_t *) aframe;
            a_pts = (a_pts - audio_start_time) * 90;
            // 比较音频时间戳和视频时间戳, 音频同步到视频
            if (a_pts > pts) {
                break;
            }
            audio_cnt++;
        }
        LOGD("now pts:%lld, last pts: %lld, diff:%lld, audio count:%d",
                pts, last_pts, pts - last_pts, audio_cnt);
    }

    // video pes count
    if (data_len > PES_PAYLOAD_MAX_LEN) {
        video_cnt = ((data_len / PES_PAYLOAD_MAX_LEN) + ((data_len % PES_PAYLOAD_MAX_LEN) != 0));
    }

    // 1 ps header
    h_len = gb_make_ps_header(header_buf + header_pos, pts, mux_rate);
    header_pos += h_len;
    if (keyframe) {
        // 2 for I frame, add SYS_HEADER and PSM_HEADER
        h_len = gb_make_sys_header(header_buf + header_pos, args->enable_audio ? 1 : 0, 1, mux_rate);
        header_pos += h_len;
        h_len = gb_make_psm_header(header_buf + header_pos,
                                   codec_ctx->codec_id == AV_CODEC_ID_H264 ? ST_ID_H264 : ST_ID_H265,
                                   args->enable_audio ? ST_ID_G711A : -1);
        header_pos += h_len;
    }

    uint32_t a_data_len = audio_cnt * (pes_h_len + g711a_frame_len);
    uint32_t v_data_len = video_cnt * pes_h_len + data_len;
    uint32_t pkt_len = header_pos + a_data_len + v_data_len;
    uint8_t *pkt_full = (uint8_t *) malloc(TS_LEN + FRAME_FLAG_LEN + DATA_LEN + pkt_len);
    uint32_t pkt_pos = 0;

    memcpy(pkt_full + pkt_pos, long2bytes(pts), TS_LEN);
    pkt_pos += TS_LEN;
    memcpy(pkt_full + pkt_pos, (uint8_t*) &keyframe, FRAME_FLAG_LEN);
    pkt_pos += FRAME_FLAG_LEN;
    memcpy(pkt_full + pkt_pos, uint2bytes(pkt_len), DATA_LEN);
    pkt_pos += DATA_LEN;

    memcpy(pkt_full + pkt_pos, header_buf, header_pos);
    pkt_pos += header_pos;

    // video
    len = data_len;
    v_buf = data;
    while (len > 0) {
        pes_pl_len = (len > PES_PAYLOAD_MAX_LEN) ? PES_PAYLOAD_MAX_LEN : len;
        pes_h_len = gb_make_pes_header(pkt_full + pkt_pos, VIDEO_ID, pes_pl_len, pts, pts);
        pkt_pos += pes_h_len;

        memcpy(pkt_full + pkt_pos, v_buf, pes_pl_len);
        pkt_pos += pes_pl_len;

        len -= pes_pl_len;
        v_buf += pes_pl_len;
    }

    if (args->enable_audio) {
        i = 0;
        while (i++ < audio_cnt) {
            uint8_t *aframe = *audio_queue.try_pop();
            if (!is_running || aframe == NULL) {
                delete aframe;
                break;
            }
            int64_t a_pts = *(int64_t *) aframe;
            a_pts = (a_pts - audio_start_time) * 90;
            // 比较音频时间戳和视频时间戳, 音频同步到视频
            if (a_pts > pts) {
                break;
            }
            pes_h_len = gb_make_pes_header(pkt_full + pkt_pos, AUDIO_ID, g711a_frame_len, a_pts, a_pts);
            pkt_pos += pes_h_len;
            memcpy(pkt_full + pkt_pos, aframe + TS_LEN, g711a_frame_len);
            pkt_pos += g711a_frame_len;
            mux_audio_frame_cnt++;
            delete aframe;
        }
    }
    if (is_running != STATE_RUNNING) {
        delete pkt_full;
        pkt_full = NULL;
        return 0;
    }

    if (is_recording) {
        if (NULL == args->fout || (args->rec_duration > 0 && keyframe && pts - last_record_pts > args->rec_duration * 90000)) {
            if (NULL != args->fout) {
                fclose(args->fout);
            }

            // 文件名格式: time#secrecy#record_type
            char time_buf[64];
            char *file_name = get_time_with_t(time_buf, 64); // 2010-11-11T19:46:17
            char new_path[1024];
            snprintf(new_path, sizeof(new_path) - 1, "%s/%c%c%c%c/%c%c/%c%c/", args->media_path,
                     file_name[0], file_name[1], file_name[2], file_name[3],  // 年
                     file_name[5], file_name[6], file_name[8], file_name[9]); // 月/日
            mkdir_r(new_path);
            strcat(file_name, "#0#time"); // TODO: #secrecy#record_type
            char *rec_file = get_record_full_name(new_path, file_name);
            args->fout = fopen(rec_file, "wb");
            free(rec_file);
            last_record_pts = pts;
        }

        if (args->fout != NULL) {
            uint8_t *buf = pkt_full + TS_LEN + FRAME_FLAG_LEN + DATA_LEN;
            fwrite(buf, 1, pkt_len, args->fout);
        }
    }

    if (gb_sender != NULL) {
        pending_send_que_size = gb_sender->addPacket(pkt_full);
    }
    return pending_send_que_size;
}

int GB28181Muxer::endMuxerInternal() {
    LOGI("[muxer]ending, audio left num: %d, video left num: %d", audio_queue.size(), video_queue.size());

    while (!audio_queue.empty()) {
        uint8_t *aframe = *audio_queue.try_pop();
        delete aframe;
    }
    while (!video_queue.empty()) {
        uint8_t *vframe = *video_queue.try_pop();
        delete vframe;
    }
    audio_queue.push(NULL);
    video_queue.push(NULL);
    pthread_join(thread_encode, NULL);
    pthread_join(thread_mux, NULL);
    audio_queue.clear();
    video_queue.clear();

    if (gb_sender != NULL) {
        delete gb_sender;
        gb_sender = NULL;
    }

    if (fmt_ctx != NULL) {
        if (codec_ctx != NULL) {
            if (codec_ctx->opaque != NULL) {
                av_freep(&codec_ctx->opaque);
            }
            avcodec_close(codec_ctx);
            codec_ctx = NULL;
        }
        avformat_free_context(fmt_ctx);
        fmt_ctx = NULL;
    }
    if (frame != NULL) {
        av_free(pic_buf); pic_buf = NULL;
        av_frame_free(&frame);
    }
    if (frame420 != NULL) {
        av_free(pic_buf420); pic_buf420 = NULL;
        av_frame_free(&frame420);
    }

    if (pkt.size > 0) {
        av_packet_unref(&pkt);
    }

    if (args->fout != NULL) {
        fclose(args->fout);
        args->fout = NULL;
    }
    last_record_pts = 0;

    LOGI("[muxer]ended")
    return 0;
}

int GB28181Muxer::startRecord()
{
    LOGI("[muxer]start record, tid:%d", gettid());
    is_recording = true;
    return 0;
}

int GB28181Muxer::stopRecord()
{
    LOGI("[muxer]stop record, tid:%d", gettid());
    is_recording = false;
    return 0;
}

int GB28181Muxer::endMuxer() {
    LOGI("[muxer]end mux, tid:%d", gettid());
    if (is_running == STATE_STOPPED) {
        return 0;
    }
    is_running = STATE_STOPPED;
    return endMuxerInternal();
}

/**
 * https://blog.csdn.net/u010126792/article/details/86593199
 * 设置输出视频流尺寸，采样率
 * YV12 is a 4:2:0 YCrCb planar format comprised of a WxH Y plane followed by (W/2) x (H/2) Cr and Cb planes.
 *
 * NV12,NV21,YV12,I420都属于YUV420,但YUV420又分为YUV420P，YUV420SP，P与SP区别就是，
 * 前者YUV420P UV顺序存储，而YUV420SP则是UV交错存储，这是最大的区别
 *
 * YUV444
 * （1）YUV444p：YYYYYYYYY VVVVVVVVV UUUUUUUU
 * YUV422
 * （1）YUV422p：YYYYYYYY VVVV UUUU
 * （2）YUVY：YUYV YUYV YUYV YUYV
 * （3）UYVY：UYVY UYVY UYVY UYVY
 * YUV420
 * （1）YUV420p
 *      YV12：YYYYYYYY VV UU
 *      YU12（I420）：YYYYYYYY UU VV
 *  (2）YUV420sp
 *      NV12：YYYYYYYY UVUV
 *      NV21：YYYYYYYY VUVU
 *
 * N21  0 ~ width*height是Y分量，width*height ~ width*height*3/2是VU交替存储
 *
 * NV21-> YUV420P
 *  memcpy(pFrameYUV->data[0],in,y_len);
 * for(i=0;i<uv_len;i++)
 * {
 *   *(pFrameYUV->data[2]+i)=*(in+y_len+i*2);
 *   *(pFrameYUV->data[1]+i)=*(in+y_len+i*2+1);
 * }
 *
 * @param yuv_buf nv21
 * @param pts
 * @param pFrame
 */
void GB28181Muxer::customFilter(const uint8_t *yuv_buf, int64_t pts, AVFrame *out_frame) {
    RotationMode rotate = (RotationMode) args->v_rotate;
    int new_w = 0, new_h = 0;

    switch (rotate) {
        case kRotate0:   new_w = args->out_width; new_h = args->out_height; break;
        case kRotate90:  new_w = args->out_height; new_h = args->out_width; break;
        case kRotate180: new_w = args->out_width; new_h = args->out_height; break;
        case kRotate270: new_w = args->out_height; new_h = args->out_width; break;
        default: break;
    }

    if (args->thread_num > 0) {
        ConvertToI420(yuv_buf, in_size, // args->in_width * args->in_height
                      frame->data[0], frame->linesize[0],
                      frame->data[1], frame->linesize[1],
                      frame->data[2], frame->linesize[2],
                      0, 0, args->in_width, args->in_height,
                      args->out_width, args->out_height,
                      rotate, FourCC::FOURCC_NV21);
    } else {
        ConvertToI420(yuv_buf, in_size, // args->in_width * args->in_height
                      frame420->data[0], frame420->linesize[0],
                      frame420->data[1], frame420->linesize[1],
                      frame420->data[2], frame420->linesize[2],
                      0, 0, args->in_width, args->in_height,
                      args->out_width, args->out_height,
                      rotate, FourCC::FOURCC_NV21);
        I420ToNV12(frame420->data[0], frame420->linesize[0],
                   frame420->data[1], frame420->linesize[1],
                   frame420->data[2], frame420->linesize[2],
                   out_frame->data[0], out_frame->linesize[0],
                   out_frame->data[1], out_frame->linesize[1],
                   new_w, new_h);
    }

//    // y值在h方向开始行
//    int y_h_start_index = args->in_height - args->out_height;
//    // uv值在h方向开始行
//    int uv_h_start_index = (y_h_start_index + 1) >> 1;
//    int half_in_w = (args->in_width + 1) >> 1;
//    int half_in_h =  (args->in_height + 1) >> 1;
//    int half_out_w = (args->out_width + 1) >> 1;
//    int half_out_h =  (args->out_height + 1) >> 1;
//    const uint8_t* y_ptr = yuv_buf;
//    const uint8_t* v_ptr = yuv_buf + (args->in_width * args->in_height);
//    const uint8_t* u_ptr = yuv_buf + (args->in_width * args->in_height / 4);
//    if (rotate == ROTATE_90_CROP_LT) {
//        new_w = args->out_height;
//        for (int i = y_h_start_index; i < args->in_height; i++) {
//            for (int j = 0; j < args->out_width; j++) {
//                int index = args->in_width * i + j;
//                uint8_t value = *(yuv_buf + index);
//                *(out_frame->data[0] + j * args->out_height +
//                  (args->out_height - (i - y_h_start_index) - 1)) = value;
//            }
//        }
//        for (int i = uv_h_start_index; i < half_in_h; i++) {
//            for (int j = 0; j < half_out_w; j++) {
//                int index = half_in_w * i + j;
//                uint8_t v = *(yuv_buf + in_y_size + index);
//                uint8_t u = *(yuv_buf + in_y_size * 5 / 4 + index);
//                *(out_frame->data[2] + (j * half_out_h + (half_out_h -
//                                                     (i - uv_h_start_index) - 1))) = v;
//                *(out_frame->data[1] + (j * half_out_h + (half_out_h -
//                                                     (i - uv_h_start_index) - 1))) = u;
//            }
//        }
//    } else if (rotate == ROTATE_0_CROP_LT) {
//        new_w = args->out_width;
//        for (int i = y_h_start_index; i < args->in_height; i++) {
//            for (int j = 0; j < args->out_width; j++) {
//                int index = args->in_width * i + j;
//                uint8_t value = *(yuv_buf + index);
//                *(out_frame->data[0] + (i - y_h_start_index) * args->out_width + j) = value;
//            }
//        }
//        for (int i = uv_h_start_index; i < half_in_h; i++) {
//            for (int j = 0; j < half_out_w; j++) {
//                int index = half_in_w * i + j;
//                uint8_t v = *(yuv_buf + in_y_size + index);
//                uint8_t u = *(yuv_buf + in_y_size * 5 / 4 + index);
//                *(out_frame->data[2] + ((i - uv_h_start_index) * half_out_w + j)) = v;
//                *(out_frame->data[1] + ((i - uv_h_start_index) * half_out_w + j)) = u;
//            }
//        }
//    } else if (rotate == ROTATE_270_CROP_LT_MIRROR_LR) {
//        int y_width_start_index = args->in_width - args->out_width;
//        int uv_width_start_index = y_width_start_index >> 1;
//        new_w = args->out_height;
//
//        for (int i = 0; i < args->out_height; i++) {
//            for (int j = y_width_start_index; j < args->in_width; j++) {
//                int index = args->in_width * (args->out_height - i - 1) + j;
//                uint8_t value = *(yuv_buf + index);
//                *(out_frame->data[0] + (args->out_width - (j - y_width_start_index) - 1)
//                                    * args->out_height + i) = value;
//            }
//        }
//        for (int i = 0; i < half_out_h; i++) {
//            for (int j = uv_width_start_index; j < half_in_w; j++) {
//                int index = half_in_w * (half_out_h - i - 1) + j;
//                uint8_t v = *(yuv_buf + in_y_size + index);
//                uint8_t u = *(yuv_buf + in_y_size * 5 / 4 + index);
//                *(out_frame->data[2] + (half_out_w - (j - uv_width_start_index) - 1)
//                                    * half_out_h + i) = v;
//                *(out_frame->data[1] + (half_out_w - (j - uv_width_start_index) - 1)
//                                    * half_out_h + i) = u;
//            }
//        }
//    }

    if (g_osd_info.enable) {
        for (const osd_t& osd : g_osd_info.osds) {
            uint8_t *dest = out_frame->data[0] + (osd.y < 0 ? (new_h + osd.y) : osd.y) * new_w;

            if (osd.date_len > 0) {
                char date[64];
                time_t cur_ts = (time_t) (pts / 1000);
                strftime(date, sizeof(date), osd.text.c_str(), localtime(&cur_ts));

                for (int i = 0; i < osd.date_len; i++) {
                    int index = get_index(*(date + i));
                    if (index >= g_osd_info.chars.size())
                        continue;
                    char *ch = (char *) g_osd_info.chars.at(index).c_str();
                    uint8_t *column = dest + i * g_osd_info.char_width;
                    for (int j = 0; j < g_osd_info.char_height; j++) {
                        uint8_t *dest_index = column + j * new_w + (osd.x < 0 ? (new_w + osd.x) : osd.x);
                        char *src = ch + j * g_osd_info.char_width;
                        for (int k = 0; k < g_osd_info.char_width; k++) {
                            if (*(src + k) != 0) { //黑色背景色
                                *(dest_index + k) = -21; //水印文字颜色: -21为白色，0为黑色
                            }
                        }
                    }
                }
            } else {
                int osd_len = osd.text.size();
                for (int i = 0; i < osd_len; i++) {
                    int index = get_index(*(osd.text.c_str() + i));
                    if (index >= g_osd_info.chars.size())
                        continue;
                    char *ch = (char *) g_osd_info.chars.at(index).c_str();
                    uint8_t *column = dest + i * g_osd_info.char_width;
                    for (int j = 0; j < g_osd_info.char_height; j++) {
                        uint8_t *dest_index = column + j * new_w + (osd.x < 0 ? (new_w + osd.x) : osd.x);
                        char *src = ch + j * g_osd_info.char_width;
                        for (int k = 0; k < g_osd_info.char_width; k++) {
                            if (*(src + k) != 0) { //黑色背景色
                                *(dest_index + k) = -21; //水印文字颜色: -21为白色，0为黑色
                            }
                        }
                    }
                }
            }
        }
    }
}

/**
 * 码率变化范围因子：factor（0-1，默认0.3，可以设置）
 * 码率检查调整周期：VideoBitrateAdjustPeriod（默认30秒）
 *
 * 内部自动统计上行流量
 * （1）如果检测到当前已拥塞，且持续在VideoBitrateAdjustPeriod周期（在外部可设置）内一直拥塞，则以上一次码率
 *      的factor步长降低码率，如果下一次还是拥塞，则重复上述步骤，最低降低到码率不低于原始设置码率的factor；
 * （2）如果检测到当前不拥塞，则以上一次码率的factor步长进行增加码率，如果下一次VideoBitrateAdjustPeriod周期
 *      检查还不不拥塞，则重复上述步骤，最高升高到不超过原始设置码率的factor；
 *
 * 浮动因子 0.3，用的VBR，设置的1500K
 * 网络拥堵的时候，降低到0.7*1500，下次还是拥堵则继续降低到0.7*0.7*1500，直到最低  0.3*1500
 * 网络变好的时候，则会加上去，从当前的码率 x 变为 1.3*x,下次检测还是畅通则变为1.3*1.3*x ，直到变为 1500*1.3
 *
 * @param up
 */
void GB28181Muxer::adjustBitrate(bool up) {
    AVCPBProperties *cpb;
    int target_rate;

    if (codec_ctx->opaque == NULL) {
        return;
    }
    cpb = (AVCPBProperties *) codec_ctx->opaque;
    if (up) {
        target_rate = codec_ctx->bit_rate * (1 + args->v_bitrate_factor);
        if (target_rate > max_rate) {
            target_rate = max_rate;
        }
    } else {
        target_rate = codec_ctx->bit_rate * (1 - args->v_bitrate_factor);
        if (target_rate < min_rate) {
            target_rate = min_rate;
        }
    }
    if (target_rate == codec_ctx->bit_rate) {
        return;
    }

    LOGI("change bitrate %lld to %d [%d %d]", codec_ctx->bit_rate, target_rate, min_rate, max_rate);
    codec_ctx->bit_rate = target_rate;
    cpb->avg_bitrate = target_rate;

    if (args->thread_num < 0) {
        switch (args->v_bitrate_mode) {
            case VideoBitrateMode::BITRATE_MODE_CQ:
            case VideoBitrateMode::BITRATE_MODE_VBR:
                cpb->max_bitrate = target_rate * (1 + args->v_bitrate_factor);
                cpb->min_bitrate = target_rate * (1 - args->v_bitrate_factor);
                break;
            case VideoBitrateMode::BITRATE_MODE_CBR:
                cpb->max_bitrate = target_rate;
                cpb->min_bitrate = target_rate;
                break;
        }
    } else {
        switch (args->v_bitrate_mode) {
            case VideoBitrateMode::BITRATE_MODE_CQ:
            case VideoBitrateMode::BITRATE_MODE_VBR:
                codec_ctx->bit_rate_tolerance = cpb->avg_bitrate * args->v_bitrate_factor;
                codec_ctx->rc_buffer_size = cpb->max_bitrate;
                codec_ctx->rc_max_rate = cpb->max_bitrate;
                codec_ctx->rc_min_rate = cpb->min_bitrate;

                break;
            case VideoBitrateMode::BITRATE_MODE_CBR:
                codec_ctx->rc_buffer_size =
                codec_ctx->rc_max_rate = codec_ctx->rc_min_rate = cpb->avg_bitrate;
                break;
        }
    }
}

static int64_t calcBitrate(int quality, int width, int height, int fps) {
    static const int BasicRate[4] = {240, 400, 500, 700};

    int64_t bit_rate = 0;
    double factor = sqrt((double) (width * height * fps) / (360 * 640 * 10.0));

    if (quality <= 85) {
        //10KB
        bit_rate = (int64_t) (BasicRate[0] * factor * quality / 85 + 0.5);
    } else if (quality <= 100) {
        //16KB
        int BitrateTemp = BasicRate[0] + ((BasicRate[1] - BasicRate[0]) * (quality - 85) / 15.0);
        bit_rate = (int64_t) (BitrateTemp * factor + 0.5);
    } else if (quality <= 125) {
        //28KB
        int BitrateTemp = BasicRate[1] + ((BasicRate[2] - BasicRate[1]) * (quality - 100) / 25.0);
        bit_rate = (int64_t) (BitrateTemp * factor + 0.5);
    } else if (quality <= 150) {
        //40KB
        int BitrateTemp = BasicRate[2] + ((BasicRate[3] - BasicRate[2]) * (quality - 125) / 25.0);
        bit_rate = (int64_t) (BitrateTemp * factor + 0.5);
    } else {
        int BitrateTemp = BasicRate[3] + 16 * (quality - 150);
        bit_rate = (int64_t) (BitrateTemp * factor + 0.5);
    }
    bit_rate *= 1000;

    return bit_rate;
}
