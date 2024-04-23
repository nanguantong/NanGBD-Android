package com.example.nan.gb28181_android.utils;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.text.TextUtils;

import com.example.nan.gb28181_android.NanGBDApplication;
import com.nan.gbd.library.osd.OsdConfig;
import com.nan.gbd.library.sip.GBDevice;
import com.nan.gbd.library.sip.SipConfig;

import java.io.IOException;
import java.net.ServerSocket;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * @author NanGBD
 * @date 2019.6.8
 */
public class SPUtil {

    public static final String KEY_CONFIG_CHANGED = "key_config_changed";
    public static final String KEY_SERVER_CERT = "key_server_cert";

    private static int mLocalPort;

    /**
     * 初始化本地设备信令绑定端口号
     * @return 本地端口号
     */
    public static int initLocalPort() {
        if (mLocalPort != 0) {
            return mLocalPort;
        }
        try (ServerSocket serverSocket = new ServerSocket(0)) {
            mLocalPort = serverSocket.getLocalPort();
        } catch (IOException e) {
            mLocalPort = 5061;
        }
        return mLocalPort;
    }

    public static SharedPreferences getSharedPreferences() {
        return getSharedPreferences(NanGBDApplication.getNanApplication());
    }

    public static SharedPreferences getSharedPreferences(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context);
    }

    /**
     * 设置sip配置项
     * @param sipConfig
     */
    public static void setSipConfig(SipConfig sipConfig) {
        GBDevice gbDevice;
        if (null == sipConfig || null == (gbDevice = sipConfig.getGbDevice())) {
            return;
        }
        getSharedPreferences()
                .edit()
                .putString("serverIp", sipConfig.getServerIp())
                .putInt("serverPort", sipConfig.getServerPort())
                .putString("serverId", sipConfig.getServerId())
                .putString("serverDomain", sipConfig.getServerDomain())
                .putString("serverCert", sipConfig.getServerCert())

                .putInt("gmLevel", gbDevice.getGmLevel().ordinal())
                .putString("deviceName", gbDevice.getName())
                .putString("manufacturer", gbDevice.getManufacturer())
                .putString("model", gbDevice.getModel())
                .putString("firmware", gbDevice.getFirmware())
                .putString("serialNumber", gbDevice.getSerialNumber())
                .putInt("channel", gbDevice.getChannel())
                .putString("userName", gbDevice.getUsername())
                .putString("userId", gbDevice.getUserid())
                .putString("password", gbDevice.getPassword())
                .putInt("devicePort", gbDevice.getPort())
                .putInt("protocol", gbDevice.getProtocol().getValue())
                .putInt("inviteCount", gbDevice.getInviteCount())
                .putInt("regExpiry", gbDevice.getRegExpiry())
                .putInt("heartbeatInterval", gbDevice.getHeartbeatInterval())
                .putInt("heartbeatCount", gbDevice.getHeartbeatCount())
                .putFloat("longitude", (float) (gbDevice.getLongitude() != null ? gbDevice.getLongitude() : GBDevice.INVALID_LONGITUDE))
                .putFloat("longitude", (float) (gbDevice.getLatitude() != null ? gbDevice.getLatitude() : GBDevice.INVALID_LATITUDE))
                .putString("videoId", gbDevice.getVideoId())
                .putString("audioId", gbDevice.getAudioId())
                .putString("alarmId", gbDevice.getAlarmId())
                .putString("charset", gbDevice.getCharset().getValue())
                .apply();
    }

    /**
     * 获取sip配置项
     * @return SipConfig
     */
    public static SipConfig getSipConfig() {
        SharedPreferences sp = getSharedPreferences();

        GBDevice gbDevice = new GBDevice();
        gbDevice.setGmLevel(GBDevice.GmLevel.get(sp.getInt("gmLevel", 0)));
        gbDevice.setName(sp.getString("deviceName", "NanGBD"));
        gbDevice.setManufacturer(sp.getString("manufacturer", "nanuns"));
        gbDevice.setModel(sp.getString("model", ""));
        gbDevice.setFirmware(sp.getString("firmware", ""));
        gbDevice.setSerialNumber(sp.getString("serialNumber", ""));
        gbDevice.setChannel(sp.getInt("channel", 1));
        gbDevice.setUsername(sp.getString("userName", "34020000001320000001"));
        gbDevice.setUserid(sp.getString("userId", "34020000001320000001"));
        gbDevice.setPassword(sp.getString("password", "12345678"));
        gbDevice.setPort(sp.getInt("devicePort", initLocalPort()));
        gbDevice.setProtocol(sp.getInt("protocol", GBDevice.Protocol.UDP.getValue()) ==
                GBDevice.Protocol.UDP.getValue() ? GBDevice.Protocol.UDP : GBDevice.Protocol.TCP);
        gbDevice.setInviteCount(sp.getInt("inviteCount", 1));
        gbDevice.setVoiceCount(sp.getInt("voiceCount", 1));
        gbDevice.setRegExpiry(sp.getInt("regExpiry", 3600));
        gbDevice.setHeartbeatInterval(sp.getInt("heartbeatInterval", 30));
        gbDevice.setHeartbeatCount(sp.getInt("heartbeatCount", 3));
        gbDevice.setVideoId(sp.getString("videoId", "34020000001320000001"));
        gbDevice.setAudioId(sp.getString("audioId", "34020000001370000001"));
        gbDevice.setAlarmId(sp.getString("alarmId", "34020000001340000001"));
        gbDevice.setCharset(sp.getString("charset", GBDevice.Charset.GB2312.getValue()).equals
                (GBDevice.Charset.GB2312.getValue()) ? GBDevice.Charset.GB2312 : GBDevice.Charset.UTF8);
        gbDevice.setLongitude((double) sp.getFloat("longitude", (float) GBDevice.INVALID_LONGITUDE));
        gbDevice.setLatitude((double) sp.getFloat("latitude", (float) GBDevice.INVALID_LATITUDE));

        SipConfig sipConfig = new SipConfig();
        sipConfig.setServerIp(sp.getString("serverIp", "192.168.1.11"));
        sipConfig.setServerPort(sp.getInt("serverPort", 15060));
        sipConfig.setServerId(sp.getString("serverId", "34020000002000000001"));
        sipConfig.setServerDomain(sp.getString("serverDomain", "3402000000"));
        sipConfig.setServerCert(sp.getString("serverCert", ""));
        sipConfig.setGbDevice(gbDevice);

        /////////////////////////// test ///////////////////////////
        gbDevice.setName(sp.getString("deviceName", "NANGBD")); // ZJ_TEST2
        gbDevice.setManufacturer(sp.getString("manufacturer", "nanuns")); // GZNFDL
        gbDevice.setModel(sp.getString("model", "HW-IH-0211"));
        gbDevice.setFirmware(sp.getString("firmware", "GPG1402201000543"));
        gbDevice.setSerialNumber(sp.getString("serialNumber", "GPG1402201000543"));
        /////////////////////////// test ///////////////////////////

        return sipConfig;
    }

    /**
     * 设置osd水印配置项
     * @param osdConfig
     */
    public static void setOsdConfig(OsdConfig osdConfig) {
        if (null == osdConfig) {
            return;
        }
        getSharedPreferences()
                .edit()
                .putBoolean("osdEnable", osdConfig.isEnable())
                .putInt("osdFontSize", osdConfig.getFontSize())
                .putInt("osdCharWidth", osdConfig.getCharWidth())
                .putInt("osdCharHeight", osdConfig.getCharHeight())
                .apply();
    }

    /**
     * 获取osd水印配置项
     * @return OsdConfig
     */
    public static OsdConfig getOsdConfig() {
        SharedPreferences sp = getSharedPreferences();

        OsdConfig osdConfig = new OsdConfig();
        osdConfig.setEnable(sp.getBoolean("osdEnable", true));
        osdConfig.setFontSize(sp.getInt("osdFontSize", 15));
        osdConfig.setCharWidth(sp.getInt("osdCharWidth", 9));
        osdConfig.setCharHeight(sp.getInt("osdCharHeight", 12));

        OsdConfig.Osd osdDate = new OsdConfig.Osd();
        osdDate.text = "%Y-%m-%d %H:%M:%S";
        osdDate.x = 20; osdDate.y = 50; osdDate.dateLen = 19;
        osdConfig.addOsd(osdDate);

        OsdConfig.Osd osdDeviceName = new OsdConfig.Osd();
        osdDeviceName.text = sp.getString("deviceName", "NANGBD");
        osdDeviceName.x = -((osdDeviceName.text.length() * osdConfig.getCharWidth()) + 20);
        osdDeviceName.y = -50;
        osdConfig.addOsd(osdDeviceName);

        return osdConfig;
    }

    /* ============================ 使能摄像头后台采集 ============================ */
    private static final String KEY_ENABLE_BACKGROUND_CAMERA = "key_enable_background_camera";

    /**
     * 获取是否使能摄像头后台采集
     * @param context
     * @return true：启用，false：禁用
     */
    public static boolean getEnableBackgroundCamera(Context context) {
        return getSharedPreferences(context)
                .getBoolean(KEY_ENABLE_BACKGROUND_CAMERA, false);
    }

    /**
     * 设置是否使能摄像头后台采集
     * @param context
     * @param value
     */
    public static void setEnableBackgroundCamera(Context context, boolean value) {
        getSharedPreferences(context)
                .edit()
                .putBoolean(KEY_ENABLE_BACKGROUND_CAMERA, value)
                .apply();
    }

    /* ============================ 音频采集长开 ============================ */
    private static final String KEY_AUDIO_STARTUP = "key_audio_startup";

    /**
     * 获取是否音频采集长开保持
     * @param context
     * @return true：启用，false：禁用
     */
    public static boolean getAudioStartup(Context context) {
        return getSharedPreferences(context)
                .getBoolean(KEY_AUDIO_STARTUP, false);
    }

    /**
     * 设置是否音频采集长开保持
     * @param context
     * @param value 是否长开
     */
    public static void setAudioStartup(Context context, boolean value) {
        getSharedPreferences(context)
                .edit()
                .putBoolean(KEY_AUDIO_STARTUP, value)
                .apply();
    }

    /* ============================ 推送视频 ============================ */
    private static final String KEY_ENABLE_VIDEO = "key_enable_video";

    /**
     * 是否使能推送视频
     * @param context
     * @return true：启用，false：禁用
     */
    public static boolean getEnableVideo(Context context) {
        return getSharedPreferences(context)
                .getBoolean(KEY_ENABLE_VIDEO, true);
    }

    /**
     * 设置使能推送视频
     * @param context
     * @param value true: 能推送视频
     */
    public static void setEnableVideo(Context context, boolean value) {
        getSharedPreferences(context)
                .edit()
                .putBoolean(KEY_ENABLE_VIDEO, value)
                .apply();
    }

    /* ============================ 推送音频 ============================ */
    private static final String KEY_ENABLE_AUDIO = "key_enable_audio";

    /**
     * 是否使能推送视频
     * @param context
     * @return true：启用，false：禁用
     */
    public static boolean getEnableAudio(Context context) {
        return getSharedPreferences(context)
                .getBoolean(KEY_ENABLE_AUDIO, false);
    }

    /**
     * 设置使能推送音频
     * @param context
     * @param value true: 能推送音频
     */
    public static void setEnableAudio(Context context, boolean value) {
        getSharedPreferences(context)
                .edit()
                .putBoolean(KEY_ENABLE_AUDIO, value)
                .apply();
    }

    /* ========================= 视频编码器 ======================= */
    private static final String KEY_VIDEO_ENCODEC = "key_video_encodec";

    /**
     * 获取视频编码器列表
     * @return 视频编码器列表
     */
    public static List<String> getVideoEncodecs() {
        List<String> encodecs = new ArrayList<>();
        encodecs.add("h264");
        encodecs.add("h265");
        return encodecs;
    }

    /**
     * 获取视频编码器
     * @param context
     * @return 视频编码器
     */
    public static String getVideoEncodec(Context context) {
        return getSharedPreferences(context)
                .getString(KEY_VIDEO_ENCODEC, "h264");
    }

    /**
     * 设置视频编码器
     * @param context
     * @param value 视频编码器值
     */
    public static void setVideoEncodec(Context context, String value) {
        getSharedPreferences(context)
                .edit()
                .putString(KEY_VIDEO_ENCODEC, value)
                .apply();
    }

    /* ============================ 码率 ============================ */
    private static final String KEY_BITRATE_ADDED_KBPS = "key_bitrate_kbps";

    /**
     * 获取视频编码码率
     * @param context
     * @return 视频编码码率
     */
    public static int getBitrateKbps(Context context) {
        return getSharedPreferences(context)
                .getInt(KEY_BITRATE_ADDED_KBPS, 300000);
    }

    /**
     * 设置视频编码码率
     * @param context
     * @param value 码率值
     */
    public static void setBitrateKbps(Context context, int value) {
        getSharedPreferences(context)
                .edit()
                .putInt(KEY_BITRATE_ADDED_KBPS, value)
                .apply();
    }

    /* ========================= 码率控制模式 ======================= */
    private static final String KEY_BITRATE_MODE = "key_bitrate_mode";

    /**
     * 获取视频编码码率控制列表
     * @return 视频编码码率控制列表
     */
    public static List<String> getVideoBitrateModes() {
        List<String> modes = new ArrayList<>();
        modes.add("cq");
        modes.add("vbr");
        modes.add("cbr");
        return modes;
    }

    /**
     * 获取视频编码码率控制模式
     * @param context
     * @return 视频编码码率控制模式
     */
    public static String getBitrateMode(Context context) {
        return getSharedPreferences(context)
                .getString(KEY_BITRATE_MODE, "cq");
    }

    /**
     * 设置视频编码码率控制模式
     * @param context
     * @param value 码率控制值
     */
    public static void setBitrateMode(Context context, String value) {
        getSharedPreferences(context)
                .edit()
                .putString(KEY_BITRATE_MODE, value)
                .apply();
    }

    /* ============================ 帧率 ============================ */
    private static final String KEY_FRAMERATE = "key_framerate";

    /**
     * 获取视频编码帧率
     * @param context
     * @return 视频编码帧率
     */
    public static int getFramerate(Context context) {
        return getSharedPreferences(context)
                .getInt(KEY_FRAMERATE, 25);
    }

    /**
     * 设置视频编码帧率
     * @param context
     * @param value 帧率值
     */
    public static void setFramerate(Context context, int value) {
        getSharedPreferences(context)
                .edit()
                .putInt(KEY_FRAMERATE, value)
                .apply();
    }

    /* ============================ 视频profile ============================ */
    private static final String KEY_VPROFILE = "key_vprofile";

    /**
     * 获取视频编码profile列表
     * @return 视频编码profile列表
     */
    public static List<String> getVideoProfiles() {
        List<String> profiles = new ArrayList<>();
        profiles.add("baseline");
        profiles.add("main");
        profiles.add("high");
        return profiles;
    }

    /**
     * 获取视频编码profile
     * @param context
     * @return
     */
    public static String getVideoProfile(Context context) {
        return getSharedPreferences(context)
                .getString(KEY_VPROFILE, "baseline");
    }

    /**
     * 设置视频编码profile
     * @param context
     * @param value 视频编码profile
     */
    public static void setVideoProfile(Context context, String value) {
        getSharedPreferences(context)
                .edit()
                .putString(KEY_VPROFILE, value)
                .apply();
    }

    /* ============================ 线程数 ============================ */
    private static final String KEY_THREADNUM = "key_threadnum";

    /**
     * 获取视频编码线程数
     * @param context
     * @return 视频编码线程数
     */
    public static int getThreadNum(Context context) {
        return getSharedPreferences(context)
                .getInt(KEY_THREADNUM, -1);
    }

    /**
     * 设置视频编码线程数
     * @param context
     * @param value 线程数
     */
    public static void setThreadNum(Context context, int value) {
        getSharedPreferences(context)
                .edit()
                .putInt(KEY_THREADNUM, value)
                .apply();
    }

    /* ============================ 摄像头支持的分辨率 ============================ */
    private static final String KEY_CAMERA_RESOLUTION = "key_camera_resolution";

    /**
     * 获取摄像头支持的分辨率
     * @param context
     * @return 摄像头支持的分辨率
     */
    public static List<String> getCameraResolution(Context context) {
        SharedPreferences sp = getSharedPreferences(context);
        List<String> resolutions = new ArrayList<>();
        String res = sp.getString(KEY_CAMERA_RESOLUTION, "");

        if (!TextUtils.isEmpty(res)) {
            String[] arr = res.split(";");
            if (arr.length > 0) {
                resolutions = Arrays.asList(arr);
            }
        }
        return resolutions;
    }

    /**
     * 设置摄像头支持的分辨率
     * @param context
     * @param value 分辨率值
     */
    public static void setCameraResolution(Context context, String value) {
        getSharedPreferences(context)
                .edit()
                .putString(KEY_CAMERA_RESOLUTION, value)
                .apply();
    }

    /* ============================ 摄像头支持的帧率 ============================ */
    private static final String KEY_CAMERA_FRAMERATES = "key_camera_framerates";

    /**
     * 获取摄像头支持的帧率列表
     * @param context
     * @return 摄像头支持的帧率列表
     */
    public static List<Integer> getCameraFramerate(Context context) {
        SharedPreferences sp = getSharedPreferences(context);
        List<Integer> framerates = new ArrayList<>();
        String frs = sp.getString(KEY_CAMERA_FRAMERATES, "");

        if (!TextUtils.isEmpty(frs)) {
            String[] arr = frs.split(";");
            for (String f : arr) {
                framerates.add(Integer.parseInt(f));
            }
        }
        return framerates;
    }

    /**
     * 设置摄像头支持的帧率
     * @param context
     * @param value 帧率值
     */
    public static void setCameraFramerate(Context context, String value) {
        getSharedPreferences(context)
                .edit()
                .putString(KEY_CAMERA_FRAMERATES, value)
                .apply();
    }

    /**
     * 获取录制路径
     * @return 完整的录制路径
     */
    public static String getRecordPath() {
        return Environment.getExternalStorageDirectory()  + "/NanGBD";
    }
}
