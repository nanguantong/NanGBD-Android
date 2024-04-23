package com.nan.gbd.library.media;

import android.media.MediaRecorder;
import android.util.Log;

import com.nan.gbd.library.JNIBridge;
import com.nan.gbd.library.bus.GBEventCallback;
import com.nan.gbd.library.source.GBMediaSource;
import com.nan.gbd.library.source.MediaSourceType;
import com.nan.gbd.library.utils.BusUtils;

import java.util.concurrent.ConcurrentHashMap;

import static com.nan.gbd.library.sip.GBEvent.*;

/**
 * @author NanGBD
 * @date 2020.6.8
 */
public class MediaRecorderNative extends GBMediaRecorder implements MediaRecorder.OnErrorListener {

    private static final String TAG = MediaRecorder.class.getSimpleName();

    @Override
    public void onError(MediaRecorder mr, int code, int extra) {
        try {
            if (mr != null)
                mr.reset();
        } catch (Exception e) {
            Log.w(TAG, "onError", e);
        }
        onVideoError(code, extra);
    }

    /**
     * 开始推流
     * @return true:成功
     */
    protected synchronized boolean startMux() {
        Log.i(TAG, "startMuxer pid:" + Thread.currentThread().getId());

        if (mRecording) {
            return true;
        }
        mRecording = true;

        if (!mAudioStartup && mEnableAudio) {
            if (mAudioRecorder == null) {
                mAudioRecorder = new AudioRecorder(this);
            }
        }

        int vencodec = GBMediaSource.VIDEO_ENCODEC_NONE;
        if (mSourceType == MediaSourceType.MS_TYPE_CAMERA) {
            if ("h264".equals(mMediaSource.getVideoEncodec())) {
                vencodec = GBMediaSource.VIDEO_ENCODEC_H264;
            } else if ("h265".equals(mMediaSource.getVideoEncodec())) {
                vencodec = GBMediaSource.VIDEO_ENCODEC_H265;
            }
        }

        int profile = 1;
        if ("baseline".equals(mMediaSource.getVideoProfile())) {
            profile = 1;
        } else if ("main".equals(mMediaSource.getVideoProfile())) {
            profile = 2;
        } else if ("high".equals(mMediaSource.getVideoProfile())) {
            profile = 3;
        }

        int bitrateMode = GBMediaSource.BITRATE_MODE_CQ;
        if ("cq".equals(mMediaSource.getVideoBitrateMode())) {
            bitrateMode = GBMediaSource.BITRATE_MODE_CQ;
        } else if ("vbr".equals(mMediaSource.getVideoBitrateMode())) {
            bitrateMode = GBMediaSource.BITRATE_MODE_VBR;
        } else if ("cbr".equals(mMediaSource.getVideoBitrateMode())) {
            bitrateMode = GBMediaSource.BITRATE_MODE_CBR;
        }

        int ret = JNIBridge.nanInitMuxer(
                mMediaOutput == null ? "" : mMediaOutput.getOutputDir(),
                mMediaOutput == null ? "" : mMediaOutput.getOutputName(),
                mMediaOutput == null ? 0 : mMediaOutput.getDuration(),
                mEnableVideo,
                mEnableAudio && mAudioRecorder != null,
                vencodec,
                mMediaSource.getVideoRotate(),
                mMediaSource.getPreviewWidth(),
                mMediaSource.getPreviewHeight(),
                mMediaSource.getPreviewWidth(),
                mMediaSource.getPreviewHeight(),
                mMediaSource.getFramerate(),
                mMediaSource.getGop(),
                mMediaSource.getVideoBitrate(),
                bitrateMode,
                profile,
                mAudioRecorder == null ? 0 : mAudioRecorder.getFrameLen(),
                mMediaSource.getThreadNum(),
                mMediaSource.getAvQueueSize(),
                mMediaSource.getVideoBitrateFactor(),
                mMediaSource.getVideoBitrateAdjustPeriod(),
                mMediaSource.getStreamStatPeriod()
        );

        if (ret != 0) {
            Log.e(TAG, "initMuxer error:" + ret);
            mRecording = false;
            return false;
        }
        mMediaSource.setRecording(true);
        try {
            if (mEnableAudio && mAudioRecorder != null && !mAudioRecorder.isAlive()) {
                mAudioRecorder.start();
            }
        } catch (Exception e) {
            Log.e(TAG, "initMuxer start audio stream error:" + e.getMessage());
        }

        return true;
    }

