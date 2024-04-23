package com.nan.gbd.library.http.config;

/**
 * @author NanGBD
 * @date 2020.3.8
 */
public class OpenConfig {
    public static volatile String SUCCESS_CODE = "0";

    public static volatile String DEFAULT_VERSION = "";

    public static volatile String FORMAT_TYPE = "json";

    public static volatile String TIMESTAMP_PATTERN = "yyyy-MM-dd HH:mm:ss";

    public static volatile String DEFAULT_ACCESS_TOKEN = "";

    public static volatile byte[] URL_API = new byte[] { 104,116,116,112,115,58,47,47,97,112,105,46,110,97,110,117,110,115,46,99,111,109,47,97,112,105 };

    /** 成功返回码值 */
    private String successCode = SUCCESS_CODE;
    /** 默认版本号 */
    private String defaultVersion = DEFAULT_VERSION;
    /** 接口属性名 */
    private String apiName = "name";
    /** 版本号名称 */
    private String versionName = "version";
    /** appKey名称 */
    private String appKeyName = "app_key";
    /** data名称 */
    private String dataName = "data";
    /** 时间戳名称 */
    private String timestampName = "timestamp";
    /** 时间戳格式 */
    private String timestampPattern = "yyyy-MM-dd HH:mm:ss";
    /** 签名串名称 */
    private String signName = "sign";
    /** 格式化名称 */
    private String formatName = "format";
    /** 格式类型 */
    private String formatType = "json";
    /** accessToken名称 */
    private String accessTokenName = "access_token";
    /** 国际化语言 */
    private String locale = "zh-CN";
    /** 响应code名称 */
    private String responseCodeName = "code";
    /** 请求超时时间 */
    private int connectTimeoutSeconds = 10;
    /** http读取超时时间 */
    private int readTimeoutSeconds = 10;

    public String getSuccessCode() {
        return successCode;
    }

    public void setSuccessCode(String successCode) {
        this.successCode = successCode;
    }

    public String getDefaultVersion() {
        return defaultVersion;
    }

    public void setDefaultVersion(String defaultVersion) {
        this.defaultVersion = defaultVersion;
    }

    public String getApiName() {
        return apiName;
    }

    public void setApiName(String apiName) {
        this.apiName = apiName;
    }

    public String getVersionName() {
        return versionName;
    }

    public void setVersionName(String versionName) {
        this.versionName = versionName;
    }

    public String getAppKeyName() {
        return appKeyName;
    }

    public void setAppKeyName(String appKeyName) {
        this.appKeyName = appKeyName;
    }

    public String getDataName() {
        return dataName;
    }

    public void setDataName(String dataName) {
        this.dataName = dataName;
    }

    public String getTimestampName() {
        return timestampName;
    }

    public void setTimestampName(String timestampName) {
        this.timestampName = timestampName;
    }

    public String getSignName() {
        return signName;
    }

    public void setSignName(String signName) {
        this.signName = signName;
    }

    public String getFormatName() {
        return formatName;
    }

    public void setFormatName(String formatName) {
        this.formatName = formatName;
    }

    public String getFormatType() {
        return formatType;
    }

    public void setFormatType(String formatType) {
        this.formatType = formatType;
    }

    public String getTimestampPattern() {
        return timestampPattern;
    }

    public void setTimestampPattern(String timestampPattern) {
        this.timestampPattern = timestampPattern;
    }

    public String getAccessTokenName() {
        return accessTokenName;
    }

    public void setAccessTokenName(String accessTokenName) {
        this.accessTokenName = accessTokenName;
    }

    public String getLocale() {
        return locale;
    }

    public void setLocale(String locale) {
        this.locale = locale;
    }

    public String getResponseCodeName() {
        return responseCodeName;
    }

    public void setResponseCodeName(String responseCodeName) {
        this.responseCodeName = responseCodeName;
    }

    public int getConnectTimeoutSeconds() {
        return connectTimeoutSeconds;
    }

    public void setConnectTimeoutSeconds(int connectTimeoutSeconds) {
        this.connectTimeoutSeconds = connectTimeoutSeconds;
    }

    public int getReadTimeoutSeconds() {
        return readTimeoutSeconds;
    }

    public void setReadTimeoutSeconds(int readTimeoutSeconds) {
        this.readTimeoutSeconds = readTimeoutSeconds;
    }
}
