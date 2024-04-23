package com.nan.gbd.library.source.camera;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.os.Build;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.WindowManager;

import com.nan.gbd.library.JNIBridge;
import com.nan.gbd.library.bus.CameraFramerate;
import com.nan.gbd.library.bus.CameraResolution;
import com.nan.gbd.library.media.GBMediaRecorder;
import com.nan.gbd.library.source.GBMediaSource;
import com.nan.gbd.library.utils.BusUtils;
import com.nan.gbd.library.utils.DeviceUtils;
import com.nan.gbd.library.utils.StringUtils;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * https://github.com/EricLi22/AndroidMultiMedia
 * https://juejin.cn/post/6844903966556291079
 * https://blog.csdn.net/weixin_34315485/article/details/88028505
 * https://github.com/javandoc/MediaPlus
 *
 * @author NanGBD
 * @date 2020.7.1
 */
public class GBCamera extends GBMediaSource implements Camera.PreviewCallback {

    private static final String TAG = GBCamera.class.getSimpleName();

    private GBCamera(Context context) {
        super(context);
    }

    public static GBCamera getInstance(Context context) {
        if (mInstance == null) {
            synchronized (GBCamera.class) {
                if (mInstance == null) {
                    mInstance = new GBCamera(context);
                }
            }
        }
        return mInstance;
    }

    private static volatile GBCamera mInstance;

    // Arbitrary queue depth.  Higher number means more memory allocated & held,
    // lower number means more sensitivity to processing time in the client (and
    // potentially stalling the capturer if it runs out of buffers to write to).
    private static final int NUMBER_OF_CAPTURE_BUFFERS = 1;
    private final Set<byte[]> mQueuedBuffers = new HashSet<byte[]>();

    /**
     * 摄像头对象
     */
    protected Camera mCamera;
    /**
     * 摄像头类型（前置/后置），默认后置
     */
    protected int mCameraId = Camera.CameraInfo.CAMERA_FACING_BACK;
    /**
     * 摄像头参数
     */
    protected Camera.Parameters mParameters = null;
    /**
     * 摄像头支持的预览尺寸集合
     */
    protected List<Camera.Size> mSupportedPreviewSizes;
    /**
     * 摄像头支持的帧率集合
     */
    protected List<Integer> mSupportedFramerates;
    /**
     * 摄像头支持的帧率范围集合
     */
    List<int[]> mSupportedFramerateRanges;

    /**
     * 开始预览
     * @return 0:成功, 否则失败
     */
    @Override
    public int startPreview() {
        try {
            if (mCameraId == Camera.CameraInfo.CAMERA_FACING_BACK)
                mCamera = Camera.open();
            else
                mCamera = Camera.open(mCameraId);
            if (null == mCamera) {
                Log.e(TAG, "open camera " + mCameraId + " failed");
            }

            mCamera.setErrorCallback(new Camera.ErrorCallback(){
                @Override
                public void onError(int error, Camera camera) {
                    //may be Camera.CAMERA_ERROR_EVICTED - Camera was disconnected due to use by higher priority user
                    stopPreview();
                }
            });

            mParameters = mCamera.getParameters();

            // 查看支持的预览尺寸
            if (mSupportedPreviewSizes == null) {
                StringBuilder sb = new StringBuilder();
                mSupportedPreviewSizes = mParameters.getSupportedPreviewSizes();
                for (Camera.Size str : mSupportedPreviewSizes) {
                    sb.append(str.width).append("x").append(str.height).append(";");
                }
                BusUtils.BUS.post(new CameraResolution(sb.toString()));
            }

            // 查看支持的帧率
            if (mSupportedFramerates == null) {
                mSupportedFramerates = mParameters.getSupportedPreviewFrameRates();
                if (!mSupportedFramerates.isEmpty()) {
                    MIN_FRAME_RATE = mSupportedFramerates.get(0);
                    MAX_FRAME_RATE = mSupportedFramerates.get(mSupportedFramerates.size() - 1);

                    StringBuilder sb = new StringBuilder();
                    for (Integer fr : mSupportedFramerates) {
                        sb.append(fr).append(";");
                    }
                    BusUtils.BUS.post(new CameraFramerate(sb.toString()));
                    Log.d(TAG, "supported framerates: " + mSupportedFramerates.toString());
                }
            }

            // 查看支持的帧率范围
            if (mSupportedFramerateRanges == null) {
                mSupportedFramerateRanges = mParameters.getSupportedPreviewFpsRange();
                StringBuilder sb = new StringBuilder();
                for (int[] r : mSupportedFramerateRanges) {
                    sb.append("[");
                    for (int i = 0; i < r.length; i++) {
                        if (i != 0) {
                            sb.append("-");
                        }
                        sb.append(r[i]);
                    }
                    sb.append("]");
                }
                Log.d(TAG, "supported fps ranges " + sb);
            }

            Camera.CameraInfo cameraInfo = setCameraDisplayOrientation(mCameraId, mCamera);
            try {
                //mSurfaceHolder.setFixedSize(mPreviewWidth, mPreviewHeight);
                mCamera.setPreviewDisplay(mSurfaceHolder);
            } catch (IOException e) {
                Log.e(TAG, "setPreviewDisplay failed: " + e.getMessage());
                return GBMediaRecorder.MEDIA_ERROR_CAMERA_SET_PREVIEW_DISPLAY;
            }

            //设置摄像头参数
            prepareCameraParams();
            mCamera.setParameters(mParameters);
            setPreviewCallback();
            mCamera.startPreview();

            mVideoRotate = cameraInfo.orientation;
        } catch (Exception e) {
            Log.e(TAG, "startPreview failed: " + e.getMessage());
            return GBMediaRecorder.MEDIA_ERROR_CAMERA_PREVIEW;
        }
        return 0;
    }

