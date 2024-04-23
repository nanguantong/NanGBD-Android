#ifndef NANGBD_USER_ARGS_H
#define NANGBD_USER_ARGS_H

#include <linux/in.h>
#include <unistd.h>
#include <map>
#include <mutex>
#include <vector>
#include "ua/ua_config.h"

#define MAX_INVITE_COUNT 3
#define MAX_VOICE_COUNT  1
#define MAX_RESEND_COUNT 3

#define BITRATE_FACTOR 0.3
#define BITRATE_ADJUST_PERIOD_SEC 30
#define STREAM_STAT_PERIOD_SEC 3

#define MAX_REC_DURATION_SEC 3600

int onCallbackStatus(int event_type, int key, const char* param, int param_len);
int onCallbackInvite(const task_t* task);
int onCallbackCancel(const task_t* task);
int onCallbackRecord(int start);
int onCallbackGmSignature(int start);
int onCallbackGmEncrypt(int start);

int onCallStop(int cid, bool is_voice = false, bool stop = false);
int onCallsStop(std::vector<int> cids, bool is_voice = false, bool stop = false);

typedef struct StreamSender {
    char server_ip[64] = {0};
    int server_port = 0;
    int local_port = 0;
    protocol_type transport = protocol_type::PROTO_TCP;
    int ssrc = 0;
    int payload_type = 0;

    uint16_t seq_num = 0;
    int sockfd = -1; // 发送者本地sock
    struct sockaddr_in client_addr = {0};
    struct sockaddr_in server_addr = {0};

    bool drop_frame = false; // 丢帧直到关键帧
    int retries = 0;
} StreamSender;

typedef enum VideoCodec {
    NONE  = 0,
    H264  = 1,
    H265  = 2,
} VideoCodec;

typedef enum VideoProfile {
    PROFILE_BASELINE = 1,
    PROFILE_MAIN     = 2,
    PROFILE_HIGH     = 3,
} VideoProfile;

typedef enum VideoColorFormat {
    COLOR_FormatYUV420Planar                              = 0x13, // 19
    COLOR_FormatYUV420SemiPlanar                          = 0x15, // 21
    COLOR_FormatYCbYCr                                    = 0x19,
    COLOR_FormatAndroidOpaque                             = 0x7f000789,
    COLOR_FormatYUV420Flexible                            = 0x7f420888, // 2135033992
    COLOR_QCOM_FormatYUV420SemiPlanar                     = 0x7fa30c00,
    COLOR_QCOM_FormatYUV420SemiPlanar32m                  = 0x7fa30c04,
    COLOR_QCOM_FormatYUV420PackedSemiPlanar64x32Tile2m8ka = 0x7fa30c03,
    COLOR_TI_FormatYUV420PackedSemiPlanar                 = 0x7f000100,
    COLOR_TI_FormatYUV420PackedSemiPlanarInterlaced       = 0x7f000001,
} ColorFormat;

typedef enum VideoBitrateMode {
    BITRATE_MODE_CQ  = 0,
    BITRATE_MODE_VBR = 1,
    BITRATE_MODE_CBR = 2,
} VideoBitrateMode;

typedef struct UserArgs {
    std::mutex *mtx_sender = NULL;
    std::map<int, StreamSender> *senders = NULL; // key: cid 取流发送

    int invite_count;            //最大支持并发取流数
    int voice_count;             //最大支持并发语音对讲数
    char *media_path = NULL;     //合成后的录像储存目录
    FILE *fout = NULL;           //合成后的录像文件
    int rec_duration;            //录制时长(秒)
    bool enable_video;
    bool enable_audio;
    VideoCodec v_encodec;        //视频编码器
    int in_width;                //输出宽度
    int in_height;               //输入高度
    int out_height;              //输出高度
    int out_width;               //输出宽度
    int frame_rate;              //视频帧率
    int gop;                     //视频gop(秒)
    int64_t v_bitrate;           //视频比特率
    int v_bitrate_mode;          //视频比特率控制模式
    VideoProfile v_profile;      //视频profile
    ColorFormat v_color_format;  //视频硬编码颜色空间
    int v_rotate;                //一些滤镜操作控制
    int a_frame_len;             //音频帧长
    int queue_max;               //队列最大长度
    int thread_num;              //编解码线程数

    double v_bitrate_factor;     //视频比特率上下浮动因子[0, 1]
    int v_bitrate_adjust_period; //视频比特率动态调整周期(秒)
    int stream_stat_period;      //流状态统计周期(秒)
} UserArgs;

#endif // NANGBD_USER_ARGS_H
