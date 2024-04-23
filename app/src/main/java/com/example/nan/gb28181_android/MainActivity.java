package com.example.nan.gb28181_android;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.graphics.drawable.Drawable;
import android.net.ConnectivityManager;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.alibaba.fastjson.JSON;
import com.alibaba.fastjson.JSONObject;
import com.example.nan.gb28181_android.utils.DrawingUtil;
import com.example.nan.gb28181_android.utils.SPUtil;
import com.example.nan.gb28181_android.utils.WindowUtil;
import com.nan.gbd.library.auth.AuthConfig;
import com.nan.gbd.library.bus.CameraFramerate;
import com.nan.gbd.library.bus.CameraResolution;
import com.nan.gbd.library.gps.GpsCallback;
import com.nan.gbd.library.gps.GpsManager;
import com.nan.gbd.library.gps.MobilePos;
import com.nan.gbd.library.media.GBMediaRecorder;
import com.nan.gbd.library.media.MediaRecorderNative;
import com.nan.gbd.library.osd.OsdConfig;
import com.nan.gbd.library.osd.OsdGenerator;
import com.nan.gbd.library.sip.SipConfig;
import com.nan.gbd.library.sip.SipUaNative;
import com.nan.gbd.library.bus.GBEventCallback;
import com.nan.gbd.library.source.MediaSourceType;
import com.nan.gbd.library.utils.BusUtils;
import com.nan.gbd.library.utils.DeviceUtils;
import com.nan.gbd.library.utils.StringUtils;
import com.squareup.otto.Subscribe;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

import static com.nan.gbd.library.sip.GBEvent.*;

/**
 * @author NanGBD
 * @date 2019.6.8
 */