    /**
     * 停止预览
     */
    @Override
    public void stopPreview() {
        if (mCamera != null) {
            try {
                // mCamera.lock();
                mCamera.stopPreview();
                mCamera.setPreviewCallbackWithBuffer(null);
                mQueuedBuffers.clear();
                mCamera.release();
            } catch (Exception e) {
                Log.e(TAG, "stopPreview failed: " + e.getMessage());
            }
            mCamera = null;
        }
        mPreviewFrames = 0;
        mDropFrameIndex = 0;
    }

    private int mDropFrameIndex = 0;
    private long st = System.currentTimeMillis();
    private int mVideoFrameCount;
    private long mLastTimeMs;
    private double mSamplingFps;

    // Calculate sampling FPS
    private void calcSamplingFps() {
        if (mVideoFrameCount == 0) {
            mLastTimeMs = System.currentTimeMillis();
            mVideoFrameCount++;
        } else {
            if (++mVideoFrameCount >= 100) {
                long curTs = System.currentTimeMillis();
                long diffTimeMillis = curTs - mLastTimeMs;
                mSamplingFps = (double) mVideoFrameCount * 1000 / diffTimeMillis;
                mVideoFrameCount = 0;
                Log.i(TAG, "fps++++ " + mSamplingFps);
            }
        }
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (mRecording) {
            mPreviewFrames++;
            long nt = System.currentTimeMillis();
            if (false) {
                Log.d(TAG, "onPreviewFrame: " + nt + ", diff:" + (nt - st) + ", pts diff: " +
                        (nt - st) * 90 + ", frames:" + mPreviewFrames);
                st = nt;
            }
            //Log.d("tttttt", "preview frames " + mPreviewFrames);

            if (mFrameRate >= mPreviewFrameRate) {
                JNIBridge.nanPushVideoFrame(data, nt, true);
            } else {
                // 1 2 3 4 5 - 6 7 8 9 10 - 11 12      13 14 15 - 16 17 18 19 20 - 21 22 23 24 25
                if (mPreviewFrames % (mPreviewFrameRate / mFrameRate) == 0) {
                    JNIBridge.nanPushVideoFrame(data, nt, true);
                }

//                int drop_frame = mPreviewFrameRate / mFrameRate;
//                int drop_frame_remain = mPreviewFrameRate % mFrameRate;
//                if (drop_frame > 1) {
//                    // 丢帧处理: 每 mPreviewFrameRate / mFrameRate 取一帧
//                    if (mPreviewFrames % drop_frame == 0) {
//                        if (mDropFrameIndex > mFrameRate) {
//                            mDropFrameIndex++;
//                            Log.i("tttttt", "  aaaaa " + mDropFrameIndex);
//                        }
//                        if (drop_frame_remain == 0 ||
//                            mDropFrameIndex < mFrameRate /*后面的帧全丢*/) {
//                            JNIBridge.nanPushVideoFrame(data, nt, true);
//                            Log.i("tttttt", "  ppppppppp " + mDropFrameIndex);
//                        }
//                    } else {
//                        mDropFrameIndex++;
//                        Log.i("tttttt", "  dddd " + mDropFrameIndex);
//                    }
//                    if (mDropFrameIndex + mFrameRate >= mPreviewFrameRate) {
//                        Log.i("tttttt", "reset---------" + mDropFrameIndex + " \n\n");
//                        mDropFrameIndex = 0;
//                    }
//                } else {
//                    // 6/15 7/15 8/15 9/15 10/15 11/15 12/15 13/15 14/15
//                    // 7/25 8/15 9/25 10/25 13/25 14/25 15/25 16/25
//
//                    // 丢帧处理: 每两帧取一帧，取drop_frame_remain帧后往后补足 mFrameRate-drop_frame_remain
//                    // 1 2 3 4 5 - 6 7 8 9 10 - 11 12 13 14 15 - 16 17 18 19 20 - 21 22 23 24 25 - 26 27 28 29 30 31
//                    // 1   2   3     4   5  5   6  7  8  9  10   1     2     3       4     5
//                    // 当第一个数是奇数时，选择偶数 mPreviewFrames % 2 == 0 && (mDropFrameIndex % 2) != 0
//                    // 当第一个数是偶数时，选择奇数 mPreviewFrames % 2 != 0 && (mDropFrameIndex % 2) != 0
//
//
//                    if ((mPreviewFrames % 2 == 0 && mDropFrameIndex > 0 && (mPreviewFrames % mDropFrameIndex) == 0) ||
//                        (mPreviewFrames % 2 != 0 && mDropFrameIndex > 0 && (mDropFrameIndex == 1 || ((mPreviewFrames - 1) % mDropFrameIndex) != 0)) ||
//                            /**
//                             * 3 1
//                             * 5 2
//                             * 7 3
//                             *
//                             * 17 1
//                             * 19 2
//                             * 21 3
//                             */
//                            mDropFrameIndex > drop_frame_remain) {
//                        if (mDropFrameIndex <= drop_frame_remain ||
//                            mDropFrameIndex <= mFrameRate /*后面的帧全取*/) {
//                            Log.i("tttttt", "  ppppppppp " + mDropFrameIndex);
//                            JNIBridge.nanPushVideoFrame(data, nt, true);
//                        }
//                        if (mDropFrameIndex <= mFrameRate && mDropFrameIndex >= drop_frame_remain) {
//                            if (mDropFrameIndex == drop_frame_remain) {
//
//                            }
//                            mDropFrameIndex++;
//                            Log.i("tttttt", "  aaaaa " + mDropFrameIndex);
//                        }
//                    } else {
//                        mDropFrameIndex++;
//                        Log.i("tttttt", "  dddd " + mDropFrameIndex);
//                    }
//
//                    if (mDropFrameIndex > mFrameRate) {
//                        Log.i("tttttt", "reset---------" + mDropFrameIndex + " \n\n");
//                        mDropFrameIndex = 0;
//                    }
//                }
            }
        }

        //calcSamplingFps();
        if (null != mOnPreviewListener) {
            mOnPreviewListener.onPreviewFrame(data, mPreviewWidth, mPreviewHeight);
        }

        /**
         * https://www.jianshu.com/p/77a524265f3c  Android Camera onPreviewFrame 回调造成频繁GC的问题
         * https://www.jianshu.com/p/b0b4a20fd90a  再谈onPreviewFrame预览帧率问题
         * 只能最后调用
         */
        camera.addCallbackBuffer(data);
    }

