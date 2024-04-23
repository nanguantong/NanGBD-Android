package com.nan.gbd.library.http.request;

import com.nan.gbd.library.http.response.AuthResponse;

/**
 * 认证请求
 * @author NanGBD
 * @date 2018.3.8
 */
public class AuthRequest extends BaseRequest<AuthResponse> {
    public AuthRequest(String name) {
        this.setName(name);
    }

    public AuthRequest(String name, String version) {
        this.setName(name);
        this.setVersion(version);
    }

    @Override
    public String name() {
        return "";
    }
}
