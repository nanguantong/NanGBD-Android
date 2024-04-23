package com.nan.gbd.library.media;

/**
 * @author NanGBD
 * @date 2020.6.8
 */
public interface IMediaRecorder {
	/**
	 * 设置是否正在推流
	 * 支持外部调用主动停止推流,不支持外部调用主动开始推流
	 * @param state
	 */
	void setRecording(boolean state);

	/**
	 * 是否正在推流
	 * @return true: 正在推流, 否则没有推流
	 */
	boolean isRecording();

	/**
	 * 音频错误
	 *
	 * @param code 错误类型
	 */
	void onAudioError(int code);

	/**
	 * 视频频错误
	 *
	 * @param code 错误类型
	 * @param extra
	 */
	void onVideoError(int code, int extra);

	/**
	 * 接收音频数据
	 *
	 * @param sampleBuffer   音频采样数据帧
	 * @param sampleRateInHz 如 8000
	 * @param channelConfig  如 AudioFormat.CHANNEL_IN_MONO
	 * @param audioFormat    如 AudioFormat.ENCODING_PCM_16BIT
	 */
	void onReceiveAudioData(byte[] sampleBuffer, int sampleRateInHz, int channelConfig, int audioFormat);

	/**
	 * 接收视频数据
	 */
//	void onReceiveVideoData(byte[] buf);
}
