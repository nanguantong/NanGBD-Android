package com.nan.gbd.library.source.sceen.impl;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.util.Log;
import android.view.Surface;
import android.view.Window;

import java.lang.ref.WeakReference;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * capture video frames from screen
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class ScreenCapture //implements SurfaceTexture.OnFrameAvailableListener
{
    private static final String TAG = ScreenCapture.class.getSimpleName();

    private final static boolean TRACE = true;
    private static final boolean DEBUG_ENABLED = true;
    public static final int REQUEST_MEDIA_PROJECTION_CODE = 1001;

    private Context mContext;
    private OnScreenCaptureListener mOnScreenCaptureListener;
    public MediaProjectionManager mMediaProjectManager;
    private MediaProjection mMediaProjection;
    private VirtualDisplay mVirtualDisplay;
    public Intent mProjectionIntent;

    private int mWidth = 1280;
    private int mHeight = 720;

    public final static int SCREEN_STATE_IDLE = 0;
    public final static int SCREEN_STATE_INITIALIZING = 1;
    public final static int SCREEN_STATE_INITIALIZED = 2;
    public final static int SCREEN_STATE_STOPPING = 3;
    public final static int SCREEN_STATE_CAPTURING = 4;

    public final static int SCREEN_ERROR_SYSTEM_UNSUPPORTED = -1;
    public final static int SCREEN_ERROR_PERMISSION_DENIED = -2;

    public final static int SCREEN_RECORD_STARTED = 4;
    public final static int SCREEN_RECORD_FAILED = 5;

    private final static int MSG_SCREEN_START_SCREEN_ACTIVITY = 1;
    private final static int MSG_SCREEN_INIT_PROJECTION = 2;
    private final static int MSG_SCREEN_START = 3;
    private final static int MSG_SCREEN_RELEASE = 4;
    private final static int MSG_SCREEN_QUIT = 5;

    private final static int RELEASE_SCREEN_THREAD = 1;

    private AtomicInteger mState;
    public HwMediaCodec mMediaCodec;
    public Surface mSurface;

//    private GLRender mGLRender;
//    private int mTextureId;
//    private SurfaceTexture mSurfaceTexture;
//    private boolean mTexInited = false;
//    private ImgTexFormat mImgTexFormat;
//    /**
//     * Source pin transfer ImgTexFrame, used for gpu path and preview
//     */
//    public SrcConnector<ImgTexFrame> mImgTexSrcConnector;

    private Handler mMainHandler;
    private HandlerThread mScreenSetupThread;
    private Handler mScreenSetupHandler;

    private int mScreenDensity;

    // fill extra frame
    private Runnable mFillFrameRunnable;

    // Performance trace
    private long mLastTraceTime;
    private long mFrameDrawed;

    public ScreenCapture(Context context, int width, int height, int density) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
            throw new RuntimeException("Need API level " + Build.VERSION_CODES.LOLLIPOP);
        }

        if (context == null) {
            throw new IllegalArgumentException("the context must be not null");
        }

        mContext = context;
        mWidth = width;
        mHeight = height;
        mScreenDensity = density;

        mMediaCodec = new HwMediaCodec();
        mMainHandler = new MainHandler(this);
        mState = new AtomicInteger(SCREEN_STATE_IDLE);

