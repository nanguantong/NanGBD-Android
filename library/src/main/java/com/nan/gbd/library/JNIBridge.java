package com.nan.gbd.library;

import android.graphics.Bitmap;

import com.nan.gbd.library.osd.OsdConfig;
import com.nan.gbd.library.sip.GBEvent;

/**
 * @author NanGBD
 * @date 2020.6.8
 */
public class JNIBridge {

    static {
        System.loadLibrary("NanGBD");
    }

    public final static int UDP = 0;
    public final static int TCP = 1;
    public final static int FILE = 2;

    private static StatusCallback statusCallback;
    public static void setStatusCallback(StatusCallback cb) {
        statusCallback = cb;
    }

    /**
     * 回调函数
     *
     * @param eventType 事件类型
     * @param param 参数数组
     * @param paramLen 参数数组长度
     * @return 0:成功, 否则失败
     */
    public static int onGBDeviceCallback(int eventType, int key, byte[] param, int paramLen) {
        if (statusCallback != null) {
            return statusCallback.onCallback(eventType, GBEvent.getName(eventType), key, param) ? 0 : -1;
        }
        return 0;
    }

    /**
     * 初始化osd水印
     * @param enable     是否开启水印
     * @param charWidth  水印单个字符宽度
     * @param charHeight 水印单个字符高度
     * @param osds       osd水印列表
     * @param chars      预生成的字符集yuv数据列表
     * @return 0:成功, 否则失败
     */
    public static native int nanOsdInit(boolean enable, int charWidth, int charHeight, OsdConfig.Osd[] osds, byte[][] chars);

    /**
     * 反初始化osd水印
     * @return
     */
    public static native int nanOsdUninit();

    /**
     * 初始化设备信令配置
     *
     * @param serverIp          SIP服务器地址
     * @param serverPort        SIP服务器端口
     * @param serverId          SIP服务器ID
     * @param serverDomain      SIP服务器域
     * @param gmLevel           国密35114安全等级
     * @param name              设备名称
     * @param manufacturer      设备生产商
     * @param model             设备型号
     * @param firmware          设备固件版本
     * @param serialNumber      设备序列号
     * @param channel           视频输入通道数
     * @param username          SIP用户名
     * @param userid            SIP用户认证ID
     * @param password          SIP用户认证密码
     * @param localPort         设备端口
     * @param protocol          信令传输协议
     * @param expiry            注册有效期(秒), 范围3600-100000
     * @param heartbeatInterval 心跳周期(秒), 范围5-255
     * @param heartbeatCount    最大心跳超时次数, 范围3-255
     * @param longitude         经度
     * @param latitude          纬度
     * @param videoId           视频通道编码ID
     * @param audioId           语音输出通道编码ID
     * @param alarmId           报警通道编码ID
     * @param maxInviteCount    最大支持并发取流数
     * @param maxVoiceCount     最大支持并发语音对讲数
     * @param posCapability     定位功能支持情况, 取值:0-不支持;1-支持GPS定位;2-支持北斗定位(可选, 默认取值为0)
     * @param globalAuth        是否使用全局注册认证信息
     * @return 0:成功, 否则失败
     */
    public static native int nanUaInit(String serverIp, int serverPort, String serverId, String serverDomain,
                                       int gmLevel, String name, String manufacturer, String model, String firmware, String serialNumber,
                                       int channel, String username, String userid, String password, int localPort,
                                       String protocol, int expiry, int heartbeatInterval, int heartbeatCount,
                                       double longitude, double latitude,
                                       String videoId, String audioId, String alarmId, String charset,
                                       int maxInviteCount, int maxVoiceCount, int posCapability, int globalAuth);

    /**
     * 注册设备到国标服务平台
     * @return 0:成功, 否则失败
     */
    public static native int nanUaRegister();

    /**
     * 注销从国标国标服务平台
     * @return 0:成功, 否则失败
     */
    public static native int nanUaUnregister();

    /**
     * 设备是否注册成功到国标服务平台
     * @return true:已注册, 否则未注册
     */
    public static native boolean nanUaIsRegistered();

