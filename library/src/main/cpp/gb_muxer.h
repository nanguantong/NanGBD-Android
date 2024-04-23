//
// Created by nanuns on 2018/9/13.
//

#ifndef NANGBD_GB_MUXER_H
#define NANGBD_GB_MUXER_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}
#include "gb_sender.h"

using namespace std;

class GB28181Muxer {
public:
    GB28181Muxer(UserArgs *arg);
    ~GB28181Muxer();

public:
    /**
     * 初始化发送者
     * @param sender
     * @return 0:成功
     */
    int initSender(StreamSender *sender);

    /**
     * 结束发送者
     * @param sender
     * @return
     */
    int endSender(StreamSender *sender);

    /**
     * 初始化视频编码器
     * @return 0:成功
     */
    int initMuxer();

    /**
     * 开始视频编码和封装
     * @return 0:成功
     */
    int startMuxer();

    /**
     * 发送视频帧到编码队列
     *
     * @param buf
     * @param pts
     * @param keyframe
     * @return
     */
    int pushVideoFrame(uint8_t *buf, int len, int64_t pts, bool keyframe);

    /**
     * 编码并发送音频帧到编码队列
     *
     * @param buf
     * @param pts
     * @return
     */
    int pushAudioFrame(uint8_t *buf, int len, int64_t pts);

    /**
     * 开始录制
     * @return
     */
    int startRecord();

    /**
     * 停止录制
     * @return
     */
    int stopRecord();

    /**
     * 结束视频编码和封装
     */
    int endMuxer();

    /**
     * 配置参数
     * @return
     */
    UserArgs* getArgs();
    void setArgs(UserArgs* args);

    GB28181Sender* getSender()  { return gb_sender; };

private:
    /**
     * 合并编码的音视频并生成GB28181流的线程方法
     *
     * @param obj
     * @return
     */
    static void* startMux(void *obj);

    /**
     * 合并音视频并生成GB28181流的线程方法
     *
     * @param obj
     * @return
     */
    static void* startMuxDirect(void *obj);

    /**
     * 编码线程
     * 不断的从视频原始帧队列里面取帧出来送到ffmpeg中编码
     *
     * @param obj
     * @return
     */
    static void* startEncode(void *obj);

    /**
     * 将视频帧和音频帧打包的实际方法
     * 一个视频帧对应多个音频帧
     *
     * @return
     */
    int mux(uint8_t* data, int data_len, int64_t pts, bool keyframe);

    /**
     * 视频编码结束
     * @return
     */
    int endMuxerInternal();

    /**
     * 统计帧率
     */
    void checkFps();

    /**
     * 判断发送拥塞程度，周期性动态改变编码码率
     */
    void adjustBitrate(bool up);

    /**
     * 对视频做一些处理
     *
     * @param yuv_buf
     * @param in_y_size
     * @param format
     */
    void customFilter(const uint8_t *yuv_buf, int64_t pts, AVFrame *pFrame);

private:
    UserArgs *args = NULL;
    GB28181Sender *gb_sender = NULL;
    volatile int is_running = STATE_STOPPED;
    volatile bool is_recording = false; // 是否录制文件
    AVQueue<uint8_t *> video_queue;
    AVQueue<uint8_t *> audio_queue;
    pthread_t thread_mux, thread_encode;

    AVFormatContext *fmt_ctx = NULL;
    AVOutputFormat *fmt = NULL;
    AVStream *video_st = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVCodec *video_codec = NULL;
    AVPacket pkt;
    AVFrame *frame = NULL; // encode frame
    AVFrame *frame420 = NULL;
    uint8_t *pic_buf = NULL, *pic_buf420 = NULL;
    int pic_size = 0;
    int in_y_size = 0, out_y_size;
    int in_size = 0, out_size = 0;
    uint64_t audio_frame_cnt = 0;
    uint64_t video_frame_cnt = 0;
    int64_t audio_start_time = 0;
    int64_t video_start_time = 0;

    std::mutex mtx_encode;

    int max_rate = 0, min_rate = 0;
    int mux_rate = 0; /* bitrate in units of 50 bytes/s */
    int g711a_frame_len = 0;
    int64_t last_pts = -1;
    int64_t last_check_send_ts = 0;
    int scr_per_frame;
    bool wait_iframe;
    uint64_t enc_video_frame_cnt = 0;
    uint64_t enc_audio_frame_cnt = 0;
    uint64_t mux_video_frame_cnt = 0;
    uint64_t mux_audio_frame_cnt = 0;

    int64_t last_record_pts = 0;

    // test
    bool m_flag = false;
    double m_last_fps = 0;
    int64_t m_count = 0;
    time_t m_start_time = 0;
};

#endif // NANGBD_GB_MUXER_H