    /**
     * 停止推流
     * @return true:成功
     */
    protected synchronized boolean endMux() {
        Log.i(TAG, "endMuxer pid:" + Thread.currentThread().getId());

        if (!mRecording) {
            return true;
        }
        mRecording = false;
        mMediaSource.setRecording(false);
        if (!mAudioStartup && mAudioRecorder != null) {
            mAudioRecorder.interrupt();
            mAudioRecorder = null;
        }
        return JNIBridge.nanEndMuxer() == 0;
    }

    protected synchronized boolean startVoice(int cid) {
        Log.i(TAG, "startVoice " + cid + " pid:" + Thread.currentThread().getId());

        if (null == mAudioPlayers) {
            mAudioPlayers = new ConcurrentHashMap<>();
        }
        if (mAudioPlayers.containsKey(cid)) {
            Log.e(TAG, "exist voice cid " + cid);
            return false;
        }

        AudioPlayer audioPlayer = new AudioPlayer(this);
        mAudioPlayers.put(cid, audioPlayer);
        return audioPlayer.init();
    }

    protected synchronized boolean endVoice(int cid) {
        Log.i(TAG, "endVoice " + cid + " pid:" + Thread.currentThread().getId());

        if (null == mAudioPlayers) {
            return false;
        }
        AudioPlayer audioPlayer = mAudioPlayers.get(cid);
        if (null == audioPlayer) {
            Log.e(TAG, "not exist voice cid " + cid);
            return false;
        }

        audioPlayer.stop();
        mAudioPlayers.remove(cid);
        return true;
    }

    protected boolean onVoiceData(int cid, byte[] data) {
        if (null == mAudioPlayers || null == data || data.length == 0) {
            return false;
        }
        AudioPlayer audioPlayer = mAudioPlayers.get(cid);
        if (null == audioPlayer) {
            Log.e(TAG, "not exist voice cid " + cid);
            return false;
        }
        return audioPlayer.write(data);
    }

    /**
     * 设置是否正在推流
     * 支持外部调用主动停止推流,不支持外部调用主动开始推流
     * @param state
     */
    public synchronized void setRecording(boolean state) {
        if (mRecording || state) {
            return;
        }
        endMux();
        this.mRecording = state;
    }

    /**
     * 是否正在推流
     * @return true: 正在推流, 否则没有推流
     */
    public synchronized boolean isRecording() {
        return mRecording;
    }

    @Override
    public boolean onCallback(int code, String name, int key, byte[] param) {
        BusUtils.BUS.post(new GBEventCallback(code, name, param));
        switch (code) {
            case GB_DEVICE_EVENT_CONNECTING:
                break;
            case GB_DEVICE_EVENT_REGISTERING:
                break;
            case GB_DEVICE_EVENT_REGISTER_OK:
                break;
            case GB_DEVICE_EVENT_REGISTER_AUTH_FAIL:
                break;
            case GB_DEVICE_EVENT_UNREGISTER_OK:
                break;
            case GB_DEVICE_EVENT_UNREGISTER_FAILED:
                break;
            case GB_DEVICE_EVENT_START_VIDEO:
                return startMux();
            case GB_DEVICE_EVENT_STOP_VIDEO:
                return endMux();
            case GB_DEVICE_EVENT_START_VOICE:
                return startVoice(key);
            case GB_DEVICE_EVENT_STOP_VOICE:
                return endVoice(key);
            case GB_DEVICE_EVENT_TALK_AUDIO_DATA:
                return onVoiceData(key, param);
            case GB_DEVICE_EVENT_START_RECORD:
                break;
            case GB_DEVICE_EVENT_STOP_RECORD:
                break;
            case GB_DEVICE_EVENT_STATISTICS:
                break;
            case GB_DEVICE_EVENT_DISCONNECT:
                break;
            default:
                break;
        }
        return true;
    }
}
