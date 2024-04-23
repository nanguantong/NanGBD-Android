package com.nan.gbd.library.sip;

/**
 * sdk事件响应
 * @author NanGBD
 * @date 2020.6.8
 */
public class GBEvent {
    public static final int GB_DEVICE_EVENT_CONNECTING = 1001;         // 注册连接中
    public static final int GB_DEVICE_EVENT_REGISTERING = 1002;        // 注册正在开始中
    public static final int GB_DEVICE_EVENT_REGISTER_OK = 1003;        // 注册成功
    public static final int GB_DEVICE_EVENT_REGISTER_AUTH_FAIL = 1004; // 注册鉴权失败
    public static final int GB_DEVICE_EVENT_UNREGISTER_OK = 1005;      // 注销成功
    public static final int GB_DEVICE_EVENT_UNREGISTER_FAILED = 1006;  // 注销失败
    public static final int GB_DEVICE_EVENT_START_VIDEO = 1007;        // 开始推流
    public static final int GB_DEVICE_EVENT_STOP_VIDEO = 1008;         // 停止推流
    public static final int GB_DEVICE_EVENT_START_VOICE = 1009;        // 开始语音
    public static final int GB_DEVICE_EVENT_STOP_VOICE = 1010;         // 停止语音
    public static final int GB_DEVICE_EVENT_TALK_AUDIO_DATA = 1011;    // 语音对讲音频数据
    public static final int GB_DEVICE_EVENT_START_RECORD = 1012;       // 开始录制
    public static final int GB_DEVICE_EVENT_STOP_RECORD = 1013;        // 停止录制
    public static final int GB_DEVICE_EVENT_STATISTICS = 1100;         // 编码和发送统计信息
    public static final int GB_DEVICE_EVENT_DISCONNECT = 2000;         // 连接断开, 可在应用层做自动重新注册

    public static String getName(int code) {
        String res;
        switch (code) {
            case GB_DEVICE_EVENT_CONNECTING:
                res = "连接中...";
                break;
            case GB_DEVICE_EVENT_REGISTERING:
                res = "注册中...";
                break;
            case GB_DEVICE_EVENT_REGISTER_OK:
                res = "注册成功";
                break;
            case GB_DEVICE_EVENT_REGISTER_AUTH_FAIL:
                res = "注册鉴权失败";
                break;
            case GB_DEVICE_EVENT_UNREGISTER_OK:
                res = "注销成功";
                break;
            case GB_DEVICE_EVENT_UNREGISTER_FAILED:
                res = "注销失败";
                break;
            case GB_DEVICE_EVENT_START_VIDEO:
                res = "开始视频：H264 G711A";
                break;
            case GB_DEVICE_EVENT_STOP_VIDEO:
                res = "停止视频";
                break;
            case GB_DEVICE_EVENT_START_VOICE:
                res = "开始语音";
                break;
            case GB_DEVICE_EVENT_STOP_VOICE:
                res = "停止语音";
                break;
            case GB_DEVICE_EVENT_TALK_AUDIO_DATA:
                res = "对讲音频数据";
                break;
            case GB_DEVICE_EVENT_START_RECORD:
                res = "开始录制";
                break;
            case GB_DEVICE_EVENT_STOP_RECORD:
                res = "停止录制";
                break;
            case GB_DEVICE_EVENT_STATISTICS:
                res = "统计信息";
                break;
            case GB_DEVICE_EVENT_DISCONNECT:
                res = "已断线";
                break;
            default:
                res = "";
                break;
        }
        return res;
    }
}
