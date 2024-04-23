package com.nan.gbd.library.auth;

/**
 * @author NanGBD
 * @date 2020.6.8
 */
public class AuthConfig {
    public AuthConfig() {
    }

    public AuthConfig(String imei, String appKey, String appSecret) {
        this.imei = imei;
        this.appKey = appKey;
        this.appSecret = appSecret;
    }

    public String getImei() {
        return imei;
    }

    public void setImei(String imei) {
        this.imei = imei;
    }

    public String getAppKey() {
        return appKey;
    }

    public void setAppKey(String appKey) {
        this.appKey = appKey;
    }

    public String getAppSecret() {
        return appSecret;
    }

    public void setAppSecret(String appSecret) {
        this.appSecret = appSecret;
    }

    public String getJwt() {
        return jwt;
    }

    public void setJwt(String jwt) {
        this.jwt = jwt;
    }

    private String imei;       // 设备imei信息 (必填)
    private String appKey;     // 厂家授权码key (必填)
    private String appSecret;  // 厂家授权码secret (必填)
    private String jwt;        // 厂家授权jwt信息 (可选)
}