    /**
     * 设置回调
     */
    protected void setPreviewCallback() {
        Camera.Size size = mParameters.getPreviewSize();
        if (size != null) {
            int buffSize = (size.width * size.height * ImageFormat.getBitsPerPixel(mParameters.getPreviewFormat())) / 8;
            try {
                //PixelFormat pf = new PixelFormat();
                //PixelFormat.getPixelFormatInfo(mParameters.getPreviewFormat(), pf);

                mQueuedBuffers.clear();
                for (int i = 0; i < NUMBER_OF_CAPTURE_BUFFERS; ++i) {
                    final ByteBuffer buffer = ByteBuffer.allocateDirect(buffSize);
                    mQueuedBuffers.add(buffer.array());
                    mCamera.addCallbackBuffer(buffer.array());
                }

                mCamera.setPreviewCallbackWithBuffer(this);
            } catch (OutOfMemoryError e) {
                Log.e(TAG, "setPreviewCallback...", e);
            }
            Log.d(TAG, "setPreviewCallbackWithBuffer, width:" + size.width + " height:" + size.height);
        } else {
            mCamera.setPreviewCallback(this);
        }
    }

    /**
     * 是否支持前置摄像头
     */
    @SuppressLint("NewApi")
    @TargetApi(Build.VERSION_CODES.GINGERBREAD)
    public static boolean isSupportFrontCamera() {
        if (!DeviceUtils.hasGingerbread()) {
            return false;
        }
        int numberOfCameras = Camera.getNumberOfCameras();
        if (2 == numberOfCameras) {
            return true;
        }
        return false;
    }

