package com.nan.gbd.library.source.sceen.impl;

import android.annotation.TargetApi;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * https://blog.csdn.net/gb702250823/article/details/81669684
 * Android原生编解码接口 MediaCodec 之——踩坑
 *
 * @author NanGBD
 * @date
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class HwMediaCodec {

    private static final String TAG = HwMediaCodec.class.getSimpleName();

    public static final String MIME_TYPE_AVC = "video/avc";
    private static final boolean DEBUG = false;
    private static final int TIMEOUT_USEC = 100000;
    private static final int TIMEOUT_USEC_SHORT = 100;

    public Surface mInputSurface = null;
    private MediaCodec mEncoder = null;
    private HandlerThread mEncoderThread = null;
    private Handler mHandler = null;
    private MediaCodec.BufferInfo mBufferInfo = null;
    private volatile boolean mOutputDone = false;
    private volatile boolean mLoopOver = false;
    public MediaCodecCallback mCodecStateCb = null;

    public synchronized void init(String mime, int width, int height, int bitrate, int framerate, int interval) throws IOException {
        MediaFormat format = MediaFormat.createVideoFormat(mime, width, height);
        format.setInteger(MediaFormat.KEY_BIT_RATE, bitrate);
        format.setInteger(MediaFormat.KEY_FRAME_RATE, framerate);
        format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, interval);
        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
        //format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 1920 * 1080);
        format.setInteger(MediaFormat.KEY_PROFILE, MediaCodecInfo.CodecProfileLevel.AVCProfileBaseline);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            format.setInteger(MediaFormat.KEY_LEVEL, MediaCodecInfo.CodecProfileLevel.AVCLevel3);
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            format.setInteger(MediaFormat.KEY_LATENCY, 0);
        }

        mBufferInfo = new MediaCodec.BufferInfo();
        mEncoder = MediaCodec.createEncoderByType(mime);
        mEncoder.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        mInputSurface = mEncoder.createInputSurface();
        mEncoder.start();

        mEncoderThread = new HandlerThread("Encoder-Thread");
        mEncoderThread.start();
        mHandler = new Handler(mEncoderThread.getLooper());
    }

    public MediaCodecCallback getCodecStateCb() {
        return mCodecStateCb;
    }

    public void setCodecStateCb(MediaCodecCallback mCodecStateCb) {
        this.mCodecStateCb = mCodecStateCb;
    }

    public synchronized void start() {
        mLoopOver = false;
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                int index;
                while (!mOutputDone) {
                    // dequeue buffers until not available
                    index = mEncoder.dequeueOutputBuffer(mBufferInfo, TIMEOUT_USEC);
                    if (index == MediaCodec.INFO_TRY_AGAIN_LATER) {
                        //Log.d(TAG, "retrieving buffers time out!");
                    } else if (index == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                        Log.i(TAG, "INFO_OUTPUT_BUFFERS_CHANGED");
                    } else if (index == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                        MediaFormat newFormat = mEncoder.getOutputFormat();
                        Log.i(TAG, "output format changed.\n new format: " + newFormat.toString());

                        if (mCodecStateCb != null) {
                            mCodecStateCb.onFormatChange(newFormat);
                        }
                    } else if (index < 0) {
                        throw new RuntimeException("unexpected result from dequeueOutputBuffer");
                    } else {
                        while (index >= 0) {
                            //Log.e(TAG, "xxxxxxxxxxxxxxxxx");
                            if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
                                Log.d(TAG, "BUFFER_FLAG_CODEC_CONFIG");
                            } else if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                                mOutputDone = true;
                            }

                            if (DEBUG) {
                                Log.d(TAG, "got buffer, index: " + index
                                        + " flags:" + mBufferInfo.flags
                                        + " info: size=" + mBufferInfo.size
                                        + ", ptsUs=" + mBufferInfo.presentationTimeUs
                                        + ", offset=" + mBufferInfo.offset);
                            }

                            boolean doRender = mBufferInfo.size != 0;
                            ByteBuffer encodedData = mEncoder.getOutputBuffer(index);

                            if (doRender && encodedData != null) {
                                // android中MediaCodec硬编码中关键帧间隔时间设置问题
                                // https://blog.csdn.net/aphero/article/details/72279676
//                            if (System.currentTimeMillis() - prevTimeStamp >= 1000) { // 1000毫秒后，设置参数
//                                prevTimeStamp = System.currentTimeMillis();
//                                if (Build.VERSION.SDK_INT >= 23) {
//                                    Bundle params = new Bundle();
//                                    params.putInt(MediaCodec.PARAMETER_KEY_REQUEST_SYNC_FRAME, 0);
//                                    encoder.setParameters(params);
//                                }
//                            }

                                if (mCodecStateCb != null) {
                                    mCodecStateCb.onFrameEncode(mBufferInfo, encodedData);
                                }
                            }
                            mEncoder.releaseOutputBuffer(index, doRender /*false*/);

                            // allow shorter wait for 2nd round to move on quickly.
                            index = mEncoder.dequeueOutputBuffer(mBufferInfo, TIMEOUT_USEC_SHORT /*0L*/);
                        }
                        //Log.e(TAG, "yyyyyyyyyyyyy");
                    }
                }

                mLoopOver = true;
                Log.i(TAG, "end hw");
            }
        });
    }

    public synchronized void stop() {
        mOutputDone = true;
        while (!mLoopOver);

        if (mEncoder != null) {
            mEncoder.signalEndOfInputStream();
            mEncoder.stop();
        }

//        if (mHandler != null) {
//            mHandler.post(new Runnable() {
//                @Override
//                public void run() {
//                    if (mEncoder != null) {
//                        mEncoder.signalEndOfInputStream();
//                        mEncoder.stop();
//                    }
//                }
//            });
//        }
    }

    public synchronized void release() {
        if (mHandler != null) {
            mHandler.removeCallbacksAndMessages(null);
            mHandler = null;
        }

        if (mEncoderThread != null) {
            mEncoderThread.quitSafely();
            mEncoderThread = null;
        }

        if (mEncoder != null) {
            mEncoder.release();
            mEncoder = null;
        }

        mInputSurface = null;
    }

    public interface MediaCodecCallback {
        void onFormatChange(MediaFormat newFormat);
        void onFrameEncode(MediaCodec.BufferInfo bufferInfo, ByteBuffer buffer);
    }
}
