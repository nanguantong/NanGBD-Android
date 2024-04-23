package com.nan.gbd.library.source;

import android.content.Context;
import android.view.SurfaceHolder;

import com.nan.gbd.library.media.GBMediaRecorder;

/**
 * @author NanGBD
 * @date 2020.7.1
 */
public abstract class GBMediaSource {
    protected Context mContext;

    public GBMediaSource(Context context) {
        if (context != null) {
            mContext = context;
        }
    }

    /**
     * 开始预览
     * @return 0:成功, 否则失败
     */
    public abstract int startPreview();

    /**
     * 停止预览
     */
    public abstract void stopPreview();

    /**
     * 视频宽度
     */
    protected static final int VIDEO_WIDTH = 640;
    /**
     * 视频高度
     */
    protected static final int VIDEO_HEIGHT = 480;

    /**
     * 最大帧率
     */
    protected static int MAX_FRAME_RATE = 25;
    /**
     * 最小帧率
     */
    protected static int MIN_FRAME_RATE = 10;

    /**
     * 队列大小
     */
    protected static final int QUEUE_MAX_SIZE = 100;

    /**
     * 画布
     */
    protected SurfaceHolder mSurfaceHolder;

    /**
     * 预览回调
     */
    protected GBMediaRecorder.OnPreviewListener mOnPreviewListener;

    /**
     * 预览录制帧数
     */
    protected volatile long mPreviewFrames = 0;

    /**
     * 视频预览宽高
     */
    protected int mPreviewWidth = VIDEO_WIDTH;
    protected int mPreviewHeight = VIDEO_HEIGHT;

    /**
     * 预览帧率
     */
    protected int mPreviewFrameRate = MAX_FRAME_RATE;

    /**
     * 视频旋转角度
     */
    protected int mVideoRotate;

    public static final int VIDEO_ENCODEC_NONE = 0;
    public static final int VIDEO_ENCODEC_H264 = 1; // h264
    public static final int VIDEO_ENCODEC_H265 = 2; // h265
    /**
     * 视频编码器
     */
    protected String mVideoEncodec = "h264";

    /**
     * 视频码率
     */
    protected int mVideoBitrate = 300000;

    public static final int BITRATE_MODE_CQ = 0;
    /** Variable bitrate mode */
    public static final int BITRATE_MODE_VBR = 1;
    /** Constant bitrate mode */
    public static final int BITRATE_MODE_CBR = 2;
    /**
     * 视频码率控制模式
     */
    protected String mVideoBitrateMode = "cq";

    /**
     * 视频编码帧率
     */
    protected int mFrameRate = MAX_FRAME_RATE;

    /**
     * 关键帧间隔(秒)
     */
    protected int mGop = 5;

    /**
     * 视频profile
     */
    protected String mVideoProfile = "baseline";

    protected static int MAX_THREAD_NUM = 8;
    /**
     * 视频编码线程数,-1是硬编码
     */
    protected int mThreadNum = -1;

    /**
     * 音视频缓存最大帧数
     */
    protected int mAvQueueSize = QUEUE_MAX_SIZE;

    /**
     * 视频比特率上下浮动因子[0, 1]
     */
    protected double mVideoBitrateFactor = 0.3f;

    /**
     * 视频比特率动态调整周期(秒)
     */
    protected int mVideoBitrateAdjustPeriod = 30;

    /**
     * 流状态统计周期(秒)
     */
    protected int mStreamStatPeriod = 3;

    /**
     * 是否正在推流
     */
    protected volatile boolean mRecording;


    public SurfaceHolder getSurfaceHolder() {
        return mSurfaceHolder;
    }
    public void setSurfaceHolder(SurfaceHolder surfaceHolder) {
        this.mSurfaceHolder = surfaceHolder;
    }

    public GBMediaRecorder.OnPreviewListener getPreviewCallback() {
        return mOnPreviewListener;
    }
    public void setPreviewCallback(GBMediaRecorder.OnPreviewListener l) {
        this.mOnPreviewListener = l;
    }

    public int getPreviewWidth() {
        return mPreviewWidth;
    }
    public void setPreviewWidth(int width) {
        this.mPreviewWidth = width;
    }

    public int getPreviewHeight() {
        return mPreviewHeight;
    }
    public void setPreviewHeight(int height) {
        this.mPreviewHeight = height;
    }

    public int getPreviewFrameRate() {
        return mPreviewFrameRate;
    }
    public void setPreviewFrameRate(int framerate) {
        this.mPreviewFrameRate = framerate;
    }

    public int getVideoRotate() {
        return mVideoRotate;
    }

    public String getVideoEncodec() {
        return mVideoEncodec;
    }
    public void setVideoEncodec(String encodec) {
        this.mVideoEncodec = encodec;
    }

    public int getVideoBitrate() {
        return mVideoBitrate;
    }
    public void setVideoBitrate(int bitrate) {
        if (bitrate > 0) {
            mVideoBitrate = bitrate;
        }
    }

    public String getVideoBitrateMode() {
        return mVideoBitrateMode;
    }
    public void setVideoBitrateMode(String bitrateMode) {
        mVideoBitrateMode = bitrateMode;
    }

    public int getGop() {
        return mGop;
    }
    public void setGop(int gop) {
        if (gop > 0 && gop <= 10) {
            mGop = gop;
        }
    }

    public int getFramerate() {
        return mFrameRate;
    }
    public void setFramerate(int framerate) {
        if (framerate > 0) {
            mFrameRate = framerate;
        }
    }

    public int getAvQueueSize() {
        return mAvQueueSize;
    }
    public void setAvQueueSize(int size) {
        if (size <= 0 || size > QUEUE_MAX_SIZE) {
            mAvQueueSize = QUEUE_MAX_SIZE;
        } else {
            mAvQueueSize = size;
        }
    }

    public double getVideoBitrateFactor() {
        return mVideoBitrateFactor;
    }
    public void setVideoBitrateFactor(double factor) {
        if (factor >= 0 && factor <= 1) {
            this.mVideoBitrateFactor = factor;
        }
    }

    public int getVideoBitrateAdjustPeriod() {
        return mVideoBitrateAdjustPeriod;
    }
    public void setVideoBitrateAdjustPeriod(int period) {
        if (period >= 0) {
            this.mVideoBitrateAdjustPeriod = period;
        }
    }

    public int getStreamStatPeriod() {
        return mStreamStatPeriod;
    }
    public void setStreamStatPeriod(int period) {
        if (period >= 0) {
            this.mStreamStatPeriod = period;
        }
    }

    public String getVideoProfile() {
        return mVideoProfile;
    }
    public void setVideoProfile(String profile) {
        mVideoProfile = profile;
    }

    public int getThreadNum() {
        return mThreadNum;
    }
    public void setThreadNum(int threadNum) {
        if (threadNum >= -1 && threadNum <= MAX_THREAD_NUM) {
            mThreadNum = threadNum;
        }
    }

    public boolean isRecording() {
        return mRecording;
    }
    public void setRecording(boolean recording) {
        this.mRecording = recording;
    }
}
