package com.nan.gbd.library.media;

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.audiofx.AcousticEchoCanceler;
import android.media.audiofx.AutomaticGainControl;
import android.os.Build;
import android.util.Log;

/**
 * 音频播放
 * https://blog.csdn.net/lancees/article/details/7671343
 * https://cstriker1407.info/blog/android-acousticechocanceler-notes/
 * https://blog.csdn.net/WificamSDK7/article/details/72917123
 *
 * 回音分为两种:
 *   声学回音（Acoustic Echo)
 *   线路回音（Line Echo)
 * 目前我们只讨论声学回音；声学回音是由于在免提或者会议应用中，扬声器的声音多次反馈到麦克风引起的；
 *
 * 回音消除原理
 * 在发送时，把不需要的回音从语音流中间去掉；
 * 对于一个混合了两个声音的语音流，要把他们分开，去掉其中一个，这个非常困难；
 * 所以，实际应用上，除了这个已经混合好的信号，我们是可以得到产生回音的原始信号，
 * 然后基于回音的原始信号，在混合信号中将回音信号剥离出来。
 *
 * @author NanGBD
 * @date 2021.7.8
 */
public class AudioPlayer {
    private static final String TAG = AudioPlayer.class.getSimpleName();

    private int mSampleRate = GBMediaRecorder.AUDIO_SAMPLE_RATE;
    private int mChannelConfig = AudioFormat.CHANNEL_OUT_MONO;
    private int mAudioFormat = AudioFormat.ENCODING_PCM_16BIT;

    private AudioTrack mAudioTrack;
    private static AcousticEchoCanceler mAec;
    private static AutomaticGainControl mAgc;
    private IMediaRecorder mMediaRecorder;

    public AudioPlayer(IMediaRecorder mediaRecorder) {
        this.mMediaRecorder = mediaRecorder;
    }

    public boolean init() {
        // 一帧的大小,minBufSize必须大于等于一帧
        final int minBufSize = getFrameLen();
        Log.d(TAG, "sample rate:" + mSampleRate + ", frame size:" + minBufSize);

        if (AudioTrack.ERROR_BAD_VALUE == minBufSize) {
            mMediaRecorder.onAudioError(GBMediaRecorder.AUDIO_RECORD_ERROR_GET_FRAME_SIZE_NOT_SUPPORT);
            return false;
        }

        // STREAM_ALARM：警告声
        // STREAM_MUSCI：音乐声，例如music等
        // STREAM_RING：铃声
        // STREAM_SYSTEM：系统声音
        // STREAM_VOCIE_CALL：电话声音
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            mAudioTrack = new AudioTrack.Builder()
                    .setAudioAttributes(new AudioAttributes.Builder()
                            .setUsage(AudioAttributes.USAGE_VOICE_COMMUNICATION)
                            .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
                            .setLegacyStreamType(AudioManager.MODE_IN_COMMUNICATION)
                            .build())
                    .setAudioFormat(new AudioFormat.Builder()
                            .setEncoding(mAudioFormat)
                            .setSampleRate(mSampleRate)
                            .setChannelMask(mChannelConfig)
                            .build())
                    .setTransferMode(AudioTrack.MODE_STREAM)
                    .setBufferSizeInBytes(minBufSize * 2)
                    .build();
        } else {
            mAudioTrack = new AudioTrack(AudioManager.MODE_IN_COMMUNICATION,
                    mSampleRate, mChannelConfig, mAudioFormat,
                    minBufSize * 2, AudioTrack.MODE_STREAM);
        }
        if (mAudioTrack.getState() != AudioTrack.STATE_INITIALIZED) {
            mMediaRecorder.onAudioError(GBMediaRecorder.AUDIO_PLAYER_ERROR_CREATE_FAILED);
            return false;
        }

        try {
            /**
             * E/AudioEffect: set(): AudioFlinger could not create effect , status: -22
             * E/AudioEffects-JNI: AudioEffect initCheck failed -3
             * E/AudioEffect-JAVA: Error code -3 when initializing AudioEffect.
             */
//            if (AcousticEchoCanceler.isAvailable()) {
//                mAec = AcousticEchoCanceler.create(mAudioTrack.getAudioSessionId());
//                if (mAec != null) {
//                    mAec.setEnabled(true);
//                }
//            }
//            if (AutomaticGainControl.isAvailable()) {
//                mAgc = AutomaticGainControl.create(mAudioTrack.getAudioSessionId());
//                if (mAgc != null) {
//                    mAgc.setEnabled(true);
//                }
//            }

            //mAudioTrack.setVolume(1.0f);
            // 2. 开始播放
            mAudioTrack.play();
        } catch (Exception e) {
            Log.i(TAG, "audio play error:" + e.getMessage());
            mMediaRecorder.onAudioError(GBMediaRecorder.AUDIO_ERROR_UNKNOWN);
        }
        return true;
    }

    public boolean write(byte[] data) {
        if (null == mAudioTrack || mAudioTrack.getPlayState() != AudioTrack.PLAYSTATE_PLAYING) {
            return false;
        }
        // Speex.playback(data)
        int ret = mAudioTrack.write(data, 0, data.length);
        if (ret < 0) {
            Log.e(TAG, "audio play failed:" + ret);
            return false;
        }
        return true;
    }

    public void stop() {
        try {
            if (mAudioTrack != null) {
                mAudioTrack.stop();
                mAudioTrack.release();
                mAudioTrack = null;
            }

            if (mAec != null) {
                mAec.setEnabled(false);
                mAec.release();
                mAec = null;
            }

            if (mAgc != null) {
                mAgc.setEnabled(false);
                mAgc.release();
                mAgc = null;
            }
        } catch (Exception e) {
            Log.e(TAG, "stop audio: " + e.getMessage());
        }
    }

    /**
     * 计算bufferSizeInBytes：int size = 采样率 x 位宽 x 通道数
     * @return
     */
    public int getFrameLen() {
        return AudioTrack.getMinBufferSize(mSampleRate, mChannelConfig, mAudioFormat);
    }
}