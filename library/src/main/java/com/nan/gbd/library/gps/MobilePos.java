package com.nan.gbd.library.gps;

/**
 * 移动位置信息
 *
 * @author NanGBD
 * @date 2021.2.28
 */
public class MobilePos {
    private String time;        // 产生通知时间(必选), 格式: 2018-02-10T19:47:59
    private double longitude;   // 经度(必选)
    private double latitude;    // 纬度(必选)
    private double speed;       // 速度,单位:km/h(可选)
    private double direction;   // 方向,取值为当前摄像头方向与正北方的顺时针夹角,取值范围0°~360°,单位:(°)(可选)
    private double altitude;    // 海拔高度,单位:m(可选)

    public String getTime() {
        return time;
    }

    public void setTime(String time) {
        this.time = time;
    }

    public double getLongitude() {
        return longitude;
    }

    public void setLongitude(double longitude) {
        this.longitude = longitude;
    }

    public double getLatitude() {
        return latitude;
    }

    public void setLatitude(double latitude) {
        this.latitude = latitude;
    }

    public double getSpeed() {
        return speed;
    }

    public void setSpeed(double speed) {
        this.speed = speed;
    }

    public double getDirection() {
        return direction;
    }

    public void setDirection(double direction) {
        this.direction = direction;
    }

    public double getAltitude() {
        return altitude;
    }

    public void setAltitude(double altitude) {
        this.altitude = altitude;
    }
}