public class MainActivity extends AppCompatActivity implements // View.OnClickListener,
        VideoInputDialog.OnSelectVideoInputListener, GpsCallback,
        GBMediaRecorder.OnErrorListener, GBMediaRecorder.OnPreparedListener, GBMediaRecorder.OnPreviewListener {

    private static final String TAG = MainActivity.class.getSimpleName();

    private static final int REQUEST_PERMISSION_ID = 20;
    public static final int REQUEST_SETTINGS = 100;

    private static final String GB_CODE = "GBCode";
    private static final String GB_STATE = "GBState";
    private static final String GB_PARAM = "GBParam";
    private static final int MSG_STATE = 100;

    private boolean isPermissionGranted = false;
    // https://blog.csdn.net/u013233097/article/details/78595048
    private static final String[] REQUESTED_PERMISSIONS = {
            Manifest.permission.CAMERA,
            Manifest.permission.RECORD_AUDIO,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.ACCESS_COARSE_LOCATION,
            Manifest.permission.ACCESS_FINE_LOCATION,
            Manifest.permission.READ_PHONE_STATE,
    };

    BroadcastReceiver mConnectionReceiver;
    boolean mNetConnnected;

    private Button mBtnRegister;
    private Button mBtnSettings;
    private Button mBtnSwitchVideoInput;
    private VideoInputDialog mVideoInputDialog;
    private SurfaceView mSurfaceView;
    private RelativeLayout mBottomLayout;
    private TextView mTxtStatus, mStreamStat;
    private Spinner mSpinnerResolution;

    private int mWidth = 640, mHeight = 480;
    private int mPreviewFps;
    private List<String> mListResolution = new ArrayList<>();

    private long mExitTime; // 上一次点击'返回键'的时间

    static public SipUaNative mSipUa = new SipUaNative();
    private GBMediaRecorder mMediaRecorder;
    private MediaSourceType mSourceType = MediaSourceType.MS_TYPE_CAMERA;

    /**
     * 设备授权配置
     */
    private AuthConfig mAuthConfig;

    /**
     * osd水印配置
     */
    private OsdConfig mOsdConfig;
    private OsdGenerator mOsdGen;

    /**
     * gps管理
     */
    private GpsManager gpsManager;

    Handler mHandlerAsync;


    //////////////////////// TEST ////////////////////////

    private DrawingUtil mDrawingUtil;

    //////////////////////// TEST ////////////////////////


    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case MSG_STATE: {
                    mTxtStatus.setText(msg.getData().getString(GB_STATE));

                    int code = msg.getData().getInt(GB_CODE);
                    switch (code) {
                        case GB_DEVICE_EVENT_CONNECTING:
                            break;
                        case GB_DEVICE_EVENT_REGISTERING:
                            break;
                        case GB_DEVICE_EVENT_REGISTER_OK:
                            mBtnRegister.setText("注销");
                            break;
                        case GB_DEVICE_EVENT_REGISTER_AUTH_FAIL:
                            mHandlerAsync.postDelayed(() -> {
                                while (true) {
//                                    if (!mNetConnnected) {
//                                        try {
//                                            Thread.sleep(3000);
//                                        } catch (InterruptedException ignored) { }
//                                        continue;
//                                    }
                                    mSipUa.register();
                                    break;
                                }
                            }, 15000);
                        case GB_DEVICE_EVENT_UNREGISTER_OK:
                            mBtnRegister.setText("注册");
                            break;
                        case GB_DEVICE_EVENT_UNREGISTER_FAILED:
                            Log.e(TAG, "注销失败");
                            break;
                        case GB_DEVICE_EVENT_START_VIDEO:
                            Log.d(TAG, "开始视频");
                            break;
                        case GB_DEVICE_EVENT_STOP_VIDEO:
                            Log.d(TAG, "停止视频");
                            break;
                        case GB_DEVICE_EVENT_START_VOICE:
                            Log.i(TAG, "开始语音");
                            break;
                        case GB_DEVICE_EVENT_STOP_VOICE:
                            Log.i(TAG, "停止语音");
                            break;
                        case GB_DEVICE_EVENT_TALK_AUDIO_DATA:
                            Log.i(TAG, "语音数据");
                            break;
                        case GB_DEVICE_EVENT_START_RECORD:
                            Log.i(TAG, "开始录制");
                            break;
                        case GB_DEVICE_EVENT_STOP_RECORD:
                            Log.i(TAG, "停止录制");
                            break;
                        case GB_DEVICE_EVENT_STATISTICS:
                            byte[] param = msg.getData().getByteArray(GB_PARAM);
                            if (null != param) {
                                String strParam = new String(param, StandardCharsets.UTF_8);
                                JSONObject jParam = JSON.parseObject(strParam);
                                if (jParam.containsKey("upFlow")) {
                                    int upFlow = jParam.getIntValue("upFlow");
                                    mTxtStatus.append(":" + upFlow / 1024 + " KB/s");
                                }
                            }
                            break;
                        case GB_DEVICE_EVENT_DISCONNECT:
                            Log.w(TAG, "连接断开");
                            mBtnRegister.setText("注册");

                            // 此处判断网络连通性，如果网络断了之后等网络再恢复时可以做自动重新注册处理
                            mHandlerAsync.postDelayed(() -> {
                                while (true) {
                                    if (!mNetConnnected) {
                                        try {
                                            Thread.sleep(3000);
                                        } catch (InterruptedException ignored) { }
                                        continue;
                                    }
                                    mSipUa.register();
                                    break;
                                }
                            }, 500);

                            break;
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON); // 防止锁屏
        setContentView(R.layout.activity_main);
        WindowUtil.closeAndroidPDialog();

        if (!initAuth()) {
            return;
        }

        for (String p : REQUESTED_PERMISSIONS) {
            if (!checkSelfPermission(p, REQUEST_PERMISSION_ID)) {
                Log.e(TAG, "please grant " + p);
                return;
            }
        }
        // 权限已经开启
        isPermissionGranted = true;

        // 网络检查器
        mConnectionReceiver = registerConnectionReceiver();

        // 异步处理器
        HandlerThread ht = new HandlerThread("asyncThread");
        ht.start();
        //创建handler, 通过绑定一个looper消息管理, 即这个Handler是运行在ht这个线程中(防止阻塞UI线程)
        mHandlerAsync = new Handler(ht.getLooper());

        // 事件订阅接收器, sdk事件通知订阅回调必须加此句才能生效
        BusUtils.BUS.register(this);

        // 输出log，此处发布正式版的时候可以不写
        try {
            Log.i("log", "path: " + GBMediaRecorder.getLogOutPutPath());
            Runtime.getRuntime().exec("logcat -f " + GBMediaRecorder.getLogOutPutPath());
        } catch (IOException e) {
            e.printStackTrace();
        }

        initView();

        mBtnRegister.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mHandlerAsync.post(() -> {
                    if (mSipUa == null) {
                        return;
                    }
                    int ret;
                    if (!mSipUa.isRegistered()) {
                        ret = mSipUa.register();
                        if (ret != 0) {
                            sendMessage(-1, "注册失败");
                        }
                    } else {
                        if (mMediaRecorder != null && mMediaRecorder.isRecording()) {
                            mMediaRecorder.setRecording(false);
                            sendMessage(-1, "推流结束");
                        }
                        ret = mSipUa.unregister();
                        if (ret != 0) {
                            sendMessage(-1, "注销失败");
                        }
                    }
                });
            }
        });

        mBtnSwitchVideoInput.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if ((mVideoInputDialog != null && mVideoInputDialog.isShowing())) {
                    return;
                }
                mVideoInputDialog = new VideoInputDialog(MainActivity.this, view, MainActivity.this);
                mVideoInputDialog.show();
            }
        });
    }

    @Override
    public void onResume() {
        Log.i(TAG, "onResume");
        super.onResume();

        initMediaRecorder();
        initOsd();
        initGps();

        mHandlerAsync.post(() -> {
            initSipUa();
        });
    }

    @Override
    protected void onDestroy() {
        Log.i(TAG, "onDestroy");
        super.onDestroy();

        unregisterConnectionReceiver();
        BusUtils.BUS.unregister(this);

        mHandlerAsync.post(() -> {
            if (mMediaRecorder != null) {
                mMediaRecorder.release();
            }
            if (mSipUa != null) {
                mSipUa.uninit();
            }
        });
    }

    @Override
    public void finish() {
        Log.d(TAG, "finish");

        if (mVideoInputDialog != null && mVideoInputDialog.isShowing()) {
            mVideoInputDialog.dismiss();
        }
        super.finish();
    }

    /**
     * Take care of popping the fragment back stack or finishing
     * the activity as appropriate.
     */
    @Override
    public void onBackPressed() {
        if (mMediaRecorder != null && mMediaRecorder.isRecording() && SPUtil.getEnableBackgroundCamera(this)) {
            new AlertDialog.Builder(this).setTitle("是否允许后台上传？")
                    .setMessage("您设置了使能摄像头后台采集,是否继续在后台采集并上传视频？如果是，记得直播结束后,再回来关闭直播")
                    .setNeutralButton("后台采集", (dialogInterface, i) -> {
                        MainActivity.super.onBackPressed();
                    })
                    .setPositiveButton("退出程序", (dialogInterface, i) -> {
                        mMediaRecorder.setRecording(false);
                        MainActivity.super.onBackPressed();
                        Toast.makeText(MainActivity.this, "程序已退出", Toast.LENGTH_SHORT).show();
                    })
                    .setNegativeButton(android.R.string.cancel, null)
                    .show();
            return;
        }

        //与上次点击返回键时刻作差
        if ((System.currentTimeMillis() - mExitTime) > 2000) {
            //大于2000ms则认为是误操作，使用Toast进行提示
            Toast.makeText(this, "再按一次退出程序", Toast.LENGTH_SHORT).show();
            //并记录下本次点击“返回键”的时刻，以便下次进行判断
            mExitTime = System.currentTimeMillis();
        } else {
//            //彻底关闭整个APP
//            if (Build.VERSION.SDK_INT > Build.VERSION_CODES.ECLAIR_MR1) {
//                Intent startMain = new Intent(Intent.ACTION_MAIN);
//                startMain.addCategory(Intent.CATEGORY_HOME);
//                startMain.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
//                startActivity(startMain);
//                System.exit(0);
//            } else { // android2.1
//                ActivityManager am = (ActivityManager) getSystemService(ACTIVITY_SERVICE);
//                am.restartPackage(getPackageName());
//            }
            super.onBackPressed();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        Log.i(TAG, "onRequestPermissionsResult " + grantResults[0] + " " + requestCode);

        switch (requestCode) {
            case REQUEST_PERMISSION_ID: {
                for (int i = 0; i < grantResults.length; i++) {
                    if (grantResults[i] != PackageManager.PERMISSION_GRANTED) {
                        showToast("权限申请是失败", Toast.LENGTH_LONG);
                        finish();
                        return;
                    }
                }
                initView();
                break;
            }
        }
    }

//    @Override
//    public void onClick(View view) {
//        int vId = view.getId();
//        switch (vId) {
//            case R.id.btn_start_register:
//                break;
//            case R.id.btn_settings:
//                break;
//            default:break;
//        }
//    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_SETTINGS && resultCode == RESULT_OK) {
            Bundle bundle;
            if (data != null && (bundle = data.getExtras()) != null) {
                // 配置变化
                boolean configChanged = bundle.getBoolean(SPUtil.KEY_CONFIG_CHANGED);
                if (configChanged) {
                    mSipUa.onConfigChanged(true);
                }
            }
        }
    }

    private boolean checkSelfPermission(String permission, int requestCode) {
        Log.i(TAG, "checkSelfPermission " + permission + " " + requestCode);
        if (Build.VERSION.SDK_INT >= 23 && ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, REQUESTED_PERMISSIONS, requestCode);
            return false;
        }
        return true;
    }

    /**
     * 注册网络连接监听器
     * @return
     */
    private BroadcastReceiver registerConnectionReceiver() {
        BroadcastReceiver connectionReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (!DeviceUtils.isMobileConnected(context) && !DeviceUtils.isWifiConnected(context)) {
                    mNetConnnected = false;
                    // TODO: 如果网络断了
                    //  (1) 可以调用 mSipUa.unregister() 立即注销掉连接，
                    //  (2) 或者等待 sdk 自动回调 EVENT_DISCONNECT 事件时等待网络恢复后主动再次注册
                    Log.w(TAG, "network disconnect ...");
                    showToast("网络连接断了", Toast.LENGTH_LONG);
                } else {
                    mNetConnnected = true;
                    Log.i(TAG, "network connected");
                    showToast("网络连接上了", Toast.LENGTH_LONG);
                }
            }
        };
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
        registerReceiver(connectionReceiver, intentFilter);
        return connectionReceiver;
    }

    /**
     * 注销网络连接监听器
     */
    private void unregisterConnectionReceiver() {
        if (mConnectionReceiver != null) {
            unregisterReceiver(mConnectionReceiver);
        }
    }

    private void initView() {
        mBtnRegister = findViewById(R.id.btn_start_register);
        mBtnSettings = findViewById(R.id.btn_settings);
        mBtnSwitchVideoInput = findViewById(R.id.btn_switch_video_input);
        mSurfaceView = findViewById(R.id.record_preview);
        mBottomLayout = findViewById(R.id.bottom_lt);
        mTxtStatus = findViewById(R.id.txt_status);
        mSpinnerResolution = findViewById(R.id.spn_resolution);

        Drawable drawable = getResources().getDrawable(R.drawable.register);
        drawable.setBounds(0,0,30,30);
        mBtnRegister.setCompoundDrawables(drawable,null,null,null);

        drawable = getResources().getDrawable(R.drawable.settings);
        drawable.setBounds(0,0,30,30);
        mBtnSettings.setCompoundDrawables(drawable,null,null,null);

        drawable = getResources().getDrawable(R.drawable.camera);
        drawable.setBounds(0,0,30,30);
        mBtnSwitchVideoInput.setCompoundDrawables(drawable,null,null,null);

        //mSurfaceView.setZOrderOnTop(true);
        //mSurfaceView.getHolder().setFormat(SurfaceView.TRANSPARENT);
    }

    /**
     * 初始化设备授权，通过appKey和appSecret（厂家授权码）进行配置，每一台设备唯一
     */
    private boolean initAuth() {
        String imei = DeviceUtils.getIMEI(this);
        // TODO: 此处从南宙科技咨询授权码，输入对应的设备认证
        String appKey = "863258031729916";
        String appSecret = "c335c342ee304e2abc0d3d9b8e022d3a";

        if (StringUtils.isEmpty(appKey)) {
            return false;
        }
        mAuthConfig = new AuthConfig(imei, appKey, appSecret);
        return true;
    }

    /**
     * 初始化水印
     */
    private void initOsd() {
        mOsdConfig = SPUtil.getOsdConfig();
        if (null == mOsdGen) {
            mOsdGen = new OsdGenerator();
        }
        mOsdGen.init(mOsdConfig, this, "fonts/OpenSansCondensed-Light.ttf");
    }

    /**
     * 初始化gps地图
     */
    private boolean initGps() {
        boolean ret;
        if (null == gpsManager) {
            gpsManager = new GpsManager();
        }
        ret = gpsManager.init(this, this);
        if (!ret) {
            return false;
        }
        return gpsManager.start(500, 0.1f);
    }

    /**
     * 初始化国标设备sip
     */
    private void initSipUa() {
        int ret;
        if (null == mSipUa) {
            mSipUa = new SipUaNative();
        }
        SipConfig sipConfig = SPUtil.getSipConfig();
        ret = mSipUa.init(mAuthConfig, sipConfig, mOsdConfig);
        if (ret != 0) {
            sendMessage(-1, "初始化信令失败");
            return;
        }

        // 初始化(也可通过界面导入)添加国密平台证书
        if (null != sipConfig.getServerCert()) {
            mSipUa.setServerCert(sipConfig.getServerId(), sipConfig.getServerCert(), false, null);
        }

        // TODO: 此处可以初始化设备移动位置信息
//        sipConfig.getGbDevice().setPosCapability(1);
//        MobilePos mobilePos = new MobilePos();
//        mobilePos.setTime("2018-02-10T19:47:59");
//        mobilePos.setLongitude(17.272);
//        mobilePos.setLatitude(11.7594);
//        mSipUa.setMobilePos(mobilePos);

        // TODO: 根据具体需求动态设置报警信息
//        AlarmInfo alarm = new AlarmInfo();
//        alarm.setPriority("0");
//        alarm.setMethod("5");
//        alarm.setTime("2018-02-10T19:47:59");
//        //alarm.setDesc("test alarm");
//        alarm.setLongitude(17.272);
//        alarm.setLatitude(11.7594);
//        mSipUa.setAlarm(alarm);
    }

    /**
     * 初始化采集录制
     */
    private void initMediaRecorder() {
        if (!isPermissionGranted && mSourceType == MediaSourceType.MS_TYPE_CAMERA) {
            sendMessage(-1, "初始化采集失败");
            return;
        }
        if (mMediaRecorder == null) {
            mMediaRecorder = new MediaRecorderNative();
            mMediaRecorder.setOnPreviewListener(this);
            mMediaRecorder.setOnErrorListener(this);
            mMediaRecorder.setOnPreparedListener(this);
        }
        mMediaRecorder.setMediaSource(this, mSourceType);
        mMediaRecorder.updateResolution(mWidth, mHeight);
        mMediaRecorder.setPreviewFramerate(SPUtil.getFramerate(this));
        mMediaRecorder.setVideoEncodec(SPUtil.getVideoEncodec(this));
        mMediaRecorder.setVideoBitrate(SPUtil.getBitrateKbps(this));
        mMediaRecorder.setVideoBitrateMode(SPUtil.getBitrateMode(this));
        mMediaRecorder.setFramerate(SPUtil.getFramerate(this));
        mMediaRecorder.setGop(5);
        mMediaRecorder.setVideoProfile(SPUtil.getVideoProfile(this));
        mMediaRecorder.setThreadNum(SPUtil.getThreadNum(this));
        mMediaRecorder.setAvQueueSize(SPUtil.getFramerate(this) * 5);
        mMediaRecorder.setVideoBitrateFactor(0.3);
        mMediaRecorder.setVideoBitrateAdjustPeriod(10);
        mMediaRecorder.setStreamStatPeriod(3);
        mMediaRecorder.setEnableAV(SPUtil.getEnableVideo(this), SPUtil.getEnableAudio(this));
        mMediaRecorder.setAudioStartup(SPUtil.getAudioStartup(this));
        mMediaRecorder.setFileOutput("", 60);
        mMediaRecorder.setSurfaceHolder(mSurfaceView.getHolder());
        mMediaRecorder.prepare();
    }

    /**
     * 初始化画布
     */
    private void initSurfaceView() {
        final int screenWidth = DeviceUtils.getScreenWidth(this);
        final int screenHeight = DeviceUtils.getScreenHeight(this);
        // 避免摄像头的转换，只取上面h部分
//        ((RelativeLayout.LayoutParams) mBottomLayout.getLayoutParams()).topMargin =
//                (int) (screenWidth / ((GBMediaRecorder.getPreviewHeight() * 1.0f) / GBMediaRecorder.getPreviewWidth()));
        int width = screenWidth;
        int height = screenHeight;
        height = (int) (screenWidth * (mWidth * 1.0f)) / mHeight;
        Log.d(TAG, "initSurfaceView: w=" + width + ",h=" + height);
        FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams) mSurfaceView.getLayoutParams();
        lp.width = width;
        lp.height = height;
        mSurfaceView.setLayoutParams(lp);

