package com.nan.gbd.library.http.request;

import com.alibaba.fastjson.JSON;
import com.alibaba.fastjson.JSONObject;
import com.alibaba.fastjson.annotation.JSONField;
import com.nan.gbd.library.http.response.BaseResponse;
import com.nan.gbd.library.http.config.OpenConfig;
import com.nan.gbd.library.utils.ClassUtils;

import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * 请求对象父类，后续请求对象都要继承这个类
 * @param <T> 对应的Response对象
 * @author NanGBD
 * @date 2020.3.8
 */
public abstract class BaseRequest<T extends BaseResponse<?>> {

    private static String FORMAT_TYPE = OpenConfig.FORMAT_TYPE;
    private static String TIMESTAMP_PATTERN = OpenConfig.TIMESTAMP_PATTERN;
    private static String DEFAULT_ACCESS_TOKEN = OpenConfig.DEFAULT_ACCESS_TOKEN;

    private String name;
    private String version;
    private Object data;
    private String timestamp = new SimpleDateFormat(TIMESTAMP_PATTERN).format(new Date());
    private String access_token = DEFAULT_ACCESS_TOKEN;
    private String format = FORMAT_TYPE;

    @JSONField(serialize = false)
    private RequestMethod requestMethod = RequestMethod.POST;

    @JSONField(serialize = false)
    private Class<T> responseClass;

    @JSONField(serialize = false)
    public abstract String name();

    @SuppressWarnings("unchecked")
    public BaseRequest() {
        this.name = this.name();
        this.version = this.version();

        this.responseClass = (Class<T>) ClassUtils.getSuperClassGenricType(this.getClass(), 0);
    }

    @JSONField(serialize = false)
    protected String version() {
        return OpenConfig.DEFAULT_VERSION;
    }

    public RequestForm createRequestForm() {
        String json = JSON.toJSONString(this);
        JSONObject map = JSON.parseObject(json);
        RequestForm requestForm = new RequestForm(map);
        return requestForm;
    }

    public void setParam(Object param) {
        this.setData(param);
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getVersion() {
        return version;
    }

    public void setVersion(String version) {
        this.version = version;
    }

    public Object getData() {
        return data;
    }

    /** 等同setParam() */
    public void setData(Object data) {
        this.data = data;
    }

    public String getAccess_token() {
        return access_token;
    }

    public void setAccess_token(String access_token) {
        this.access_token = access_token;
    }

    public String getFormat() {
        return format;
    }

    public void setFormat(String format) {
        this.format = format;
    }

    public RequestMethod getRequestMethod() {
        return requestMethod;
    }

    public void setRequestMethod(RequestMethod requestMethod) {
        this.requestMethod = requestMethod;
    }

    public String getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(String timestamp) {
        this.timestamp = timestamp;
    }

    public Class<T> getResponseClass() {
        return responseClass;
    }
}
