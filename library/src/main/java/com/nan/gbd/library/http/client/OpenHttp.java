package com.nan.gbd.library.http.client;

import com.nan.gbd.library.http.config.OpenConfig;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;

import okhttp3.Cookie;
import okhttp3.CookieJar;
import okhttp3.HttpUrl;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

/**
 * @author NanGBD
 * @date 2020.3.8
 */
public class OpenHttp {
    public static final MediaType JSON = MediaType.parse("application/json; charset=utf-8");

    private Map<String, List<Cookie>> cookieStore = new HashMap<String, List<Cookie>>();

    private OkHttpClient httpClient;
    private OpenConfig openConfig;

    public OpenHttp(OpenConfig openConfig) {
        this.openConfig = openConfig;
        this.initHttpClient(openConfig);
    }

    protected void initHttpClient(OpenConfig openConfig) {
        httpClient = new OkHttpClient.Builder()
                .connectTimeout(openConfig.getConnectTimeoutSeconds(), TimeUnit.SECONDS) // 设置链接超时时间，默认10秒
                .readTimeout(openConfig.getReadTimeoutSeconds(), TimeUnit.SECONDS)
                .cookieJar(new CookieJar() {
                    public void saveFromResponse(HttpUrl httpUrl, List<Cookie> list) {
                        cookieStore.put(httpUrl.host(), list);
                    }

                    public List<Cookie> loadForRequest(HttpUrl httpUrl) {
                        List<Cookie> cookies = cookieStore.get(httpUrl.host());
                        return cookies != null ? cookies : new ArrayList<Cookie>();
                    }
                }).build();
    }

    /**
     * get请求
     *
     * @param url
     * @param header
     * @return
     * @throws IOException
     */
    public String get(String url, Map<String, String> header) throws IOException {
        Request.Builder builder = new Request.Builder().url(url).get();
        addHeader(builder, header);

        Request request = builder.build();
        Response response = httpClient.newCall(request).execute();
        return response.body().string();
    }

    /**
     * 提交json字符串到请求体
     *
     * @param url
     * @param json
     * @param header header内容
     * @return
     * @throws IOException
     */
    public String postJsonBody(String url, String json, Map<String, String> header) throws IOException {
        RequestBody body = RequestBody.create(JSON, json);
        Request.Builder builder = new Request.Builder().url(url).post(body);
        addHeader(builder, header);

        Request request = builder.build();
        Response response = httpClient.newCall(request).execute();
        return response.body().string();
    }

    private void addHeader(Request.Builder builder, Map<String, String> header) {
        if (header != null) {
            Set<Map.Entry<String, String>> entrySet = header.entrySet();
            for (Map.Entry<String, String> entry : entrySet) {
                builder.addHeader(entry.getKey(), String.valueOf(entry.getValue()));
            }
        }
    }

    public void setCookieStore(Map<String, List<Cookie>> cookieStore) {
        this.cookieStore = cookieStore;
    }

    public void setHttpClient(OkHttpClient httpClient) {
        this.httpClient = httpClient;
    }

    public void setOpenConfig(OpenConfig openConfig) {
        this.openConfig = openConfig;
    }
}