    /**
     * 设置为后置摄像头
     */
    public void setCameraBack() {
        mCameraId = Camera.CameraInfo.CAMERA_FACING_BACK;
    }

    /**
     * 设置为前置摄像头
     */
    public void setCameraFront() {
        mCameraId = Camera.CameraInfo.CAMERA_FACING_FRONT;
    }

    /**
     * 是否前置摄像头
     */
    public boolean isFrontCamera() {
        return mCameraId == Camera.CameraInfo.CAMERA_FACING_FRONT;
    }

    /**
     * 切换前置/后置摄像头
     */
    public void switchCamera(int cameraFacingFront) {
        switch (cameraFacingFront) {
            case Camera.CameraInfo.CAMERA_FACING_FRONT:
            case Camera.CameraInfo.CAMERA_FACING_BACK:
                mCameraId = cameraFacingFront;
                stopPreview();
                startPreview();
                break;
        }
    }

    /**
     * 切换前置/后置摄像头
     */
    public void switchCamera() {
        if (mCameraId == Camera.CameraInfo.CAMERA_FACING_BACK) {
            switchCamera(Camera.CameraInfo.CAMERA_FACING_FRONT);
        } else {
            switchCamera(Camera.CameraInfo.CAMERA_FACING_BACK);
        }
    }

    /**
     * 连续自动对焦是指当场景发生变化时，相机会主动去调节焦距来达到被拍摄的物体始终是清晰的状态
     */
    private String getAutoFocusMode() {
        if (mParameters == null) {
            return null;
        }
        List<String> focusModes = mParameters.getSupportedFocusModes();
        Log.d(TAG, "supported focus: "+ focusModes.toString());
        if ((Build.MODEL.startsWith("GT-I950") || Build.MODEL.endsWith("SCH-I959") || Build.MODEL.endsWith("MEIZU MX3")) &&
                isSupported(focusModes, Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE)) {
            //判断是否支持连续自动对焦图像
            return Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE;
        } else if (isSupported(focusModes, Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)) {
            //判断是否支持连续自动对焦视频
            return Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO;
        } else if (isSupported(focusModes,  Camera.Parameters.FOCUS_MODE_AUTO)) {
            //判断是否支持单次自动对焦
            return Camera.Parameters.FOCUS_MODE_AUTO;
        }
        return null;
    }

    /**
     * 自动对焦
     *
     * @param cb
     * @return
     */
    public boolean autoFocus(Camera.AutoFocusCallback cb) {
        if (mCamera != null) {
            try {
                mCamera.cancelAutoFocus();
                if (mParameters != null) {
                    String mode = getAutoFocusMode();
                    if (StringUtils.isNotEmpty(mode)) {
                        mParameters.setFocusMode(mode);
                        mCamera.setParameters(mParameters);
                    }
                }
                mCamera.autoFocus(cb);
                return true;
            } catch (Exception e) {
                Log.e(TAG, "autoFocus", e);
                // MEDIA_ERROR_CAMERA_AUTO_FOCUS
            }
        }
        return false;
    }

