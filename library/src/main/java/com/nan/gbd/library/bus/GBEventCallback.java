package com.nan.gbd.library.bus;

/**
 * @author NanGBS
 * @date
 */
public class GBEventCallback {
    private int code;
    private String name;
    private byte[] param;

    public GBEventCallback(int code, String name, byte[] param) {
        this.code = code;
        this.name = name;
        this.param = param;
    }

    public int getCode() {
        return code;
    }

    public void setCode(int code) {
        this.code = code;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public byte[] getParam() {
        return param;
    }

    public void setParam(byte[] param) {
        this.param = param;
    }
}