    /**
     * 通知设备配置信息是否发生变化
     * @param changed
     */
    public static native void nanUaOnConfigChanged(boolean changed);

    /**
     * 获取设备证书
     * @return 设备证书
     */
    public static native String nanUaGetDeviceCert();

    /**
     * 设置服务证书
     * @param serverId  服务国标id
     * @param cert      服务证书
     * @param isFile    证书是文件还是内存
     * @param passin    证书密码
     * @return 0:成功, 否则失败
     */
    public static native int nanUaSetServerCert(String serverId, String cert, boolean isFile, String passin);

    /**
     * 设置或更新设备移动位置信息
     * @param time        产生通知时间(必选), 格式: 2018-02-10T19:47:59
     * @param longitude   经度(必选)
     * @param latitude    纬度(必选)
     * @param speed       速度,单位:km/h(可选)
     * @param direction   方向,取值为当前摄像头方向与正北方的顺时针夹角,取值范围0°~360°,单位:(°)(可选)
     * @param altitude    海拔高度,单位:m(可选)
     * @return 0:成功, 否则失败
     */
    public static native int nanUaSetMobilePos(String time, double longitude, double latitude,
                                               double speed, double direction, double altitude);

    /**
     * 设置设备报警信息
     * @param priority   报警级别(必选), 0为全部,1为一级警情,2为二级警情,3为三级警情,4为四级警情
     * @param method     报警方式(必选), 1为电话报警,2为设备报警,3为短信报警,4为 GPS报警,5为视频报警,6为设备故障报警,7其他报警
     * @param time       报警时间(必选) 如: 2009-12-04T16:23:32
     * @param desc       报警内容描述(可选)
     * @param longitude  经度信息(可选) (-1代表此参数无效)
     * @param latitude   纬度信息(可选) (-1代表此参数无效)
     * @param alarm_type 报警类型(可选) (-1代表此参数无效)
     *                   报警方式为2时,取值如下:
     *                       1-视频丢失报警;2-设备防拆报警;3-存储设备磁盘满报警;4-设备高温报警;5-设备低温报警。
     *                   报警方式为5时,取值如下:
     *                       1-人工视频报警;2-运动目标检测报警;3-遗留物检测报警;4-物体移除检测报警;5-绊线检测报警;
     *                       6-入侵检测报警;7-逆行检测报警;8-徘徊检测报警;9-流量统计报警;10-密度检测报警;
     *                       11-视频异常检测报警;12-快速移动报警。
     *                   报警方式为6时,取值如下:
     *                       1-存储设备磁盘故障报警;2-存储设备风扇故障报警。
     * @param event_type 报警类型扩展参数(可选) (-1代表此参数无效)
     *                      在入侵检测报警时可携带此参数,取值:1-进入区域;2-离开区域。
     * @return 0:成功, 否则失败
     */
    public static native int nanUaSetAlarm(String priority, String method, String time, String desc,
                                           double longitude, double latitude,
                                           int alarm_type, int event_type);

    /**
     * 取消初始化国标设备
     * @return 0:成功, 否则失败
     */
    public static native int nanUaUninit();


    /**
     * 初始化音视频封装
     *
     * @param mediaPath                视频存放目录
     * @param mediaName                视频名称
     * @param recDuration              录制时长(秒),[0, 60*60]
     * @param enableVideo              使能视频
     * @param enableAudio              使能音频
     * @param videoEncodec             视频编码器(0 / 1 / 2)
     * @param videoRotate              视频旋转角度
     * @param inWidth                  视频输入宽度
     * @param inHeight                 视频输入高度
     * @param outWidth                 视频输出宽度
     * @param outHeight                视频输出高度
     * @param framerate                视频帧率
     * @param gop                      视频gop([1, 10] 秒)
     * @param videoBitrate             视频比特率
     * @param videoBitrateMode         视频比特率控制模式(0 / 1 / 2)
     * @param videoProfile             视频profile (1 / 2 / 3)
     * @param audioFrameLen            音频长度
     * @param threadNum                编码线程数([0, 8])
     * @param queueMax                 视频队列最大长度
     * @param videoBitrateFactor       视频比特率上下浮动因子[0, 1]
     * @param videoBitrateAdjustPeriod 视频比特率动态调整周期(秒)
     * @param streamStatPeriod         流状态统计周期(秒)
     * @return 0:成功, 否则失败
     */
    public static native int nanInitMuxer(String mediaPath, String mediaName, int recDuration,
                                          boolean enableVideo, boolean enableAudio,
                                          int videoEncodec, int videoRotate,
                                          int inWidth, int inHeight, int outWidth, int outHeight,
                                          int framerate, int gop, long videoBitrate, int videoBitrateMode,
                                          int videoProfile, int audioFrameLen, int threadNum, int queueMax,
                                          double videoBitrateFactor, int videoBitrateAdjustPeriod, int streamStatPeriod);

