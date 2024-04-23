package com.example.nan.gb28181_android;

import android.app.Application;

/**
 * @author NanGBD
 * @date 2020.6.8
 */
public class NanGBDApplication extends Application {

    private static NanGBDApplication mApplication;

    public static NanGBDApplication getNanApplication() {
        return mApplication;
    }

    @Override
    public void onCreate() {
        super.onCreate();

        mApplication = this;
    }

}
