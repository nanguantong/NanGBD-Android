package com.nan.gbd.library.sip;

/**
 * @author NanGBD
 * @date 2020.6.8
 */
public class GBDevice {
    public static final double INVALID_LONGITUDE = -99999;
    public static final double INVALID_LATITUDE = -99999;

    public GBDevice() {
    }

    public enum Protocol {
        UDP(0),
        TCP(1);

        private final int value;

        Protocol(int v) {
            this.value = v;
        }

        public int getValue() {
            return value;
        }
    }

    public enum Charset {
        GB2312("GB2312"),
        UTF8("UTF-8");

        private final String value;

        Charset(String v) {
            this.value = v;
        }

        public String getValue() {
            return value;
        }

        public static Charset get(String value) {
            if (value.equalsIgnoreCase(Charset.GB2312.value)) {
                return GB2312;
            } else if (value.equalsIgnoreCase(Charset.UTF8.value)) {
                return UTF8;
            }
            return GB2312;
        }

        @Override
        public String toString() {
            return value;
        }
    }

    public enum GmLevel {
        N("不加密"),
        A("A类加密"),
        B("B类加密"),
        C("C类加密");

        private final String desc;

        GmLevel(String v) {
            this.desc = v;
        }

        public String getDesc() {
            return desc;
        }

        public static GmLevel get(int ordinal) {
            switch (ordinal) {
                case 0: return N;
                case 1: return A;
                case 2: return B;
                case 3: return C;
            }
            return N;
        }

        @Override
        public String toString() {
            return desc;
        }
    }

    private GmLevel gmLevel;       // 国密35114等级

    private String name;           // 设备名称
    private String manufacturer;   // 设备生产商
    private String model;          // 设备型号
    private String firmware;       // 设备固件版本
    private String serialNumber;   // 设备序列号
    private int channel;           // 视频输入通道数

    private String username;       // SIP用户名
    private String userid;         // SIP用户认证ID
    private String password;       // SIP用户认证密码
    private int port;              // 设备端口

    private Protocol protocol;     // 信令传输协议

    private int regExpiry;         // 注册有效期(秒), 范围3600-100000
    private int heartbeatInterval; // 心跳周期(秒), 范围5-255
    private int heartbeatCount;    // 最大心跳超时次数, 范围3-255

    private String videoId;        // 视频通道编码ID
    private String audioId;        // 语音输出通道编码ID
    private String alarmId;        // 报警通道编码ID

    private Charset charset;       // 字符编码类型 GB2312 / UTF-8

    private Double longitude;      // 经度, 可自定义

    private Double latitude;       // 纬度, 可自定义

    private int inviteCount;       // 最大支持并发取流数
    private int voiceCount;        // 最大支持并发语音对讲数
    private int posCapability;     // 定位功能支持情况, 取值:0-不支持;1-支持GPS定位;2-支持北斗定位(可选, 默认取值为0)
    private boolean globalAuth;    // 是否使用全局注册认证信息

    public GmLevel getGmLevel() {
        return gmLevel;
    }

    public void setGmLevel(GmLevel gmLevel) {
        this.gmLevel = gmLevel;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getManufacturer() {
        return manufacturer;
    }

    public void setManufacturer(String manufacturer) {
        this.manufacturer = manufacturer;
    }

    public String getModel() {
        return model;
    }

    public void setModel(String model) {
        this.model = model;
    }

    public String getFirmware() {
        return firmware;
    }

    public void setFirmware(String firmware) {
        this.firmware = firmware;
    }

    public String getSerialNumber() {
        return serialNumber;
    }

    public void setSerialNumber(String serialNumber) {
        this.serialNumber = serialNumber;
    }

    public int getChannel() {
        return channel;
    }

    public void setChannel(int channel) {
        this.channel = channel;
    }

    public String getUsername() {
        return username;
    }

    public void setUsername(String username) {
        this.username = username;
    }

    public String getUserid() {
        return userid;
    }

    public void setUserid(String userid) {
        this.userid = userid;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword(String password) {
        this.password = password;
    }

    public int getPort() {
        return port;
    }

    public void setPort(int port) {
        this.port = port;
    }

    public Protocol getProtocol() {
        return protocol;
    }

    public void setProtocol(Protocol protocol) {
        this.protocol = protocol;
    }

    public int getRegExpiry() {
        return regExpiry;
    }

    public void setRegExpiry(int regExpiry) {
        this.regExpiry = regExpiry;
    }

    public int getHeartbeatInterval() {
        return heartbeatInterval;
    }

    public void setHeartbeatInterval(int heartbeatInterval) {
        this.heartbeatInterval = heartbeatInterval;
    }

    public int getHeartbeatCount() {
        return heartbeatCount;
    }

    public void setHeartbeatCount(int heartbeatCount) {
        this.heartbeatCount = heartbeatCount;
    }

    public String getVideoId() {
        return videoId;
    }

    public void setVideoId(String videoId) {
        this.videoId = videoId;
    }

    public String getAudioId() {
        return audioId;
    }

    public void setAudioId(String audioId) {
        this.audioId = audioId;
    }

    public String getAlarmId() {
        return alarmId;
    }

    public void setAlarmId(String alarmId) {
        this.alarmId = alarmId;
    }

    public Charset getCharset() {
        return charset;
    }

    public void setCharset(Charset charset) {
        this.charset = charset;
    }

    public Double getLongitude() {
        return longitude;
    }

    public void setLongitude(Double longitude) {
        this.longitude = longitude;
    }

    public Double getLatitude() {
        return latitude;
    }

    public void setLatitude(Double latitude) {
        this.latitude = latitude;
    }

    public int getInviteCount() {
        return inviteCount;
    }

    public void setInviteCount(int inviteCount) {
        this.inviteCount = inviteCount;
    }

    public int getVoiceCount() {
        return voiceCount;
    }

    public void setVoiceCount(int voiceCount) {
        this.voiceCount = voiceCount;
    }

    public int getPosCapability() {
        return posCapability;
    }

    public void setPosCapability(int posCapability) {
        this.posCapability = posCapability;
    }

    public boolean getGlobalAuth() {
        return globalAuth;
    }

    public void setGlobalAuth(boolean globalAuth) {
        this.globalAuth = globalAuth;
    }
}
