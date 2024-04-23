package com.nan.gbd.library.source.sceen.impl;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteCallbackList;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.WindowManager;

import com.nan.gbd.library.aidl.INotification;
import com.nan.gbd.library.aidl.IScreenSharing;
import com.nan.gbd.library.media.VideoEncoderConfig;
import com.nan.gbd.library.source.sceen.Constant;
import com.nan.gbd.library.source.sceen.GBScreen;
import com.nan.gbd.library.source.sceen.gles.GLRender;
import com.nan.gbd.library.source.sceen.gles.ImgTexFrame;
import com.nan.gbd.library.source.sceen.gles.SinkConnector;

import java.io.IOException;

/**
 * @author NanGBD
 * @date 2020.7.1
 */
public class ScreenSharingService extends Service {

    private static final String TAG = ScreenSharingService.class.getSimpleName();

    private Context mContext;
    private ScreenCapture mScreenCapture;
    private boolean mIsLandSpace = false;
    private int mWidth = 1280, mHeight = 720;

    private RemoteCallbackList<INotification> mCallbacks = new RemoteCallbackList<INotification>();

    private final IScreenSharing.Stub mBinder = new IScreenSharing.Stub() {
        public void registerCallback(INotification cb) {
            if (cb != null) mCallbacks.register(cb);
        }

        public void unregisterCallback(INotification cb) {
            if (cb != null) mCallbacks.unregister(cb);
        }

        public void startShare() {
            startCapture();
        }

        public void stopShare() {
            stopCapture();
        }

        public void renewToken(String token) {
            refreshToken(token);
        }
    };

    private void initModules() {
        WindowManager wm = (WindowManager) getApplicationContext().getSystemService(Context.WINDOW_SERVICE);
        DisplayMetrics metrics = new DisplayMetrics();
        wm.getDefaultDisplay().getMetrics(metrics);

        int screenWidth = wm.getDefaultDisplay().getWidth();
        int screenHeight = wm.getDefaultDisplay().getHeight();
        if ((mIsLandSpace && screenWidth < screenHeight) ||
                (!mIsLandSpace) && screenWidth > screenHeight) {
            screenWidth = wm.getDefaultDisplay().getHeight();
            screenHeight = wm.getDefaultDisplay().getWidth();
        }

        if (mScreenCapture == null) {
            mScreenCapture = new ScreenCapture(mContext, mWidth, mHeight, metrics.densityDpi);
        }

//        if (mScreenGLRender == null) {
//            mScreenGLRender = new GLRender();
//        }
//        mScreenCapture.mImgTexSrcConnector.connect(new SinkConnector<ImgTexFrame>() {
//            @Override
//            public void onFormatChanged(Object obj) {
//                Log.d(TAG, "onFormatChanged " + obj.toString());
//            }
//
//            @Override
//            public void onFrameAvailable(ImgTexFrame frame) {
//                Log.d(TAG, "onFrameAvailable " + frame.toString());
//                mSCS.getConsumer().consumeTextureFrame(frame.mTextureId, VideoFrame.FORMAT_TEXTURE_OES,
//                        frame.mFormat.mWidth, frame.mFormat.mHeight, 0, frame.pts, frame.mTexMatrix);
//            }
//        });

        mScreenCapture.setOnScreenCaptureListener(new ScreenCapture.OnScreenCaptureListener() {
            @Override
            public void onStarted() {
                Log.d(TAG, "Screen Record Started");
            }

            @Override
            public void onError(int err) {
                Log.e(TAG, "onError " + err);
                switch (err) {
                    case ScreenCapture.SCREEN_ERROR_SYSTEM_UNSUPPORTED:
                        break;
                    case ScreenCapture.SCREEN_ERROR_PERMISSION_DENIED:
                        break;
                    default:break;
                }
            }
        });

        setOffscreenPreview(screenWidth, screenHeight);
    }

    private void deInitModules() {
        if (mScreenCapture != null) {
            mScreenCapture.release();
            mScreenCapture = null;
        }

//        if (mScreenGLRender != null) {
//            mScreenGLRender.quit();
//            mScreenGLRender = null;
//        }
    }

    /**
     * Set offscreen preview.
     *
     * @param width  offscreen width
     * @param height offscreen height
     * @throws IllegalArgumentException
     */
    public void setOffscreenPreview(int width, int height) throws IllegalArgumentException {
        if (width <= 0 || height <= 0) {
            throw new IllegalArgumentException("Invalid offscreen resolution");
        }
        //mScreenGLRender.init(width, height);
    }

    private boolean startCapture() {
        return mScreenCapture.start();
    }

    private void stopCapture() {
        mScreenCapture.stop();
    }

    private void refreshToken(String token) {
    }

    @Override
    public void onCreate() {
        mContext = getApplicationContext();
    }