    /**
     * 手动对焦
     *
     * @param focusAreas 对焦区域
     * @return
     */
    @SuppressLint("NewApi")
    @TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
    public boolean manualFocus(Camera.AutoFocusCallback cb, List<Camera.Area> focusAreas) {
        if (mCamera != null && focusAreas != null && mParameters != null && DeviceUtils.hasICS()) {
            try {
                mCamera.cancelAutoFocus();
                // 检测设备是否支持
                if (mParameters.getMaxNumFocusAreas() > 0) {
                    // mParameters.setFocusMode(Camera.Parameters.FOCUS_MODE_MACRO);
                    // Macro(close-up) focus mode
                    mParameters.setFocusAreas(focusAreas);
                }
                if (mParameters.getMaxNumMeteringAreas() > 0) {
                    mParameters.setMeteringAreas(focusAreas);
                }

                mParameters.setFocusMode(Camera.Parameters.FOCUS_MODE_MACRO);
                mCamera.setParameters(mParameters);
                mCamera.autoFocus(cb);
                return true;
            } catch (Exception e) {
                Log.e(TAG, "autoFocus", e);
                // MEDIA_ERROR_CAMERA_AUTO_FOCUS
            }
        }
        return false;
    }

    /**
     * 设置闪光灯
     *
     * @param value
     */
    private boolean setFlashMode(String value) {
        if (mParameters != null && mCamera != null) {
            try {
                if (Camera.Parameters.FLASH_MODE_TORCH.equals(value) || Camera.Parameters.FLASH_MODE_OFF.equals(value)) {
                    mParameters.setFlashMode(value);
                    mCamera.setParameters(mParameters);
                }
                return true;
            } catch (Exception e) {
                Log.e(TAG, "setFlashMode", e);
            }
        }
        return false;
    }

    /**
     * 切换闪关灯，默认关闭
     */
    public boolean toggleFlashMode() {
        if (mParameters == null) {
            return false;
        }
        try {
            final String mode = mParameters.getFlashMode();
            if (TextUtils.isEmpty(mode) || Camera.Parameters.FLASH_MODE_OFF.equals(mode)) {
                setFlashMode(Camera.Parameters.FLASH_MODE_TORCH);
            } else {
                setFlashMode(Camera.Parameters.FLASH_MODE_OFF);
            }
            return true;
        } catch (Exception e) {
            Log.e(TAG, "toggleFlashMode", e);
        }
        return false;
    }

    /**
     * 检测是否支持指定特性
     */
    private boolean isSupported(List<String> list, String key) {
        return list != null && list.contains(key);
    }

