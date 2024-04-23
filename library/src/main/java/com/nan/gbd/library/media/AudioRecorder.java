package com.nan.gbd.library.media;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.media.audiofx.AcousticEchoCanceler;
import android.media.audiofx.AutomaticGainControl;
import android.os.Process;
import android.util.Log;

/**
 * 音频录制
 *
 * @author NanGBD
 * @date 2020.6.8
 */
public class AudioRecorder extends Thread {
	private static final String TAG = AudioRecorder.class.getSimpleName();

	private int mSampleRate = GBMediaRecorder.AUDIO_SAMPLE_RATE;
	private int mChannelConfig = AudioFormat.CHANNEL_IN_MONO;
	private int mAudioFormat = AudioFormat.ENCODING_PCM_16BIT;

	private static AudioRecord mMic;
	private static AcousticEchoCanceler mAec;
	private static AutomaticGainControl mAgc;
	private IMediaRecorder mMediaRecorder;

	public AudioRecorder(IMediaRecorder mediaRecorder) {
		this.mMediaRecorder = mediaRecorder;
	}

	@Override
	public void run() {
		Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO);

		if (mSampleRate != 8000 && mSampleRate != 16000 && mSampleRate != 22050 && mSampleRate != 44100) {
			mMediaRecorder.onAudioError(GBMediaRecorder.AUDIO_RECORD_ERROR_SAMPLERATE_NOT_SUPPORT);
			return;
		}

		// 一帧的大小,minBufSize必须大于等于一帧
		final int minBufSize = getFrameLen();
		Log.d(TAG, "sample rate:" + mSampleRate + ", frame size:" + minBufSize);

		if (AudioRecord.ERROR_BAD_VALUE == minBufSize) {
			mMediaRecorder.onAudioError(GBMediaRecorder.AUDIO_RECORD_ERROR_GET_FRAME_SIZE_NOT_SUPPORT);
			return;
		}

		/**
		 * 1.配置参数,初始化AudioRecord
		 * 	audioSource:    音频采集的输入源. DEFAULT（默认）,VOICE_RECOGNITION（用于语音识别,等同于DEFAULT）,MIC（由手机麦克风输入）,VOICE_COMMUNICATION（用于VoIP应用）等
		 * 	sampleRateInHz: 采样率. 注意:目前44.1kHz是唯一可以保证兼容所有Android手机的采样率
		 * 	channelConfig:  通道数的配置. CHANNEL_IN_MONO（单通道）/CHANNEL_IN_STEREO（双通道）
		 * 	audioFormat:    配置数据位宽. ENCODING_PCM_16BIT（16bit）/ENCODING_PCM_8BIT（8bit）
		 * 	bufferSizeInBytes: 配置 AudioRecord 内部的音频缓冲区的大小, 该缓冲区的值不能低于一帧“音频帧”的大小
		 */
		mMic = new AudioRecord(MediaRecorder.AudioSource.VOICE_COMMUNICATION, mSampleRate,
				mChannelConfig, mAudioFormat, minBufSize);
		if (mMic.getState() != AudioRecord.STATE_INITIALIZED) {
			mMediaRecorder.onAudioError(GBMediaRecorder.AUDIO_RECORD_ERROR_CREATE_FAILED);
			return;
		}

		byte[] sampleBuffer = new byte[minBufSize];

		try {
			if (AcousticEchoCanceler.isAvailable()) {
				mAec = AcousticEchoCanceler.create(mMic.getAudioSessionId());
				if (mAec != null) {
					mAec.setEnabled(true);
				}
			}
			if (AutomaticGainControl.isAvailable()) {
				mAgc = AutomaticGainControl.create(mMic.getAudioSessionId());
				if (mAgc != null) {
					mAgc.setEnabled(true);
				}
			}

			// 2. 开始采集
			mMic.startRecording();

			while (!Thread.currentThread().isInterrupted()) {
				int size = mMic.read(sampleBuffer, 0, sampleBuffer.length);
				if (size > 0) {
					// Speex.capture(sampleBuffer)
					mMediaRecorder.onReceiveAudioData(sampleBuffer, mSampleRate, mChannelConfig, mAudioFormat);
				} else {
					Log.e(TAG, "audio read error:" + size);
				}
			}
		} catch (Exception e) {
			Log.e(TAG, "audio record error:" + e.getMessage());
			mMediaRecorder.onAudioError(GBMediaRecorder.AUDIO_ERROR_UNKNOWN);
		}
		stopAudio();
	}

	private void stopAudio() {
		try {
			if (mMic != null) {
				mMic.setRecordPositionUpdateListener(null);
				mMic.stop();
				mMic.release();
				mMic = null;
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
		return AudioRecord.getMinBufferSize(mSampleRate, mChannelConfig, mAudioFormat);
	}
}
