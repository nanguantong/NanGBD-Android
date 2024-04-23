package com.nan.gbd.library.gps;

/**
 * @author NanGBD
 * @date 2021.2.28
 */
public interface GpsCallback {
    void onLocationChanged(long time, double longitude, double latitude, float speed, float direction, double altitude);
}
