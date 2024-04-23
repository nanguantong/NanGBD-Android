package com.nan.gbd.library.media;

import android.content.Context;
import android.os.Environment;
import android.view.SurfaceHolder;

import com.nan.gbd.library.JNIBridge;
import com.nan.gbd.library.StatusCallback;
import com.nan.gbd.library.source.GBMediaSource;
import com.nan.gbd.library.source.MediaSourceType;
import com.nan.gbd.library.source.camera.GBCamera;
import com.nan.gbd.library.source.sceen.GBScreen;
import com.nan.gbd.library.utils.DeviceUtils;

import java.io.File;
import java.util.Map;

/**
 * 视频录制抽象类
 *
 * @author NanGBD
 * @date 2020.6.8
 */
public abstract class GBMediaRecorder implements StatusCallback,
        SurfaceHolder.Callback, IMediaRecorder {

    private static final String TAG = GBMediaRecorder.class.getSimpleName();

    public static final String FILE_DIR_DEFAULT = "/ps/";

    /////////////////////////////////////////////////////////////
    /**
     * 视频码率 1M
     */
    public static final int VIDEO_BITRATE_NORMAL = 1024;
    /**
     * 视频码率 1.5M（默认）
     */
    public static final int VIDEO_BITRATE_MEDIUM = 1536;
    /**
     * 视频码率 2M
     */
    public static final int VIDEO_BITRATE_HIGH = 2048;
    /////////////////////////////////////////////////////////////

    /**
     * 音频采样率
     */
    public static final int AUDIO_SAMPLE_RATE = 8000;

    /**
     * 视频输入源
     */
    protected GBMediaSource mMediaSource;
    protected MediaSourceType mSourceType = MediaSourceType.MS_TYPE_CAMERA;

    /**
     * 声音录制
     */
    protected AudioRecorder mAudioRecorder;

    /**
     * 声音播放
     */
    protected Map<Integer, AudioPlayer> mAudioPlayers;

    /**
     * 预览回调监听
     */
    protected OnPreviewListener mOnPreviewListener;
    /**
     * 录制错误监听
     */
    protected OnErrorListener mOnErrorListener;
    /**
     * 录制已经准备就绪的监听
     */
    protected OnPreparedListener mOnPreparedListener;

    /**
     * 是否开启音视频推流
     */
    protected boolean mEnableVideo = true, mEnableAudio = false;

    /**
     * 是否音频采集长开保持
     */
    protected boolean mAudioStartup = false;

    /**
     * 状态标记
     */
    protected boolean mPrepared, mStartPreview, mSurfaceCreated;

    /**
     * 是否正在录制
     */
    protected volatile boolean mRecording;

    /**
     * 媒体输出信息
     */
    protected MediaOutput mMediaOutput;

    /**
     * 预处理监听
     */
    public interface OnPreparedListener {
        void onSurfaceCreated(SurfaceHolder holder);

        void onSurfaceChanged(SurfaceHolder holder, int format, int width, int height);

        void onSurfaceDestroyed(SurfaceHolder holder);

        /**
         * 预处理完毕，可以开始录制
         * @param previewWidth  预览宽度
         * @param previewHeight 预览高度
         * @param previewFps    预览帧率
         */
        void onPrepared(int previewWidth, int previewHeight, int previewFps);
    }

    /**
     * 预览回调接口
     */
    public interface OnPreviewListener {
        /**
         * 视频源预览帧回调触发此回调
         * 注意: 不能在此函数中处理耗时任务
         *
         * @param data MS_TYPE_CAMERA --> 输出nv21数据
         * @param width  视频宽度
         * @param height 视频高度
         */
        void onPreviewFrame(byte[] data, int width, int height);

        /**
         * 音频源预览帧回调触发此回调
         * 注意: 不能在此函数中处理耗时任务
         *
         * @param data 音频采样数据帧
         * @param sampleRateInHz 如 8000
         * @param channelConfig  如 AudioFormat.CHANNEL_IN_MONO
         * @param audioFormat    如 AudioFormat.ENCODING_PCM_16BIT
         */
        void onPreviewSample(byte[] data, int sampleRateInHz, int channelConfig, int audioFormat);
    }

    /**
     * 错误监听
     */
    public interface OnErrorListener {
        /**
         * 视频错误回调
         * @param code    错误码
         * @param extra   额外值
         * @param message 错误消息
         */
        void onVideoError(int code, int extra, String message);

        /**
         * 音频错误回调
         * @param code    错误码
         * @param message 错误消息
         */
        void onAudioError(int code, String message);
    }

    public GBMediaRecorder() {
        JNIBridge.setStatusCallback(this);
    }

    /**
     * 设置视频采集源
     * @param context
     * @param sourceType MS_TYPE_CAMERA / MS_TYPE_SCREEN
     */
    public void setMediaSource(Context context, MediaSourceType sourceType) {
        if (mMediaSource != null && (mSourceType == sourceType)) {
            return;
        }
        stopPreview();
        mPrepared = false;

        mSourceType = sourceType;
        switch (sourceType) {
            case MS_TYPE_CAMERA:
                mMediaSource = GBCamera.getInstance(context);
                break;
            case MS_TYPE_SCREEN:
                mMediaSource = GBScreen.getInstance(context);
                break;
        }
        if (null != mMediaSource) {
            mMediaSource.setPreviewCallback(mOnPreviewListener);
        }
    }

    /**
     * 设置视频预览帧率
     * @param framerate 设置设备支持的帧率
     */
    public void setPreviewFramerate(int framerate) {
        if (mMediaSource != null) {
            mMediaSource.setPreviewFrameRate(framerate);
        }
    }

    /**
     * 设置界面预览输出
     * @param holder
     */
    @SuppressWarnings("deprecation")
    public void setSurfaceHolder(SurfaceHolder holder) {
        if (holder != null) {
            holder.addCallback(this);
            if (!DeviceUtils.hasHoneycomb()) {
                holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
            }

            //if (mMediaSource != null) {
            //    mMediaSource.setSurfaceHolder(holder);
            //}
        }
    }

    /**
     * 设置视频预览监听
     * @param l 监听者
     */
    public void setOnPreviewListener(OnPreviewListener l) {
        mOnPreviewListener = l;
        if (null != mMediaSource) {
            mMediaSource.setPreviewCallback(l);
        }
    }

    /**
     * 设置预处理监听
     * @param l 监听者
     */
    public void setOnPreparedListener(OnPreparedListener l) {
        mOnPreparedListener = l;
    }

    /**
     * 设置错误监听
     * @param l 监听者
     */
    public void setOnErrorListener(OnErrorListener l) {
        mOnErrorListener = l;
    }

    /**
     * 开始预览
     */
    public void prepare() {
        mPrepared = true;
        startPreview();
    }

    /**
     * 设置视频编码器
     * @param vencodec 默认h264, h264 / h265
     */
    public void setVideoEncodec(String vencodec) {
        if (mMediaSource != null) {
            mMediaSource.setVideoEncodec(vencodec);
        }
    }

    /**
     * 设置视频编码码率
     * @param bitrate 默认300000, 码率kbps
     */
    public void setVideoBitrate(int bitrate) {
        if (mMediaSource != null) {
            mMediaSource.setVideoBitrate(bitrate);
        }
    }

    /**
     * 设置视频编码码率控制模式
     * @param bitrateMode 默认cq, cq / vbr / cbr
     */
    public void setVideoBitrateMode(String bitrateMode) {
        if (mMediaSource != null) {
            mMediaSource.setVideoBitrateMode(bitrateMode);
        }
    }

    /**
     * 设置视频编码帧率
     * @param framerate 默认25, 设置设备支持的帧率
     */
    public void setFramerate(int framerate) {
        if (mMediaSource != null) {
            mMediaSource.setFramerate(framerate);
        }
    }

    /**
     * 设置视频编码关键帧间隔(秒)
     * @param gop 默认5, 时长[1, 10]
     */
    public void setGop(int gop) {
        if (mMediaSource != null) {
            mMediaSource.setGop(gop);
        }
    }

    /**
     * 设置视频编码profile
     * @param profile 默认baseline, baseline / main / high
     */
    public void setVideoProfile(String profile) {
        if (mMediaSource != null) {
            mMediaSource.setVideoProfile(profile);
        }
    }

    /**
     * 设置编码线程数
     * @param threadNum 默认-1, 为-1时表示硬编码，
     *                  为0时表示软编码自适应线程，>0时表示软编码线程数，最大取值 MAX_THREAD_NUM
     */
    public void setThreadNum(int threadNum) {
        if (mMediaSource != null) {
            mMediaSource.setThreadNum(threadNum);
        }
    }

    /**
     * 设置编码最大缓存队列大小
     * @param size 默认100
     */
    public void setAvQueueSize(int size) {
        if (mMediaSource != null) {
            mMediaSource.setAvQueueSize(size);
        }
    }

    /**
     * 设置视频比特率上下浮动因子
     * @param factor 默认0.3, [0, 1]之间
     */
    public void setVideoBitrateFactor(double factor) {
        if (mMediaSource != null) {
            mMediaSource.setVideoBitrateFactor(factor);
        }
    }

    /**
     * 设置视频比特率动态调整周期(秒)
     * 内部会自动计算当前网络发送拥塞程度并根据其进行上下码率调整
     * @param period 默认30, 为0表示不自动调整码率
     */
    public void setVideoBitrateAdjustPeriod(int period) {
        if (mMediaSource != null) {
            mMediaSource.setVideoBitrateAdjustPeriod(period);
        }
    }

    /**
     * 设置流状态统计周期(秒)
     * @param period 默认3
     */
    public void setStreamStatPeriod(int period) {
        if (mMediaSource != null) {
            mMediaSource.setStreamStatPeriod(period);
        }
    }

    /**
     * 设置是否使能音视频采集发送
     * @param enableVideo 默认true
     * @param enableAudio 默认false
     */
    public void setEnableAV(boolean enableVideo, boolean enableAudio) {
        mEnableVideo = enableVideo;
        mEnableAudio = enableAudio;
    }

    /**
     * 设置是否音频采集长开保持
     * @param audioStartup 默认false, true:开启, false:关闭
     */
    public void setAudioStartup(boolean audioStartup) {
        this.mAudioStartup = audioStartup;
        if (mAudioStartup) {
            if (mAudioRecorder == null) {
                mAudioRecorder = new AudioRecorder(this);
                mAudioRecorder.start();
            }
        } else {
            if (!mRecording) {
                if (mAudioRecorder != null) {
                    mAudioRecorder.interrupt();
                    mAudioRecorder = null;
                }
            }
        }
    }

    /**
     * 更新视频预览分辨率，会重置预览
     * @param w 视频宽度
     * @param h 视频高度
     */
    public void updateResolution(int w, int h) {
        stopPreview();
        if (mMediaSource != null) {
            mMediaSource.setPreviewWidth(w);
            mMediaSource.setPreviewHeight(h);
        }
        startPreview();
    }

    /**
     * 开始预览
     */
    protected void startPreview() {
        if (!mSurfaceCreated) {
            return;
        }
        if (mStartPreview || !mPrepared) {
            return;
        }

        if (mMediaSource == null || mMediaSource.getSurfaceHolder() == null) {
            return;
        }

        int ret = mMediaSource.startPreview();
        if (ret == 0) {
            mStartPreview = true;
            if (mOnPreparedListener != null) {
                mOnPreparedListener.onPrepared(mMediaSource.getPreviewWidth(),
                        mMediaSource.getPreviewHeight(), mMediaSource.getPreviewFrameRate());
            }
        } else {
            onVideoError(ret, 0);
        }
    }

    /**
     * 停止预览
     */
    protected void stopPreview() {
        if (mMediaSource != null) {
            mMediaSource.stopPreview();
        }
        mStartPreview = false;
    }

    /**
     * 释放资源
     */
    public void release() {
        setRecording(false);
        JNIBridge.nanEndMuxer();
        stopPreview();

        if (mAudioRecorder != null) {
            mAudioRecorder.interrupt();
            mAudioRecorder = null;
        }
        mPrepared = false;
        mSurfaceCreated = false;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (mMediaSource != null) {
            mMediaSource.setSurfaceHolder(holder);
        }
        this.mSurfaceCreated = true;
        if (mPrepared && !mStartPreview) {
            startPreview();
        }

        if (mOnPreparedListener != null) {
            mOnPreparedListener.onSurfaceCreated(holder);
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (mMediaSource != null) {
            mMediaSource.setSurfaceHolder(holder);
        }

        if (mOnPreparedListener != null) {
            mOnPreparedListener.onSurfaceChanged(holder, format, width, height);
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        stopPreview();
        mSurfaceCreated = false;

        if (mOnPreparedListener != null) {
            mOnPreparedListener.onSurfaceDestroyed(holder);
        }
    }

    @Override
    public void onAudioError(int code) {
        if (mOnErrorListener != null) {
            String message = getAudioMessage(code);
            mOnErrorListener.onAudioError(code, message);
        }
    }

    @Override
    public void onVideoError(int code, int extra) {
        if (mOnErrorListener != null) {
            String message = getVideoMessage(code);
            mOnErrorListener.onVideoError(code, extra, message);
        }
    }

    /**
     * 接收音频数据，传递到底层
     */
    @Override
    public void onReceiveAudioData(byte[] sampleBuffer, int sampleRateInHz, int channelConfig, int audioFormat) {
        if (mRecording) {
            long curTs = System.currentTimeMillis();
            JNIBridge.nanPushAudioFrame(sampleBuffer, curTs);
        }

        if (null != mOnPreviewListener) {
            mOnPreviewListener.onPreviewSample(sampleBuffer, sampleRateInHz, channelConfig, audioFormat);
        }
    }


    public MediaOutput setTcpOutput(String serverIp, int serverPort, int localPort, int ssrc) {
        mMediaOutput = new MediaOutput(serverIp, serverPort, localPort, "", "", JNIBridge.TCP, ssrc);
        return mMediaOutput;
    }

    public MediaOutput setUdpOutput(String serverIp, int serverPort, int ssrc) {
        mMediaOutput = new MediaOutput(serverIp, serverPort, "", "", JNIBridge.UDP, ssrc);
        return mMediaOutput;
    }

    public MediaOutput setFileOutput(String fileName, int durationSec) {
        File dcim = Environment.getExternalStorageDirectory();
        String path;
        if (DeviceUtils.isZte()) {
            if (dcim.exists()) {
                path = dcim + FILE_DIR_DEFAULT;
            } else {
                path = dcim.getPath().replace("/sdcard/", "/sdcard-ext/") + FILE_DIR_DEFAULT;
            }
        } else {
            path = dcim + FILE_DIR_DEFAULT;
        }
        File file = new File(path);
        if (!file.exists()) {
            file.mkdirs();
        }
        mMediaOutput = new MediaOutput("", 0, path, fileName, JNIBridge.FILE, 0);
        mMediaOutput.setDuration(durationSec);
        return mMediaOutput;
    }

    public static String getLogOutPutPath() {
        File path = new File(Environment.getExternalStorageDirectory() + FILE_DIR_DEFAULT);
        if (!path.exists() && !path.mkdirs()) {
            return "";
        }
        return path + "/nangbd.log";
    }


    /////////////////////////////////////////////////////////////
    public static final int AUDIO_ERROR_UNKNOWN = 0;
    /**
     * 采样率设置不支持
     */
    public static final int AUDIO_RECORD_ERROR_SAMPLERATE_NOT_SUPPORT = 1;
    /**
     * 最小缓存获取失败
     */
    public static final int AUDIO_RECORD_ERROR_GET_FRAME_SIZE_NOT_SUPPORT = 2;
    /**
     * 创建AudioRecord失败
     */
    public static final int AUDIO_RECORD_ERROR_CREATE_FAILED = 3;
    /**
     * 创建AudioPlayer失败
     */
    public static final int AUDIO_PLAYER_ERROR_CREATE_FAILED = 10;
    /////////////////////////////////////////////////////////////

    protected static String getAudioMessage(int code) {
        String res;
        switch (code) {
            case AUDIO_ERROR_UNKNOWN:
                res = "start audio record failed";
                break;
            case AUDIO_RECORD_ERROR_SAMPLERATE_NOT_SUPPORT:
                res = "sample rate not support";
                break;
            case AUDIO_RECORD_ERROR_GET_FRAME_SIZE_NOT_SUPPORT:
                res = "parameters are not supported by the hardware";
                break;
            case AUDIO_RECORD_ERROR_CREATE_FAILED:
                res = "new audio record failed";
                break;
            default:
                res = "audio record error";
                break;
        }
        return res;
    }

    /////////////////////////////////////////////////////////////
    /**
     * 未知错误
     */
    public static final int MEDIA_ERROR_UNKNOWN = 0;
    /**
     * 预览画布设置错误
     */
    public static final int MEDIA_ERROR_CAMERA_SET_PREVIEW_DISPLAY = 101;
    /**
     * 预览错误
     */
    public static final int MEDIA_ERROR_CAMERA_PREVIEW = 102;
    /**
     * 自动对焦错误
     */
    public static final int MEDIA_ERROR_CAMERA_AUTO_FOCUS = 103;
    /////////////////////////////////////////////////////////////

    protected static String getVideoMessage(int code) {
        String res;
        switch (code) {
            case MEDIA_ERROR_UNKNOWN:
                res = "媒体错误";
                break;
            case MEDIA_ERROR_CAMERA_SET_PREVIEW_DISPLAY:
                res = "设置摄像头预览显示失败";
                break;
            case MEDIA_ERROR_CAMERA_PREVIEW:
                res = "摄像头预览失败";
                break;
            case MEDIA_ERROR_CAMERA_AUTO_FOCUS:
                res = "摄像头自动对焦失败";
                break;
            default:
                res = "开始摄像头失败";
                break;
        }
        return res;
    }
}