    /**
     * 预处理一些拍摄参数
     * 注意：自动对焦参数cam_mode和cam-mode可能有些设备不支持，导致视频画面变形，
     *      需要判断一下，已知有"GT-N7100", "GT-I9308"会存在这个问题
     */
    @SuppressWarnings("deprecation")
    protected void prepareCameraParams() {
        if (mParameters == null)
            return;

        if (!mSupportedFramerates.contains(mPreviewFrameRate)) {
            boolean foundFrame = false;
            Collections.sort(mSupportedFramerates);
            for (int i = mSupportedFramerates.size() - 1; i >= 0; i--) {
                if (mSupportedFramerates.get(i) <= MAX_FRAME_RATE) {
                    mPreviewFrameRate = mSupportedFramerates.get(i);
                    foundFrame = true;
                    break;
                }
            }
            if (!foundFrame) {
                mPreviewFrameRate = mSupportedFramerates.get(0);
            }
        }
        Log.d(TAG, "selected framerate: " + mPreviewFrameRate);

        try {
            mParameters.setPreviewFpsRange(mPreviewFrameRate * 1000, mPreviewFrameRate * 1000);
        } catch (Exception e) {
            mParameters.setPreviewFrameRate(mPreviewFrameRate);
        }

        // 设置预览尺寸
        boolean foundWidth = false;
        int width = 0, height = 0;
        for (int i = mSupportedPreviewSizes.size() - 1; i >= 0; i--) {
            Camera.Size size = mSupportedPreviewSizes.get(i);
            if (size.height == mPreviewHeight) {
                if (size.width == mPreviewWidth) {
                    foundWidth = true;
                    break;
                }
            }
            // 找到最接近的一个分辨率
            if (width == 0 && height == 0 && (mPreviewHeight < size.height + 100 &&
                ((mPreviewWidth <= mPreviewHeight && size.width <= size.height) ||
                 (mPreviewWidth >= mPreviewHeight && size.width >= size.height)))) {
                width = size.width;
                height = size.height;
            }
        }
        if (!foundWidth) {
            if (width != 0) {
                mPreviewWidth = width;
                mPreviewHeight = height;
            } else {
                Log.e(TAG, "传入高度不支持或未找到对应宽度,请按照要求重新设置，否则会出现一些严重问题");
                mPreviewWidth = VIDEO_WIDTH;
                mPreviewHeight = VIDEO_HEIGHT;
            }
        }
        mParameters.setPreviewSize(mPreviewWidth, mPreviewHeight);
        mParameters.setPictureSize(mPreviewWidth, mPreviewHeight);
        Log.d(TAG, "selected preview size: " + mPreviewWidth + "x" + mPreviewHeight);

        List<Integer> supportedPreviewFormats = mParameters.getSupportedPreviewFormats();
        Log.d(TAG, "supported preview formats: " + supportedPreviewFormats.toString());
        mParameters.setPreviewFormat(ImageFormat.NV21);
        mParameters.setRecordingHint(true);

        // 设置自动连续对焦
        String mode = getAutoFocusMode();
        if (StringUtils.isNotEmpty(mode)) {
            mParameters.setFocusMode(mode);
        }

        // 设置人像模式用来拍摄人物相片，如证件照。数码相机会把光圈调到最大，做出浅景深的效果。
        // 而有些相机还会使用能够表现更强肤色效果的色调、对比度或柔化效果进行拍摄，以突出人像主体。
        //if (mCameraId == Camera.CameraInfo.CAMERA_FACING_FRONT &&
        //    isSupported(mParameters.getSupportedSceneModes(), Camera.Parameters.SCENE_MODE_PORTRAIT))
        //    mParameters.setSceneMode(Camera.Parameters.SCENE_MODE_PORTRAIT);

        if (isSupported(mParameters.getSupportedWhiteBalance(), Camera.Parameters.WHITE_BALANCE_AUTO)) {
            mParameters.setWhiteBalance(Camera.Parameters.WHITE_BALANCE_AUTO);
        }

        //是否支持视频防抖
        if (mParameters.isVideoStabilizationSupported())
            mParameters.setVideoStabilization(true);

        if (!DeviceUtils.isDevice("GT-N7100", "GT-I9308", "GT-I9300")) {
            mParameters.set("cam_mode", 1);
            mParameters.set("cam-mode", 1);
        }
    }

    /**
     * 前置摄像头要转换270度
     * 后置摄像头转换90度
     * https://github.com/byhook/ffmpeg4android/blob/master/ffmpeg-camera-stream/src/main/java/com/onzhou/ffmpeg/camera/CameraV1.java
     * https://www.jianshu.com/p/3ba0dee1732f
     * @param cameraId
     * @param camera
     */
    private Camera.CameraInfo setCameraDisplayOrientation(int cameraId, Camera camera) {
        if (mContext == null) {
            if (mCameraId == Camera.CameraInfo.CAMERA_FACING_BACK) {
                camera.setDisplayOrientation(90);
            } else {
                camera.setDisplayOrientation(270);
            }
        }
        // 相机旋转角度
        Camera.CameraInfo info = new Camera.CameraInfo();
        Camera.getCameraInfo(cameraId, info);
        // 屏幕显示旋转角度
        int rotation = ((WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay().getRotation();
        int degrees = 0;
        switch (rotation) {
            case Surface.ROTATION_0: degrees = 0; break;
            case Surface.ROTATION_90: degrees = 90; break;
            case Surface.ROTATION_180: degrees = 180; break;
            case Surface.ROTATION_270: degrees = 270; break;
        }
        int displayDegree;
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_BACK) {
            displayDegree = (info.orientation - degrees + 360) % 360;
        } else {
            displayDegree = (info.orientation + degrees) % 360;
            displayDegree = (360 - displayDegree) % 360; // compensate the mirror
        }
        // 预览数据显示旋转角度: 根据相机旋转角度和屏幕显示旋转角度, 计算预览数据显示旋转角度
        camera.setDisplayOrientation(displayDegree);
        Log.i(TAG, "camera id: " + mCameraId + " orientation: " + info.orientation +
                " rotation: " + rotation + " display degree: " + displayDegree);
        return info;
    }
}
