package com.nan.gbd.library.http.client;

import com.nan.gbd.library.http.response.BaseResponse;
import com.nan.gbd.library.http.config.OpenConfig;
import com.nan.gbd.library.http.request.RequestForm;
import com.nan.gbd.library.http.request.RequestMethod;
import com.nan.gbd.library.utils.JsonUtils;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.Map;
import java.util.Set;

/**
 * @author NanGBD
 * @date 2020.3.8
 */
public class OpenRequest {
    private static final String AND = "&";
    private static final String EQ = "=";
    private static final String UTF8 = "UTF-8";

    private static final String HTTP_ERROR_CODE = "-400";

    private OpenConfig openConfig;
    private OpenHttp openHttp;

    public OpenRequest(OpenConfig openConfig) {
        this.openConfig = openConfig;
        this.openHttp = new OpenHttp(openConfig);
    }

    public String request(String url, RequestForm requestForm, Map<String, String> header) {
        RequestMethod requestMethod = requestForm.getRequestMethod();
        boolean isGet = requestMethod.name().equalsIgnoreCase(RequestMethod.GET.toString());
        if (isGet) {
            return this.doGet(url, requestForm, header);
        } else {
            return this.doPost(url, requestForm, header);
        }
    }

    protected String doGet(String url, RequestForm requestForm, Map<String, String> header) {
        StringBuilder queryString = new StringBuilder();
        Map<String, Object> form = requestForm.getForm();
        Set<String> keys = form.keySet();
        for (String keyName : keys) {
            try {
                queryString.append(AND).append(keyName).append(EQ)
                        .append(URLEncoder.encode(String.valueOf(form.get(keyName)), UTF8));
            } catch (UnsupportedEncodingException e) {
                throw new RuntimeException(e);
            }
        }

        try {
            String requestUrl = url + "?" + queryString.toString().substring(1);
            return openHttp.get(requestUrl, header);
        } catch (IOException e) {
            return this.causeException(e);
        }

    }

    protected String doPost(String url, RequestForm requestForm, Map<String, String> header) {
        try {
            Map<String, Object> form = requestForm.getForm();
            return openHttp.postJsonBody(url, JsonUtils.toJSONString(form), header);
        } catch (IOException e) {
            return this.causeException(e);
        }
    }

    public String postJsonBody(String url, String json) throws IOException {
        return this.openHttp.postJsonBody(url, json, null);
    }

    protected String causeException(Exception e) {
        ErrorResponse result = new ErrorResponse();
        result.setCode(HTTP_ERROR_CODE);
        result.setMsg(e.getMessage());
        return JsonUtils.toJSONString(result);
    }

    static class ErrorResponse extends BaseResponse<String> {
    }
}