//        mSurfaceView.setDrawingCacheEnabled(true);
//        if (true) {
//            Bitmap surfaceViewDrawingCache = mSurfaceView.getDrawingCache();
//            FileOutputStream fos = null;
//            try {
//                fos = new FileOutputStream("/sdcard/ps/test.jpeg");
//            } catch (FileNotFoundException e) {
//                e.printStackTrace();
//            }
//            surfaceViewDrawingCache.compress(Bitmap.CompressFormat.JPEG, 100, fos);
//        }
    }

    /**
     * 初始化摄像头支持的分辨率下拉控件的列表
     */
    private void initResolutionSpinner() {
        int position = mListResolution.indexOf(String.format("%dx%d", mWidth, mHeight));
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, R.layout.spinner_reslution_item, mListResolution);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mSpinnerResolution.setAdapter(adapter);
        mSpinnerResolution.setSelection(position, false);
        mSpinnerResolution.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (mMediaRecorder != null && mMediaRecorder.isRecording()) {
                    int curPos = mListResolution.indexOf(String.format("%dx%d", mWidth, mHeight));
                    if (curPos == position)
                        return;
                    mSpinnerResolution.setSelection(curPos, false);
                    showToast("正在推流中,无法切换摄像头分辨率", Toast.LENGTH_SHORT);
                    return;
                }

                String r = mListResolution.get(position);
                String[] wh = r.split("x");
                int w = Integer.parseInt(wh[0]);
                int h = Integer.parseInt(wh[1]);

                if (mWidth != w || mHeight != h) {
                    mWidth = w;
                    mHeight = h;
                    Log.d(TAG, "switch preview w: " + mWidth + ", h:" + mHeight);
                    if (mMediaRecorder != null) {
                        mMediaRecorder.updateResolution(mWidth, mHeight);
                    }
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    /**
     * 切换视频媒体输入源
     */
    private void switchMediaSource() {
        if (mSourceType == MediaSourceType.MS_TYPE_SCREEN) {
            mDrawingUtil = new DrawingUtil(mSurfaceView.getHolder(),
                    getResources().getDisplayMetrics());
            mDrawingUtil.start();
        } else {
            if (mDrawingUtil != null) {
                mDrawingUtil.quit();
                try {
                    mDrawingUtil.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                mDrawingUtil = null;
            }
        }
    }

    @Override
    public void onSurfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "onSurfaceCreated");
        switchMediaSource();
    }

    @Override
    public void onSurfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d(TAG, "onSurfaceChanged, " + String.format("format: %d, %dx%d", format, width, height));
    }

    @Override
    public void onSurfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "onSurfaceDestroyed");
        if (mDrawingUtil != null) {
            mDrawingUtil.quit();
            try {
                mDrawingUtil.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            mDrawingUtil = null;
        }
    }

    /**
     * 摄像头初始化完毕，初始化显示
     * @param previewWidth  预览宽度
     * @param previewHeight 预览高度
     * @param previewFps    预览帧率
     */
    @Override
    public void onPrepared(int previewWidth, int previewHeight, int previewFps) {
        Log.d(TAG, "onPrepared, " + String.format("%dx%d, fps: %d", previewWidth, previewHeight, previewFps));

        mWidth = previewWidth;
        mHeight = previewHeight;
        mPreviewFps = previewFps;

        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                initSurfaceView();
            }
        });
    }

    @Override
    public void onLocationChanged(long time, double longitude, double latitude, float speed, float degrees, double altitude) {
        if (mSipUa != null) {
            String strTime = StringUtils.formatDate(time, StringUtils.DEFAULT_DATETIME_PATTERN_WITH_T);
            MobilePos mobilePos = new MobilePos();
            mobilePos.setTime(strTime);
            mobilePos.setLongitude(longitude);
            mobilePos.setLatitude(latitude);
            mobilePos.setSpeed(speed);
            mobilePos.setDirection(degrees);
            mobilePos.setAltitude(altitude);
            mSipUa.setMobilePos(mobilePos);
        }
    }

    /**
     * 视频源预览帧回调触发此回调
     * 注意: 不能在此函数中处理耗时任务
     *
     * @param data MS_TYPE_CAMERA --> 输出nv21数据
     * @param width  视频宽度
     * @param height 视频高度
     */
    @Override
    public void onPreviewFrame(byte[] data, int width, int height) {
        // TODO: 这里面自行做视频数据处理, 比如做视频识别等
    }

    /**
     * 音频源预览帧回调触发此回调
     * 注意: 不能在此函数中处理耗时任务
     *
     * @param data 音频采样数据帧
     * @param sampleRateInHz 如 8000
     * @param channelConfig  如 AudioFormat.CHANNEL_IN_MONO
     * @param audioFormat    如 AudioFormat.ENCODING_PCM_16BIT
     */
    @Override
    public void onPreviewSample(byte[] data, int sampleRateInHz, int channelConfig, int audioFormat) {
        // TODO: 这里面自行做视频数据处理, 比如做视频识别等
    }

    /**
     * 视频错误回调
     * @param code    错误码
     * @param extra   额外值
     * @param message 错误消息
     */
    @Override
    public void onVideoError(int code, int extra, String message) {
        sendMessage(-1, message);
    }

    /**
     * 音频错误回调
     * @param code    错误码
     * @param message 错误消息
     */
    @Override
    public void onAudioError(int code, String message) {
        sendMessage(-1, message);
    }

    /**
     * 从系统回调的摄像头支持的所有分辨率
     * @param res 摄像头分辨率列表数据
     */
    @Subscribe
    public void onCameraResolution(final CameraResolution res) {
        runOnUiThread(() -> {
            mListResolution = SPUtil.getCameraResolution(this);
            if (mListResolution.isEmpty()) {
                SPUtil.setCameraResolution(this, res.getResolution());
                mListResolution = SPUtil.getCameraResolution(this);
            }
            boolean supportedDefault = mListResolution.contains(String.format("%dx%d", mWidth, mHeight));
            if (!supportedDefault) {
                Log.w(TAG, "unsupported preview w: " + mWidth + ", h:" + mHeight);
            }
            initResolutionSpinner();
        });
    }

    /**
     * 从系统回调的摄像头支持的所有帧率
     * @param fr 摄像头帧率列表数据
     */
    @Subscribe
    public void onCameraFramerate(final CameraFramerate fr) {
        runOnUiThread(() -> {
            List<Integer> listFramerate = SPUtil.getCameraFramerate(this);
            if (listFramerate.isEmpty()) {
                SPUtil.setCameraFramerate(this, fr.getFramerate());
            }
        });
    }

    /**
     * 国标设备事件回调
     * @param cb 回调消息
     */
    @Subscribe
    public void onGBEventCallback(final GBEventCallback cb) {
        sendMessage(cb.getCode(), cb.getName(), cb.getParam());
    }

    /**
     * 显示推流的状态
     */
    private void sendMessage(int code, String message, byte[] param) {
        Message msg = Message.obtain();
        msg.what = MSG_STATE;
        Bundle bundle = new Bundle();
        bundle.putInt(GB_CODE, code);
        bundle.putString(GB_STATE, message);
        if (param != null) {
            bundle.putByteArray(GB_PARAM, param);
        }
        msg.setData(bundle);
        mHandler.sendMessage(msg);
    }

    private void sendMessage(int code, String message) {
        sendMessage(code, message, null);
    }

    /**
     * 打开设置页面
     */
    public void onClickSetting(View view) {
        if (mMediaRecorder != null && mMediaRecorder.isRecording()) {
            AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
            builder.setTitle("打开设置页面");
            builder.setMessage("当前正在推流，打开设置页面后摄像头预览将停止且推流结束");
            builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    mHandlerAsync.post(() -> {
                        if (mSipUa != null) {
                            mSipUa.unregister();
                        }
                    });
                    Intent intent = new Intent(MainActivity.this, SettingsActivity.class);
                    intent.putExtra("width", mWidth);
                    intent.putExtra("height", mHeight);
                    startActivityForResult(intent, REQUEST_SETTINGS);
                    overridePendingTransition(R.anim.slide_right_in, R.anim.slide_left_out);
                }
            });
            builder.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                }
            });
            builder.show();
        } else {
            Intent intent = new Intent(MainActivity.this, SettingsActivity.class);
            intent.putExtra("width", mWidth);
            intent.putExtra("height", mHeight);
            startActivityForResult(intent, REQUEST_SETTINGS);
            overridePendingTransition(R.anim.slide_right_in, R.anim.slide_left_out);
        }
    }

    /**
     * 切换分辨率
     */
    public void onClickResolution(View view) {
        mSpinnerResolution.performClick();
    }

    /**
     * 显示提示框信息
     * @param msg    消息
     * @param length 长度
     */
    private void showToast(final String msg, int length) {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(getApplicationContext(), msg, length).show();
            }
        });
    }

    /**
     * 视频采集源切换
     * @param type MS_TYPE_CAMERA / MS_TYPE_SCREEN
     */
    @Override
    public void onVideoInputSelected(MediaSourceType type) {
        if (mMediaRecorder != null && mMediaRecorder.isRecording()) {
            AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
            builder.setTitle("切换视频源");
            builder.setMessage("当前正在推流，切换视频将推流结束");
            builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    mHandlerAsync.post(() -> {
                        if (mSipUa != null) {
                            mSipUa.unregister();
                        }
                    });

                    if (type != mSourceType) {
                        mSourceType = type;
                        switchMediaSource();
                        initMediaRecorder();
                    }
                }
            });
            builder.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                }
            });
            builder.show();
        } else {
            if (type != mSourceType) {
                mSourceType = type;
                switchMediaSource();
                initMediaRecorder();
            }
        }
    }
}
