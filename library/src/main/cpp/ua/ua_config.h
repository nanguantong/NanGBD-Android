//
// Created by nanuns on 2020/6/12.
//

#ifndef NANGBD_UA_CONFIG_H
#define NANGBD_UA_CONFIG_H

#define SDK_VERSION "v1.0.23.238001"
#define UA_VERSION "NanUA/1.0"
#define CERT_PUB_FILE "/sdcard/ps/localcert.crt"
#define CERT_KEY_FILE "/sdcard/ps/localcert.key"
#define ENABLE_35114 1 // 是否能用35114功能
#define EXT_FILED 0
#define FIXED_COPYRIGHT 0

#define MEDIA_FORMAT_PS ".ps"

#define DEFAULT_MIN_PORT 10000
#define DEFAULT_MAX_PORT 10100

typedef enum {
    PROTO_UDP,
    PROTO_TCP,
} protocol_type;

typedef enum {
    UDP,
    TCP_PASSIVE,
    TCP_ACTIVE,
} ua_transport;

typedef enum {
    EVENT_CONNECTING = 1001,   // 注册连接中
    EVENT_REGISTERING,         // 注册正在开始中
    EVENT_REGISTER_OK,         // 注册成功
    EVENT_REGISTER_AUTH_FAIL,  // 注册鉴权失败
    EVENT_UNREGISTER_OK,       // 注销成功
    EVENT_UNREGISTER_FAIL,     // 注销失败
    EVENT_START_VIDEO,         // 开始推流
    EVENT_STOP_VIDEO,          // 停止推流
    EVENT_START_VOICE,         // 开始语音
    EVENT_STOP_VOICE,          // 停止语音
    EVENT_TALK_AUDIO_DATA,     // 语音对讲音频数据
    EVENT_START_RECORD,        // 开始录制
    EVENT_STOP_RECORD,         // 停止录制
    EVENT_STATISTICS = 1100,   // 编码和发送统计信息
    EVENT_DISCONNECT = 2000,   // 连接断开, 可在应用层做自动重新注册
} event_type;

typedef enum {
    LEVEL_N = 0, // 不加密
    LEVEL_A,     // A类加密
    LEVEL_B,     // B类加密
    LEVEL_C,     // C类加密
} gm_level;

typedef enum {
    PT_PCMU = 0,
    PT_PCMA = 8,
    PT_PS = 96,
    PT_MPEG4 = 97,
    PT_H264 = 98,
} payload_type;

typedef enum {
    TT_STREAM, // 取流
    TT_VOICE,  // 语音
} task_type;

/* invite task */
typedef struct task_s {
    task_type tk_type;
    int cid;
    int did;
    int tid;

    char remote_ip[32];
    int remote_port;
    char local_ip[32];
    int local_port;
    ua_transport transport;
    uint64_t start_time, end_time;
    char y[16];
    payload_type pl_type;

    int64_t invite_time;
    int acked;

    void* opaque;
} task_t;

typedef struct ua_config_s {
    char *server_ip;            // SIP服务器地址
    int server_port;            // SIP服务器端口
    char *server_id;            // SIP服务器ID
    char *server_domain;        // SIP服务器域

    int gm_level;               // 国密35114安全等级
    char *name;                 // 设备名称
    char *manufacturer;         // 设备生产商
    char *model;                // 设备型号
    char *firmware;             // 设备固件版本
    char *serial_number;        // 设备序列号
    int channel;                // 视频输入通道数

    char *username;             // SIP用户名 (设备id)
    char *userid;               // SIP用户认证ID
    char *password;             // SIP用户认证密码
    int local_port;             // SIP本地端口 (设备端口)
    char *protocol;             // 信令传输协议 (UDP/TCP)
    int expiry;                 // 注册有效期(秒), 范围3600-100000
    int heartbeat_interval;     // 心跳周期(秒), 范围5-255
    int heartbeat_count;        // 最大心跳超时次数, 范围3-255
    double longitude;           // 经度
    double latitude;            // 纬度
    char *video_id;             // 视频通道编码ID
    char *audio_id;             // 语音输出通道编码ID
    char *alarm_id;             // 报警通道编码ID
    char *charset;              // 字符编码类型 GB2312 / UTF-8

    int invite_count;           // 最大支持并发取流数
    int voice_count;            // 最大支持并发语音对讲数
    int pos_capability;         // 定位功能支持情况, 取值:0-不支持;1-支持GPS定位;2-支持北斗定位(可选, 默认取值为0)
    int global_auth;            // 是否使用全局注册认证信息

    int (*on_callback_status)(int event_type, int key, const char* param, int param_len);
    int (*on_callback_invite)(const task_t* task);
    int (*on_callback_cancel)(const task_t* task);

    int (*on_callback_record)(int start);

    int (*on_callback_gm_signature)(int start);
    int (*on_callback_gm_encrypt)(int start);
} ua_config_t;

typedef struct ua_mobilepos_s {
    char time[32];
    double longitude;
    double latitude;
    double speed;
    double direction;
    double altitude;
} ua_mobilepos_t;

typedef enum {
    ALARM_STATUS_ONDUTY,
    ALARM_STATUS_OFFDUTY,
    ALARM_STATUS_ALARM
} alarm_duty_status;

// 报警方式
typedef enum {
    ALL,            // 0-全部
    PHONE = 1,      // 1-电话报警
    DEVICE,         // 2-设备报警
    MESSAGE,        // 3-短信报警
    GPS,            // 4-GPS报警
    VIDEO,          // 5-视频报警 --> VideoAlarmType
    DEVICE_FAULT,   // 6-设备故障报警
    OTHER,          // 7-其他报警
} alarm_method;

