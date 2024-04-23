package com.nan.gbd.library.sip;

import android.util.Log;

import com.nan.gbd.library.JNIBridge;
import com.nan.gbd.library.alarm.AlarmInfo;
import com.nan.gbd.library.auth.AuthConfig;
import com.nan.gbd.library.gps.MobilePos;
import com.nan.gbd.library.http.client.OpenClient;
import com.nan.gbd.library.http.config.OpenConfig;
import com.nan.gbd.library.http.request.AuthParam;
import com.nan.gbd.library.http.request.AuthRequest;
import com.nan.gbd.library.http.response.AuthResponse;
import com.nan.gbd.library.osd.OsdConfig;
import com.nan.gbd.library.utils.StringUtils;

import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

/**
 * @author NanGBD
 * @date 2020.6.8
 */
public class SipUaNative {
    private static final String TAG = SipUaNative.class.getSimpleName();
    private AuthConfig mAuthConfig;

    /**
     * 初始化国标设备
     * @param authConfig 设备认证配置项(必须)
     * @param sipConfig 国标sip配置项(必须)
     * @param osdConfig osd水印配置项, 为空则不设置水印
     * @return 0:成功，否则失败
     */
    public synchronized int init(AuthConfig authConfig, SipConfig sipConfig, OsdConfig osdConfig) {
        if (null == authConfig || StringUtils.isEmpty(authConfig.getAppKey()) ||
                StringUtils.isEmpty(authConfig.getAppSecret()) ||
                StringUtils.isEmpty(authConfig.getImei())) {
            Log.e(TAG, "auth failed");
            return -1;
        }
        GBDevice gbDevice;
        if (null == sipConfig || null == (gbDevice = sipConfig.getGbDevice())) {
            Log.e(TAG, "sip config and device config cannot null");
            return -2;
        }
        mAuthConfig = authConfig;
        // 可以认证加密
//        if (!a()) {
//            Log.e(TAG, "auth config failed");
//            return -3;
//        }
        if (null != osdConfig) {
            JNIBridge.nanOsdInit(osdConfig.isEnable(), osdConfig.getCharWidth(),
                    osdConfig.getCharHeight(), osdConfig.getOsdsArray(), osdConfig.getCharsArray());
        }
        return JNIBridge.nanUaInit(
                sipConfig.getServerIp(),
                sipConfig.getServerPort(),
                sipConfig.getServerId(),
                sipConfig.getServerDomain(),
                gbDevice.getGmLevel().ordinal(),
                gbDevice.getName(),
                gbDevice.getManufacturer(),
                gbDevice.getModel(),
                gbDevice.getFirmware(),
                gbDevice.getSerialNumber(),
                gbDevice.getChannel(),
                gbDevice.getUsername(),
                gbDevice.getUserid(),
                gbDevice.getPassword(),
                gbDevice.getPort(),
                GBDevice.Protocol.UDP == gbDevice.getProtocol() ? "UDP" : "TCP",
                gbDevice.getRegExpiry(),
                gbDevice.getHeartbeatInterval(),
                gbDevice.getHeartbeatCount(),
                gbDevice.getLongitude(),
                gbDevice.getLatitude(),
                gbDevice.getVideoId(),
                gbDevice.getAudioId(),
                gbDevice.getAlarmId(),
                gbDevice.getCharset().getValue(),
                gbDevice.getInviteCount(),
                gbDevice.getVoiceCount(),
                gbDevice.getPosCapability(),
                gbDevice.getGlobalAuth() ? 1 : 0);
    }

    /**
     * 初始化国标设备
     * @param authConfig 设备认证配置项
     * @param sipConfig 国标sip配置项
     * @return 0:成功，否则失败
     */
    public synchronized int init(AuthConfig authConfig, SipConfig sipConfig) {
        return init(authConfig, sipConfig, null);
    }

    /**
     * 异步注册设备到国标服务平台, 是否注册成功通过事件回调获取
     * @return 0:成功, 否则失败
     */
    public synchronized int register() {
        return JNIBridge.nanUaRegister();
    }

    /**
     * 异步注销从国标国标服务平台, 是否注销成功通过事件回调获取
     * @return 0:成功, 否则失败
     */
    public synchronized int unregister() {
        return JNIBridge.nanUaUnregister();
    }

    /**
     * 设备是否注册成功到国标服务平台
     * @return true:已注册, 否则未注册
     */
    public synchronized boolean isRegistered() {
        return JNIBridge.nanUaIsRegistered();
    }

    /**
     * 通知设备配置信息是否发生变化
     * @param changed
     */
    public synchronized void onConfigChanged(boolean changed) {
        JNIBridge.nanUaOnConfigChanged(changed);
    }

    /**
     * 获取设备证书
     * @return 设备证书
     */
    public String getDeviceCert() {
        return JNIBridge.nanUaGetDeviceCert();
    }

    /**
     * 设置服务证书
     * @param serverId  服务国标id
     * @param cert      服务证书
     * @param isFile    证书是文件还是内存
     * @param passin    证书密码
     * @return 0:成功, 否则失败
     */
    public synchronized int setServerCert(String serverId, String cert, boolean isFile, String passin) {
        return JNIBridge.nanUaSetServerCert(serverId, cert, isFile, passin);
    }

    /**
     * 设置或更新设备移动位置信息
     * @param mobilePos
     * @return 0:成功, 否则失败
     */
    public synchronized int setMobilePos(MobilePos mobilePos) {
        return JNIBridge.nanUaSetMobilePos(mobilePos.getTime(), mobilePos.getLongitude(), mobilePos.getLatitude(),
                mobilePos.getSpeed(), mobilePos.getDirection(), mobilePos.getAltitude());
    }

    /**
     * 设置设备报警信息
     * @param alarm
     * @return 0:成功, 否则失败
     */
    public synchronized int setAlarm(AlarmInfo alarm) {
        return JNIBridge.nanUaSetAlarm(alarm.getPriority(), alarm.getMethod(), alarm.getTime(),
                alarm.getDesc(), alarm.getLongitude(), alarm.getLatitude(),
                alarm.getAlarmType(), alarm.getEventType());
    }

    /**
     * 反初始化国标设备
     * @return 0:成功, 否则失败
     */
    public synchronized int uninit() {
        return JNIBridge.nanUaUninit();
    }

    private boolean a() {
        ExecutorService executorService = Executors.newFixedThreadPool(1);
        Future<Boolean> f = executorService.submit(new Callable<Boolean>() {
            @Override
            public Boolean call() {
                OpenClient client = new OpenClient(new String(OpenConfig.URL_API),
                        mAuthConfig.getAppKey(), mAuthConfig.getAppSecret());
                AuthRequest request = new AuthRequest("devices.auth");
                AuthParam param = new AuthParam();
                param.setImei(mAuthConfig.getImei());
                request.setParam(param);
                request.getName();request.getVersion();request.getData();
                request.getTimestamp();request.getAccess_token();request.getFormat();
                AuthResponse response = client.execute(request);
                return response.isSuccess() && response.getData();
            }
        });
        boolean ret = false;
        try {
            ret = f.get();
        } catch (Exception e) { }
        return ret;
    }
}