    @Override
    public IBinder onBind(Intent intent) {
        try {
            mWidth = intent.getIntExtra(Constant.WIDTH, 0);
            mHeight = intent.getIntExtra(Constant.HEIGHT, 0);
            initModules();
            setUpVideoConfig(intent);
        } catch (IOException e) {
            e.printStackTrace();
        }
        return mBinder;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        deInitModules();
    }

//    private void setUpEngine(Intent intent) {
//        String appId = intent.getStringExtra(Constant.APP_ID);
//        try {
//            mRtcEngine = RtcEngine.create(getApplicationContext(), appId, new IRtcEngineEventHandler() {
//                @Override
//                public void onJoinChannelSuccess(String channel, int uid, int elapsed) {
//                    Log.d(TAG, "onJoinChannelSuccess " + channel + " " + elapsed);
//                }
//
//                @Override
//                public void onWarning(int warn) {
//                    Log.d(TAG, "onWarning " + warn);
//                }
//
//                @Override
//                public void onError(int err) {
//                    Log.d(TAG, "onError " + err);
//                }
//
//                @Override
//                public void onRequestToken() {
//                    final int N = mCallbacks.beginBroadcast();
//                    for (int i = 0; i < N; i++) {
//                        try {
//                            mCallbacks.getBroadcastItem(i).onError(Constants.ERR_INVALID_TOKEN);
//                        } catch (RemoteException e) {
//                            // The RemoteCallbackList will take care of removing
//                            // the dead object for us.
//                        }
//                    }
//                    mCallbacks.finishBroadcast();
//                }
//
//                @Override
//                public void onTokenPrivilegeWillExpire(String token) {
//                    final int N = mCallbacks.beginBroadcast();
//                    for (int i = 0; i < N; i++) {
//                        try {
//                            mCallbacks.getBroadcastItem(i).onTokenWillExpire();
//                        } catch (RemoteException e) {
//                            // The RemoteCallbackList will take care of removing
//                            // the dead object for us.
//                        }
//                    }
//                    mCallbacks.finishBroadcast();
//                }
//
//                @Override
//                public void onConnectionStateChanged(int state, int reason) {
//                    switch (state) {
//                        case Constants.CONNECTION_STATE_FAILED :
//                            final int N = mCallbacks.beginBroadcast();
//                            for (int i = 0; i < N; i++) {
//                                try {
//                                    mCallbacks.getBroadcastItem(i).onError(Constants.CONNECTION_STATE_FAILED);
//                                } catch (RemoteException e) {
//                                    // The RemoteCallbackList will take care of removing
//                                    // the dead object for us.
//                                }
//                            }
//                            mCallbacks.finishBroadcast();
//                            break;
//                        default :
//                            break;
//                    }
//                }
//            });
//        } catch (Exception e) {
//            Log.e(TAG, Log.getStackTraceString(e));
//
//            throw new RuntimeException("NEED TO check rtc sdk init fatal error\n" + Log.getStackTraceString(e));
//        }
//
//        mRtcEngine.setLogFile("/sdcard/ss_svr.log");
//        mRtcEngine.setChannelProfile(Constants.CHANNEL_PROFILE_LIVE_BROADCASTING);
//        mRtcEngine.enableVideo();
//
//        if (mRtcEngine.isTextureEncodeSupported()) {
//            mSCS = new ScreenCaptureSource();
//            mRtcEngine.setVideoSource(mSCS);
//        } else {
//            throw new RuntimeException("Can not work on device do not supporting texture" + mRtcEngine.isTextureEncodeSupported());
//        }
//
//        mRtcEngine.setClientRole(Constants.CLIENT_ROLE_BROADCASTER);
//        mRtcEngine.muteAllRemoteAudioStreams(true);
//        mRtcEngine.muteAllRemoteVideoStreams(true);
//        mRtcEngine.disableAudio();
//    }

    private void setUpVideoConfig(Intent intent) throws IOException {
        int width = intent.getIntExtra(Constant.WIDTH, 0);
        int height = intent.getIntExtra(Constant.HEIGHT, 0);
        int frameRate = intent.getIntExtra(Constant.FRAME_RATE, 15);
        int bitrate = intent.getIntExtra(Constant.BITRATE, 500000);
        int gop = intent.getIntExtra(Constant.GOP, 1);
        int orientationMode = intent.getIntExtra(Constant.ORIENTATION_MODE, 0);
        VideoEncoderConfig.FrameRate fps;
        VideoEncoderConfig.OrientationMode om;

        switch (frameRate) {
            case 1 :
                fps = VideoEncoderConfig.FrameRate.FRAME_RATE_FPS_1;
                break;
            case 7 :
                fps = VideoEncoderConfig.FrameRate.FRAME_RATE_FPS_7;
                break;
            case 10 :
                fps = VideoEncoderConfig.FrameRate.FRAME_RATE_FPS_10;
                break;
            case 15 :
                fps = VideoEncoderConfig.FrameRate.FRAME_RATE_FPS_15;
                break;
            case 24 :
                fps = VideoEncoderConfig.FrameRate.FRAME_RATE_FPS_24;
                break;
            case 30 :
                fps = VideoEncoderConfig.FrameRate.FRAME_RATE_FPS_30;
                break;
            default :
                fps = VideoEncoderConfig.FrameRate.FRAME_RATE_FPS_15;
                break;
        }

        switch (orientationMode) {
            case 1 :
                om = VideoEncoderConfig.OrientationMode.ORIENTATION_MODE_FIXED_LANDSCAPE;
                break;
            case 2 :
                om = VideoEncoderConfig.OrientationMode.ORIENTATION_MODE_FIXED_PORTRAIT;
                break;
            default :
                om = VideoEncoderConfig.OrientationMode.ORIENTATION_MODE_ADAPTIVE;
                break;
        }

        mScreenCapture.mMediaCodec.init(HwMediaCodec.MIME_TYPE_AVC, width, height, bitrate, frameRate, gop);
        mScreenCapture.mMediaCodec.setCodecStateCb(GBScreen.getInstance(null));
        mScreenCapture.mSurface = mScreenCapture.mMediaCodec.mInputSurface;
    }
}

