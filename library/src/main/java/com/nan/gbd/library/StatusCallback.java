package com.nan.gbd.library;

/**
 * @author NanGBD
 * @date 2020.6.8
 */
public interface StatusCallback {
//    public static final int FLAG_AV = 0;
//    public static final int FLAG_A = 1;
//    public static final int FLAG_V = 2;
//    public static int avFlag = FLAG_AV;

    boolean onCallback(int code, String name, int key, byte[] param);
}
