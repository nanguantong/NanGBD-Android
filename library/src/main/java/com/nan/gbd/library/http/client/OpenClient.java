package com.nan.gbd.library.http.client;

import com.nan.gbd.library.http.response.BaseResponse;
import com.nan.gbd.library.http.config.OpenConfig;
import com.nan.gbd.library.http.request.BaseRequest;
import com.nan.gbd.library.http.request.RequestForm;
import com.nan.gbd.library.utils.JsonUtils;
import com.nan.gbd.library.utils.SignUtils;
import com.nan.gbd.library.utils.StringUtils;

import java.util.HashMap;
import java.util.Map;

/**
 * @author NanGBD
 * @date 2020.3.8
 */
public class OpenClient {
    private static final String ACCEPT_LANGUAGE = "Accept-Language";
    private static final String AUTHORIZATION = "Authorization";
    private static final String PREFIX_BEARER = "Bearer ";
    private static final OpenConfig DEFAULT_CONFIG = new OpenConfig();

    private String url;
    private String appKey;
    private String secret;

    private OpenConfig openConfig;
    private OpenRequest openRequest;

    public OpenClient(String url, String appKey, String secret) {
        this(url, appKey, secret, DEFAULT_CONFIG);
    }

    public OpenClient(String url, String appKey, String secret, OpenConfig openConfig) {
        if (openConfig == null) {
            throw new IllegalArgumentException("openConfig不能为null");
        }
        this.url = url;
        this.appKey = appKey;
        this.secret = secret;
        this.openConfig = openConfig;

        this.openRequest = new OpenRequest(openConfig);
    }

    /**
     * 请求接口
     * @param request 请求对象
     * @param <T> 返回对应的Response
     * @return 返回Response
     */
    public <T extends BaseResponse<?>> T execute(BaseRequest<T> request) {
        return this.execute(request, null);
    }

    /**
     * 请求接口
     * @param request 请求对象
     * @param jwt jwt
     * @param <T> 返回对应的Response
     * @return 返回Response
     */
    public <T extends BaseResponse<?>> T execute(BaseRequest<T> request, String jwt) {
        RequestForm requestForm = request.createRequestForm();
        // 表单数据
        Map<String, Object> form = requestForm.getForm();
        // 检查accessToken是否存在，防止序列化后json不识别
        Object objAccessToken = form.get(this.openConfig.getAccessTokenName());
        if (null == objAccessToken) {
            form.put(this.openConfig.getAccessTokenName(), request.getAccess_token());
        }

        form.put(this.openConfig.getAppKeyName(), this.appKey);
        // 将data部分转成json并urlencode
        Object data = form.get(this.openConfig.getDataName());
        String dataJson = JsonUtils.toJSONString(data);
        dataJson = StringUtils.encodeUrl(dataJson);
        form.put(this.openConfig.getDataName(), dataJson);
        // 生成签名，并加入到form中
        String sign = SignUtils.createSign(form, this.secret);
        form.put(this.openConfig.getSignName(), sign);

        // 构建http请求header
        Map<String, String> header = this.buildHeader(jwt);
        // 执行请求
        String resp = doExecute(this.url, requestForm, header);

        return this.parseResponse(resp, request);
    }

    protected String doExecute(String url, RequestForm requestForm, Map<String, String> header) {
        return openRequest.request(this.url, requestForm, header);
    }

    protected <T extends BaseResponse<?>> T parseResponse(String resp, BaseRequest<T> request) {
        return JsonUtils.parseObject(resp, request.getResponseClass());
    }

    protected Map<String, String> buildHeader(String jwt) {
        Map<String, String> header = new HashMap<String, String>();
        header.put(ACCEPT_LANGUAGE, this.openConfig.getLocale());
        if (StringUtils.isNotEmpty(jwt)) {
            header.put(AUTHORIZATION, PREFIX_BEARER + jwt);
        }
        return header;
    }

    public OpenRequest getOpenRequest() {
        return openRequest;
    }

    public void setOpenRequest(OpenRequest openRequest) {
        this.openRequest = openRequest;
    }

    public String getAppKey() {
        return appKey;
    }

    public void setAppKey(String appKey) {
        this.appKey = appKey;
    }

    public OpenConfig getOpenConfig() {
        return openConfig;
    }
}