//        mGLRender = render;
//        mGLRender.addListener(mGLRenderListener);
//        mImgTexSrcConnector = new SrcConnector<>();
//        mFillFrameRunnable = new Runnable() {
//            @Override
//            public void run() {
//                if (mState.get() == SCREEN_STATE_CAPTURING) {
//                    mGLRender.requestRender();
//                    mMainHandler.postDelayed(mFillFrameRunnable, 100);
//                }
//            }
//        };

        initScreenSetupThread();
    }

    /**
     * Start screen record.<br/>
     * Can only be called on mState IDLE.
     */
    public boolean start() {
        if (DEBUG_ENABLED) {
            Log.d(TAG, "start");
        }

        if (mState.get() != SCREEN_STATE_IDLE) {
            return false;
        }

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
            Message msg = mMainHandler.obtainMessage(SCREEN_RECORD_FAILED, SCREEN_ERROR_SYSTEM_UNSUPPORTED, 0);
            mMainHandler.sendMessage(msg);
            return false;
        }

        mState.set(SCREEN_STATE_INITIALIZING);
        mScreenSetupHandler.removeMessages(MSG_SCREEN_START_SCREEN_ACTIVITY);
        mScreenSetupHandler.sendEmptyMessage(MSG_SCREEN_START_SCREEN_ACTIVITY);
        return true;
    }

    /**
     * stop screen record
     */
    public void stop() {
        if (DEBUG_ENABLED) {
            Log.d(TAG, "stop");
        }

        if (mState.get() == SCREEN_STATE_IDLE) {
            return;
        }
        mState.set(SCREEN_STATE_STOPPING);

        // stop fill frame
        mMainHandler.removeCallbacks(mFillFrameRunnable);

        Message msg = new Message();
        msg.what = MSG_SCREEN_RELEASE;
        msg.arg1 = ~RELEASE_SCREEN_THREAD;

        mScreenSetupHandler.removeMessages(MSG_SCREEN_RELEASE);
        mScreenSetupHandler.sendMessage(msg);
    }

    public void release() {
        // stop fill frame
        if (mMainHandler != null) {
            mMainHandler.removeCallbacks(mFillFrameRunnable);
        }

        if (mState.get() == SCREEN_STATE_IDLE) {
            mScreenSetupHandler.removeMessages(MSG_SCREEN_QUIT);
            mScreenSetupHandler.sendEmptyMessage(MSG_SCREEN_QUIT);
            quitScreenSetupThread();
            return;
        }

        Message msg = new Message();
        msg.what = MSG_SCREEN_RELEASE;
        msg.arg1 = RELEASE_SCREEN_THREAD;

        mState.set(SCREEN_STATE_STOPPING);
        mScreenSetupHandler.removeMessages(MSG_SCREEN_RELEASE);
        mScreenSetupHandler.sendMessage(msg);

        quitScreenSetupThread();
    }

    /**
     * screen status changed listener
     *
     * @param listener
     */
    public void setOnScreenCaptureListener(OnScreenCaptureListener listener) {
        mOnScreenCaptureListener = listener;
    }

//    @Override
//    public void onFrameAvailable(SurfaceTexture st) {
//        if (mState.get() != SCREEN_STATE_CAPTURING) {
//            return;
//        }
//        mGLRender.requestRender();
//        if (mMainHandler != null) {
//            mMainHandler.removeCallbacks(mFillFrameRunnable);
//            mMainHandler.postDelayed(mFillFrameRunnable, 100);
//        }
//    }

//    private void initTexFormat() {
//        mImgTexFormat = new ImgTexFormat(ImgTexFormat.COLOR_FORMAT_EXTERNAL_OES, mWidth, mHeight);
//        mImgTexSrcConnector.onFormatChanged(mImgTexFormat);
//    }

    public final void initProjection(int requestCode, int resultCode, Intent intent) {
        if (DEBUG_ENABLED) {
            Log.d(TAG, "initProjection");
        }

        if (requestCode != REQUEST_MEDIA_PROJECTION_CODE) {
            if (DEBUG_ENABLED) {
                Log.d(TAG, "Unknown request code: " + requestCode);
            }
        } else if (resultCode != Activity.RESULT_OK) {
            Log.e(TAG, "Screen Cast Permission Denied, resultCode: " + resultCode);
            Message msg = mMainHandler.obtainMessage(SCREEN_RECORD_FAILED, SCREEN_ERROR_PERMISSION_DENIED, 0);
            mMainHandler.sendMessage(msg);
            stop();
        } else {
            // get media projection and virtual display
            mMediaProjection = mMediaProjectManager.getMediaProjection(resultCode, intent);

            if (mSurface != null) {
                startScreenCapture();
            } else {
                mState.set(SCREEN_STATE_INITIALIZED);
            }
        }
    }