// 视频报警类型
typedef enum {
    MANUAL = 1,     // 1-人工视频报警
    MOTION_OBJECT,  // 2-运动目标检测报警
    LEFTOVER,       // 3-遗留物检测报警
    OBJECT_REMOVE,  // 4-物体移除检测报警
    TRIPWIRE,       // 5-绊线检测报警
    INTRUSION,      // 6-入侵检测报警
    RETROGRADE,     // 7-逆行检测报警
    WANDERING,      // 8-徘徊检测报警
    TRAFFIC_STAT,   // 9-流量统计报警
    DENSITY,        // 10-密度检测报警
    VIDEO_ANOMALY,  // 11-视频异常检测报警
    FAST_MOVE,      // 12-快速移动报警
} video_alarm_type;

// 故障报警类型
typedef enum {
    STORAGE_DISK = 1, // 1-存储设备磁盘故障报警
    STORAGE_FAN,      // 2-存储设备风扇故障报警
} defect_alarm_type;

typedef enum {
    VIDEO_LOSS = 1,      // 1-视频丢失报警
    DISASSEMBLY_PREVENT, // 2-设备防拆报警
    STORAGE_FULL,        // 3-存储设备磁盘满报警
    HIGH_TEMPERATURE,    // 4-设备高温报警
    LOW_TEMPERATURE,     // 5-设备低温报警
} device_alarm_type;

typedef struct ua_alarm_query_s {
    /**
     * 报警起止级别(可选)(StartAlarmPriority EndAlarmPriority)
     * 0为全部,1为一级警情,2为二级警情,3为三级警情,4为四级警情
     */
    char start_priority[8], end_priority[8];
    /**
     * 报警方式条件(可选)(AlarmMethod)
     * 取值0为全部,1为电话报警,2为设备报警,3为短信报警,4为 GPS报警,5为视频报警,6为设备故障报警,7其他报警
     * 可以为直接组合如12为电话报警或设备报警
     */
    char method[8];
    /**
     * 报警类型(必选)(AlarmType)
     */
    char type[8];
    /**
     * 报警发生起止时间(可选)(StartAlarmTime EndAlarmTime)
	 * 如: 2010-11-11T19:46:17
     */
    char start_time[24], end_time[24];
} ua_alarm_query_t;

typedef struct ua_alarm_s {
    /**
     * 报警级别(必选)(AlarmPriority)
     * 1为一级警情,2为二级警情,3为三级警情,4为四级警情
     */
    char priority[8];
    /**
     * 报警方式(必选)(AlarmMethod)
     * 取值1为电话报警,2为设备报警,3为短信报警,4为 GPS报警,5为视频报警,6为设备故障报警,7其他报警
     *
     * 注: 设备发送报警方式为2的"设备报警"通知后,平台需进行 A.2.3a)"报警复位"控制操作,设备才能发送新的"设备报警"通知
     */
    char method[8];
    /**
     * 报警时间(必选)(AlarmTime)
     * 如: 2009-12-04T16:23:32
     */
    char time[32];
    /**
     * 报警内容描述(可选)(AlarmDescription)
     */
    char desc[2048];
    /**
     * 经纬度信息(可选)(Longitude Latitude)
     */
    double longitude, latitude;
    /**
     * 报警类型(可选)(AlarmType)
     * 报警方式为2时,不携带 AlarmType 为默认的设备报警,携带 AlarmType 取值及对应报警类型为: device_alarm_type
     * 报警方式为5时,取值为: VideoAlarmType
     * 报警方式为6时,取值为: DefectAlarmType
     */
    int type;
    /**
     * 报警类型扩展参数(可选)(AlarmTypeParam)
     * 在入侵检测报警时可携带<EventType>事件类型</EventType>,取值:1-进入区域;2-离开区域。
     */
    int event_type;
} ua_alarm_t;


// 录像产生类型
typedef enum {
    REC_TYPE_ALL,
    REC_TYPE_TIME,
    REC_TYPE_ALARM,
    REC_TYPE_MANUAL,
} record_type;

typedef struct record_info_s {
    char start_time[24];  // 录像起止时间(可选) 2010-11-11T19:46:17
    char end_time[24];
    char file_path[1024]; // 文件路径名(可选)
    char address[1024];   // 录像地址(可选 支持不完全查询)
    int secrecy;          // 保密属性(可选)缺省为0; 0:不涉密,1:涉密
    record_type type;     // 录像产生类型(可选)
    char recorder_id[24]; // 录像触发者ID(可选)
    /**
     * 录像模糊查询属性(可选, string)缺省为0;
     * 0:不进行模糊查询,此时根据SIP消息中To头域URI中的ID值确定查询录像位置,
     *  若ID值为本域系统ID则进行中心历史记录检索,若为前端设备ID则进行前端设备历史记录检索;
     * 1:进行模糊查询,此时设备所在域应同时进行中心检索和前端检索并将结果统一返回。
     */
    int indistinct_query;

    // 上面字段是查询，下面字段是响应
    char name[64];      // 设备/区域名称(必选)
    char file_size[24]; // 录像文件大小Byte(可选) 注：查询请求时没有此字段
} record_info_t;


/**
 * 获取录制文件全名
 *
 * @param path
 * @param file_name
 * @return
 */
static char* get_record_full_name(const char *path, const char *file_name)
{
    size_t path_size = strlen(path) + strlen(file_name) + strlen(MEDIA_FORMAT_PS) + 1;
    char *full_name = (char *) malloc(path_size + 1);
    strcpy(full_name, path);
    strcat(full_name, "/");
    strcat(full_name, file_name);
    strcat(full_name, MEDIA_FORMAT_PS);
    return full_name;
}

#endif // NANGBD_UA_CONFIG_H
