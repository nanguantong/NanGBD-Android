package com.nan.gbd.library.alarm;

/**
 * 报警信息
 *
 * @author NanGBD
 * @date 2022.2.7
 */
public class AlarmInfo {
    private String priority;  // 报警级别(必选), 0为全部,1为一级警情,2为二级警情,3为三级警情,4为四级警情
    private String method;    // 报警方式(必选), 1为电话报警,2为设备报警,3为短信报警,4为 GPS报警,5为视频报警,6为设备故障报警,7其他报警
    private String time;      // 报警时间(必选) 如: 2009-12-04T16:23:32
    private String desc;      // 报警内容描述(可选)
    private double longitude; // 经度信息(可选)
    private double latitude;  // 纬度信息(可选)
    /**
     * 报警类型(可选)
     * 报警方式为2时,取值如下:
     *     1-视频丢失报警;2-设备防拆报警;3-存储设备磁盘满报警;4-设备高温报警;5-设备低温报警。
     * 报警方式为5时,取值如下:
     *     1-人工视频报警;2-运动目标检测报警;3-遗留物检测报警;4-物体移除检测报警;5-绊线检测报警;
     *     6-入侵检测报警;7-逆行检测报警;8-徘徊检测报警;9-流量统计报警;10-密度检测报警;
     *     11-视频异常检测报警;12-快速移动报警。
     * 报警方式为6时,取值如下:
     *     1-存储设备磁盘故障报警;2-存储设备风扇故障报警。
     */
    private int alarmType;
    private int eventType;    // 报警类型扩展参数(可选), 在入侵检测报警时可携带此参数,取值:1-进入区域;2-离开区域。

    public String getPriority() {
        return priority;
    }

    public void setPriority(String priority) {
        this.priority = priority;
    }

    public String getMethod() {
        return method;
    }

    public void setMethod(String method) {
        this.method = method;
    }

    public String getTime() {
        return time;
    }

    public void setTime(String time) {
        this.time = time;
    }

    public String getDesc() {
        return desc;
    }

    public void setDesc(String desc) {
        this.desc = desc;
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

    public int getAlarmType() {
        return alarmType;
    }

    public void setAlarmType(int alarmType) {
        this.alarmType = alarmType;
    }

    public int getEventType() {
        return eventType;
    }

    public void setEventType(int eventType) {
        this.eventType = eventType;
    }
}