//    private GLRender.GLRenderListener mGLRenderListener = new GLRender.GLRenderListener() {
//        @Override
//        public void onReady() {
//            Log.d(TAG, "onReady");
//        }
//
//        @Override
//        public void onSizeChanged(int width, int height) {
//            Log.d(TAG, "onSizeChanged : " + width + "*" + height);
//            mWidth = width;
//            mHeight = height;
//
//            if (mVirtualDisplay != null) {
//                mVirtualDisplay.release();
//                mVirtualDisplay = null;
//            }
//
//            mTexInited = false;
//            mTextureId = GlUtil.createOESTextureObject();
//            if (mSurfaceTexture != null) {
//                mSurfaceTexture.release();
//            }
//            mSurfaceTexture = new SurfaceTexture(mTextureId);
//            mSurfaceTexture.setDefaultBufferSize(mWidth, mHeight);
//            mSurfaceTexture.setOnFrameAvailableListener(ScreenCapture.this);
//            if (mSurface != null) {
//                mSurface.release();
//            }
//            mSurface = new Surface(mSurfaceTexture);
//
//            if (mState.get() >= SCREEN_STATE_INITIALIZED && mVirtualDisplay == null) {
//                mScreenSetupHandler.removeMessages(MSG_SCREEN_START);
//                mScreenSetupHandler.sendEmptyMessage(MSG_SCREEN_START);
//            }
//        }
//
//        @Override
//        public void onDrawFrame() {
//            long pts = System.nanoTime() / 1000000;
//            try {
//                mSurfaceTexture.updateTexImage();
//            } catch (Exception e) {
//                Log.e(TAG, "updateTexImage failed, ignore");
//                return;
//            }
//
//            if (!mTexInited) {
//                mTexInited = true;
//                initTexFormat();
//            }
//            float[] texMatrix = new float[16];
//            mSurfaceTexture.getTransformMatrix(texMatrix);
//            ImgTexFrame frame = new ImgTexFrame(mImgTexFormat, mTextureId, texMatrix, pts);
//            try {
//                mImgTexSrcConnector.onFrameAvailable(frame);
//            } catch (Exception e) {
//                e.printStackTrace();
//                Log.e(TAG, "Draw frame failed, ignore");
//            }
//
//            if (TRACE) {
//                mFrameDrawed++;
//                long tm = System.currentTimeMillis();
//                long tmDiff = tm - mLastTraceTime;
//                if (tmDiff >= 5000) {
//                    float fps = mFrameDrawed * 1000.f / tmDiff;
//                    Log.d(TAG, "screen fps: " + String.format(Locale.getDefault(), "%.2f", fps));
//                    mFrameDrawed = 0;
//                    mLastTraceTime = tm;
//                }
//            }
//        }
//
//        @Override
//        public void onReleased() {
//        }
//    };

    private void startScreenCapture() {
        mVirtualDisplay = mMediaProjection.createVirtualDisplay("ScreenCapture",
                mWidth, mHeight, mScreenDensity,
                DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC, mSurface, null, null);

        mMediaCodec.start();

        mState.set(SCREEN_STATE_CAPTURING);
        Message msg = mMainHandler.obtainMessage(SCREEN_RECORD_STARTED, 0, 0);
        mMainHandler.sendMessage(msg);
    }

    private static class MainHandler extends Handler {
        private final WeakReference<ScreenCapture> weakCapture;

        public MainHandler(ScreenCapture screenCapture) {
            super();
            this.weakCapture = new WeakReference<>(screenCapture);
        }

        @Override
        public void handleMessage(Message msg) {
            ScreenCapture screenCapture = weakCapture.get();
            if (screenCapture == null) {
                return;
            }
            switch (msg.what) {
                case SCREEN_RECORD_STARTED:
                    if (screenCapture.mOnScreenCaptureListener != null) {
                        screenCapture.mOnScreenCaptureListener.onStarted();
                    }
                    break;
                case SCREEN_RECORD_FAILED:
                    if (screenCapture.mOnScreenCaptureListener != null) {
                        screenCapture.mOnScreenCaptureListener.onError(msg.arg1);
                    }
                    break;
                default:
                    break;
            }
        }
    }

    private void initScreenSetupThread() {
        mScreenSetupThread = new HandlerThread("screen_setup_thread", Thread.NORM_PRIORITY);
        mScreenSetupThread.start();
        mScreenSetupHandler = new Handler(mScreenSetupThread.getLooper()) {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_SCREEN_START_SCREEN_ACTIVITY: {
                        screenSetup();
                        break;
                    }
                    case MSG_SCREEN_INIT_PROJECTION: {
                        initProjection(msg.arg1, msg.arg2, mProjectionIntent);
                        break;
                    }
                    case MSG_SCREEN_START: {
                        startScreenCapture();
                        break;
                    }
                    case MSG_SCREEN_RELEASE: {
                        screenRelease(msg.arg1);
                        break;
                    }
                    case MSG_SCREEN_QUIT: {
                        mScreenSetupThread.quit();
                    }
                }
            }
        };
    }

    private void quitScreenSetupThread() {
        try {
            mScreenSetupThread.join();
        } catch (InterruptedException e) {
            Log.d(TAG, "quitThread " + Log.getStackTraceString(e));
        } finally {
            mScreenSetupThread = null;
        }

        if (mMainHandler != null) {
            mMainHandler.removeCallbacksAndMessages(null);
            mMainHandler = null;
        }
    }

    private void screenSetup() {
        if (DEBUG_ENABLED) {
            Log.d(TAG, "screenSetup");
        }

        if (mMediaProjectManager == null) {
            mMediaProjectManager = (MediaProjectionManager) mContext.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
        }

        Intent intent = new Intent(mContext, ScreenCapture.ScreenCaptureAssistantActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        ScreenCapture.ScreenCaptureAssistantActivity.mScreenCapture = this;
        mContext.startActivity(intent);
    }

    private void screenRelease(int isQuit) {
        if (DEBUG_ENABLED) {
            Log.d(TAG, "screenRelease");
        }

        mState.set(SCREEN_STATE_IDLE);

        if (mVirtualDisplay != null) {
            mVirtualDisplay.release();
            mVirtualDisplay = null;
        }

        if (mMediaProjection != null) {
            mMediaProjection.stop();
            mMediaProjection = null;
        }

        if (mMediaCodec != null) {
            mMediaCodec.stop();
            mMediaCodec.release();
            mMediaCodec = null;
        }

        if (isQuit == RELEASE_SCREEN_THREAD) {
            mScreenSetupHandler.sendEmptyMessage(MSG_SCREEN_QUIT);
        }
    }

    public static class ScreenCaptureAssistantActivity extends Activity {
        public static ScreenCapture mScreenCapture;

        @Override
        public void onCreate(Bundle bundle) {
            super.onCreate(bundle);
            requestWindowFeature(Window.FEATURE_NO_TITLE);
            if (mScreenCapture.mMediaProjectManager == null) {
                mScreenCapture.mMediaProjectManager =
                        (MediaProjectionManager) this.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
            }

            this.startActivityForResult(mScreenCapture.mMediaProjectManager.createScreenCaptureIntent(),
                    ScreenCapture.REQUEST_MEDIA_PROJECTION_CODE);
        }

        @Override
        public void onActivityResult(int requestCode, int resultCode, Intent intent) {
            if (mScreenCapture != null && mScreenCapture.mState.get() != SCREEN_STATE_IDLE) {
                Message msg = new Message();
                msg.what = MSG_SCREEN_INIT_PROJECTION;
                msg.arg1 = requestCode;
                msg.arg2 = resultCode;
                mScreenCapture.mProjectionIntent = intent;
                mScreenCapture.mScreenSetupHandler.removeMessages(MSG_SCREEN_INIT_PROJECTION);
                mScreenCapture.mScreenSetupHandler.sendMessage(msg);
            }
            mScreenCapture = null;
            finish();
        }
    }

    public interface OnScreenCaptureListener {

        /**
         * Notify screen capture started.
         */
        void onStarted();

        /**
         * Notify error occurred while camera capturing.
         *
         * @param err err code.
         * @see #SCREEN_ERROR_SYSTEM_UNSUPPORTED
         * @see #SCREEN_ERROR_PERMISSION_DENIED
         */
        void onError(int err);
    }

}
