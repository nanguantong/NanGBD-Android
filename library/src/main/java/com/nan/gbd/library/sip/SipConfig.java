package com.nan.gbd.library.sip;

/**
 * @author NanGBD
 * @date 2020.6.8
 */
public class SipConfig {
    private String serverIp;     // SIP服务器地址
    private int serverPort;      // SIP服务器端口
    private String serverId;     // SIP服务器ID
    private String serverDomain; // SIP服务器域
    private String serverCert;   // SIP服务器证书

    private GBDevice gbDevice;   // 设备配置信息

    public SipConfig() {
    }

    public String getServerIp() {
        return serverIp;
    }

    public void setServerIp(String serverIp) {
        this.serverIp = serverIp;
    }

    public int getServerPort() {
        return serverPort;
    }

    public void setServerPort(int serverPort) {
        this.serverPort = serverPort;
    }

    public String getServerId() {
        return serverId;
    }

    public void setServerId(String serverId) {
        this.serverId = serverId;
    }

    public String getServerDomain() {
        return serverDomain;
    }

    public void setServerDomain(String serverDomain) {
        this.serverDomain = serverDomain;
    }

    public String getServerCert() {
        return serverCert;
    }

    public void setServerCert(String serverCert) {
        this.serverCert = serverCert;
    }

    public GBDevice getGbDevice() {
        return gbDevice;
    }

    public void setGbDevice(GBDevice gbDevice) {
        this.gbDevice = gbDevice;
    }
}