    /**
     * 推送视频帧
     * @param data     一帧视频数据
     * @param ts       采集帧时间(毫秒)
     * @param keyframe 是否是关键帧
     * @return 0:成功, 否则失败
     */
    public static native int nanPushVideoFrame(byte[] data, long ts, boolean keyframe);

    /**
     * 推送音频帧
     * @param data 一帧音频sample数据
     * @param pts  采集帧时间(毫秒)
     * @return 0:成功, 否则失败
     */
    public static native int nanPushAudioFrame(byte[] data, long pts);

    /**
     * 结束音视频封装
     * @return 0:成功, 否则失败
     */
    public static native int nanEndMuxer();


    ///////////////////////////////////////////////////////////////////////////

    /**
     * nv12 与 nv21区别
     * NV12: YYYYYYYY UVUV     =>YUV420SP
     * NV21: YYYYYYYY VUVU     =>YUV420SP
     * rgb 转 nv21
     *
     * @param argb
     * @param width
     * @param height
     * @return
     */
    public static native byte[] argbIntToNV21Byte(int[] argb, int width, int height);

    /**
     * rgb 转 nv12
     *
     * @param argb
     * @param width
     * @param height
     * @return
     */
    public static native byte[] argbIntToNV12Byte(int[] argb, int width, int height);

    /**
     * rgb 转灰度 nv
     * 也就是yuv 中只有 yyyy 没有uv 数据
     *
     * @param argb
     * @param width
     * @param height
     * @return
     */
    public static native byte[] argbIntToGrayNVByte(int[] argb, int width, int height);

    /**
     * nv21 转 nv12
     *
     * @param nv21Src  源数据
     * @param nv12Dest 目标数组
     * @param width    数组长度 len=width*height*3/2
     * @param height
     */
    public static native void nv21ToNv12(byte[] nv21Src, byte[] nv12Dest, int width, int height);

    /**
     * @param bitmap cannot be used after call this function
     * @param width  the width of bitmap
     * @param height the height of bitmap
     * @return return the NV21 byte array, length = width * height * 3 / 2
     */
    public static byte[] bitmapToNV21(Bitmap bitmap, int width, int height) {
        int[] argb = new int[width * height];
        bitmap.getPixels(argb, 0, width, 0, 0, width, height);
        byte[] nv21 = argbIntToNV21Byte(argb, width, height);
        return nv21;
    }

    /**
     * @param bitmap cannot be used after call this function
     * @param width  the width of bitmap
     * @param height the height of bitmap
     * @return return the NV12 byte array, length = width * height * 3 / 2
     */
    public static byte[] bitmapToNV12(Bitmap bitmap, int width, int height) {
        int[] argb = new int[width * height];
        bitmap.getPixels(argb, 0, width, 0, 0, width, height);
        byte[] nv12 = argbIntToNV12Byte(argb, width, height);
        return nv12;
    }

    /**
     * @param bitmap cannot be used after call this function
     * @param width  the width of bitmap
     * @param height the height of bitmap
     * @return return the NV12 byte array, length = width * height
     */
    public static byte[] bitmapToGrayNV(Bitmap bitmap, int width, int height) {
        int[] argb = new int[width * height];
        bitmap.getPixels(argb, 0, width, 0, 0, width, height);
        byte[] nv12 = argbIntToGrayNVByte(argb, width, height);
        return nv12;
    }
}
