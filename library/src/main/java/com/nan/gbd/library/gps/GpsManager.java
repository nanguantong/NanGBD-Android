package com.nan.gbd.library.gps;

import android.annotation.SuppressLint;
import android.content.Context;
import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.location.OnNmeaMessageListener;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;

//import com.google.android.things.contrib.driver.gps.NmeaGpsModule;

import java.util.List;

/**
 * Android原生方式获取经纬度和城市信息
 *
 * Android中定位的方式 一般有这四种：GPS定位，WIFI定准，基站定位，AGPS定位(基站+GPS)
 *
 * @author NanGBD
 * @date 2021.2.28
 */
public class GpsManager {
    private static final String TAG = GpsManager.class.getSimpleName();

    private static LocationManager mLocationManager;
    private String mLocationProvider;
    private GpsCallback cbGps;

    public boolean init(Context context, GpsCallback cb) {
        if (null == mLocationManager) {
            mLocationManager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
        }
        if (null == mLocationManager) {
            Log.e(TAG, "no location manager");
            return false;
        }

        /**
         * passive：被动提供，由其他程序提供
         * gps：通过GPS获取定位信息
         * network：通过网络获取定位信息
         */
        List<String> providers;
        //providers = mLocationManager.getAllProviders();
        providers = mLocationManager.getProviders(true);

        if (null == mLocationProvider) {
            if (providers.contains(LocationManager.GPS_PROVIDER)) {
                mLocationProvider = LocationManager.GPS_PROVIDER;
            }
        }
        if (null == mLocationProvider) {
            if (providers.contains(LocationManager.NETWORK_PROVIDER)) {
                mLocationProvider = LocationManager.NETWORK_PROVIDER;
            }
        }

        if (null == mLocationProvider) {
            Criteria criteria = new Criteria();
            // 设置定位精确度 Criteria.ACCURACY_COARSE比较粗略，Criteria.ACCURACY_FINE则比较精细
            criteria.setAccuracy(Criteria.ACCURACY_FINE);
            // 设置是否要求速度
            criteria.setSpeedRequired(true);
            // 设置是否需要海拔信息
            criteria.setAltitudeRequired(true);
            // 设置是否需要方位信息
            criteria.setBearingRequired(true);
            // 设置是否允许运营商收费
            criteria.setCostAllowed(true);
            // 设置对电源的需求
            criteria.setPowerRequirement(Criteria.POWER_LOW);
            mLocationProvider = mLocationManager.getBestProvider(criteria, true);
        }

        if (null == mLocationProvider) {
            Log.e(TAG, "no location provider");
            return false;
        }

        this.cbGps = cb;
        return true;
    }

    /**
     * 开始定位
     * @param intervalMs 更新数据时间(毫秒)
     * @param distance 位置间隔(米)
     */
    @SuppressLint("MissingPermission")
    public boolean start(int intervalMs, float distance) {
        if (null == mLocationManager || null == mLocationProvider) {
            return false;
        }
        try {
            mLocationManager.requestLocationUpdates(
                    mLocationProvider,  //定位提供者
                    intervalMs,         //更新数据时间
                    distance,           //位置间隔
                    mLocationListener); //位置监听器 GPS定位信息发生改变时触发，用于更新位置信息

            //mLocationManager.addNmeaListener()

            // 获取最新的定位信息
            Location location = mLocationManager.getLastKnownLocation(mLocationProvider);
            if (null != location) {
                Log.i(TAG, "location: " + location);
                if (null != cbGps) {
                    cbGps.onLocationChanged(location.getTime(), location.getLongitude(), location.getLatitude(),
                          location.getSpeed(), location.getBearing(), location.getAltitude());
                }
            }
        } catch (Exception e) {
            Log.i(TAG, "request location failed: " + e.getMessage());
            return false;
        }
        return true;
    }

    private boolean isGpsAvailable() {
        return null != mLocationManager && null != mLocationProvider;
    }

    @SuppressLint("MissingPermission")
    private Location getLastKnownLocation() {
        //获取地理位置管理器
        List<String> providers = mLocationManager.getProviders(true);
        Location bestLocation = null;
        for (String provider : providers) {
            Location l = mLocationManager.getLastKnownLocation(provider);
            if (l == null) {
                continue;
            }
            if (bestLocation == null || l.getAccuracy() < bestLocation.getAccuracy()) {
                // Found best last known location
                bestLocation = l;
            }
        }
        return bestLocation;
    }

    public LocationListener mLocationListener = new LocationListener() {
        //当坐标改变时触发，如果Provider传进相同的坐标，它就不会被触发
        @Override
        public void onLocationChanged(Location location) {
            if (location != null) {
                Log.i(TAG, "location changed: " + location.toString());
                if (null != cbGps) {
                    cbGps.onLocationChanged(location.getTime(), location.getLongitude(), location.getLatitude(),
                            location.getSpeed(), location.getBearing(), location.getAltitude());
                }
            }
        }

        // Provider的状态在可用、暂时不可用和无服务三个状态直接切换时触发
        @Override
        public void onStatusChanged(String provider, int status, Bundle extras) {
            Log.d("GPS status changed", provider);
        }

        // Provider被enable时触发，比如GPS被打开
        @Override
        public void onProviderEnabled(String provider) {
            Log.d("GPS enabled", provider);
        }

        // Provider被disable时触发，比如GPS被关闭
        @Override
        public void onProviderDisabled(String provider) {
            Log.d("GPS disabled", provider);
        }
    };

//    private OnNmeaMessageListener mMessageListener = null;
//    private boolean initNmea() {
//        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
//            mMessageListener = new OnNmeaMessageListener() {
//                @Override
//                public void onNmeaMessage(String message, long timestamp) {
//                    Log.d(TAG, "NMEA: " + message);
//                }
//            };
//        }
//        return true;
//    }
//
//    private static final String UART_DEVICE_NAME = "UART0";
//    public boolean init() {
//        String uartPortName = UART_DEVICE_NAME;
//        try {
//            NmeaGpsModule mGpsModule;
//            mGpsModule = new NmeaGpsModule(
//                    uartPortName,
//                    baudRate        // specified baud rate for your GPS peripheral
//            );
//            mGpsModule.setGpsAccuracy(accuracy); // specified accuracy for your GPS peripheral
//            mGpsModule.setGpsModuleCallback(new GpsModuleCallback() {
//                @Override
//                public void onGpsSatelliteStatus(GnssStatus status) {
//                }
//
//                @Override
//                public void onGpsTimeUpdate(long timestamp) {
//                }
//
//                @Override
//                public void onGpsLocationUpdate(Location location) {
//                }
//
//                @Override
//                public void onNmeaMessage(String nmeaMessage) {
//                }
//            });
//            mGpsModule.close();
//        } catch (IOException e) {
//            // couldn't configure the gps module...
//        }
//        return true;
//    }

}
