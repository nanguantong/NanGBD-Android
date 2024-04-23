package com.nan.gbd.library.http.request;

import java.util.HashMap;
import java.util.Map;

/**
 * @author NanGBD
 * @date 2020.3.8
 */
public class RequestForm {
    private RequestMethod requestMethod = RequestMethod.POST;
    /** 请求表单内容 */
    private Map<String, Object> form;

    public RequestForm(Map<? extends String, ?> m) {
        this.form = new HashMap<>(m);
    }

    public Map<String, Object> getForm() {
        return form;
    }

    public void setForm(Map<String, Object> form) {
        this.form = form;
    }

    public RequestMethod getRequestMethod() {
        return requestMethod;
    }

    public void setRequestMethod(RequestMethod requestMethod) {
        this.requestMethod = requestMethod;
    }
}
