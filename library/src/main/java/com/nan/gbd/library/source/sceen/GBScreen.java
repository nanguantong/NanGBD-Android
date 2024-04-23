package com.nan.gbd.library.source.sceen;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import com.nan.gbd.library.JNIBridge;
import com.nan.gbd.library.aidl.INotification;
import com.nan.gbd.library.aidl.IScreenSharing;
import com.nan.gbd.library.source.GBMediaSource;
import com.nan.gbd.library.source.sceen.impl.HwMediaCodec;
import com.nan.gbd.library.source.sceen.impl.ScreenSharingService;

import java.nio.ByteBuffer;

/**
 * https://docs.agora.io/cn/Video/screensharing_android?platform=Android
 * https://cloud.tencent.com/developer/article/1341499
 * https://www.jianshu.com/p/f2e4db2fb211 Android 录屏时控制帧率
 *
 * @author NanGBD
 * @date 2020.7.1
 */
public class GBScreen extends GBMediaSource implements HwMediaCodec.MediaCodecCallback {
    private static final String TAG = GBScreen.class.getSimpleName();
    private static final boolean DEBUG = false;

    private static volatile GBScreen mInstance;
    private static IScreenSharing mScreenShareSvc;
    private IStateListener mStateListener;
    private byte[] mExtradata;
    private boolean mSentExtra = false;
    private long mVideoPtsOffset, mAudioPtsOffset;
    private long mKeyFrameCnt = 0;

    private GBScreen(Context context) {
        super(context);
    }

    public static GBScreen getInstance(Context context) {
        if (mInstance == null) {
            synchronized (GBScreen.class) {
                if (mInstance == null) {
                    mInstance = new GBScreen(context);
                }
            }
        }
        return mInstance;
    }

    private ServiceConnection mScreenShareConn = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
            mScreenShareSvc = IScreenSharing.Stub.asInterface(service);
            try {
                mScreenShareSvc.registerCallback(mNotification);
                mScreenShareSvc.startShare();
            } catch (RemoteException e) {
                Log.e(TAG, Log.getStackTraceString(e));
            }
        }

        public void onServiceDisconnected(ComponentName className) {
            mScreenShareSvc = null;
        }
    };

    private INotification mNotification = new INotification.Stub() {
        /**
         * This is called by the remote service to tell us about error happened.
         * Note that IPC calls are dispatched through a thread
         * pool running in each process, so the code executing here will
         * NOT be running in our main thread like most other things -- so,
         * if to update the UI, we need to use a Handler to hop over there.
         */
        public void onError(int error) {
            Log.e(TAG, "screen sharing service error happened: " + error);
            mStateListener.onError(error);
        }

        public void onTokenWillExpire() {
            Log.d(TAG, "access token for screen sharing service will expire soon");
            mStateListener.onTokenWillExpire();
        }
    };

    /**
     * 开始预览
     * @return 0:成功, 否则失败
     */
    @Override
    public int startPreview() {
        String appId = "", token = "";

        if (mScreenShareSvc == null) {
            Intent intent = new Intent(mContext, ScreenSharingService.class);
            intent.putExtra(Constant.APP_ID, appId);
            intent.putExtra(Constant.ACCESS_TOKEN, token);
            intent.putExtra(Constant.WIDTH, mPreviewWidth);
            intent.putExtra(Constant.HEIGHT, mPreviewHeight);
            intent.putExtra(Constant.FRAME_RATE, mFrameRate);
            intent.putExtra(Constant.BITRATE, mVideoBitrate);
            intent.putExtra(Constant.GOP, mGop);
            //intent.putExtra(Constant.ORIENTATION_MODE, vec.orientationMode.getValue());
            mContext.bindService(intent, mScreenShareConn, Context.BIND_AUTO_CREATE);
        } else {
            try {
                mScreenShareSvc.startShare();
            } catch (RemoteException e) {
                Log.e(TAG, Log.getStackTraceString(e));
            }
        }

        return 0;
    }

    /**
     * 停止预览
     */
    @Override
    public void stopPreview() {
        if (mScreenShareSvc != null) {
            try {
                mScreenShareSvc.stopShare();
                mScreenShareSvc.unregisterCallback(mNotification);
            } catch (RemoteException e) {
                Log.e(TAG, Log.getStackTraceString(e));
            } finally {
                mScreenShareSvc = null;
            }
        }
        mContext.unbindService(mScreenShareConn);
    }

    public void renewToken(String token) {
        if (mScreenShareSvc != null) {
            try {
                mScreenShareSvc.renewToken(token);
            } catch (RemoteException e) {
                e.printStackTrace();
                Log.e(TAG, Log.getStackTraceString(e));
            }
        } else {
            Log.e(TAG, "screen sharing service not exist");
        }
    }

    public void setListener(IStateListener listener) {
        mStateListener = listener;
    }

    @Override
    public void onFormatChange(MediaFormat newFormat) {
        ByteBuffer sps = newFormat.getByteBuffer("csd-0");
        ByteBuffer pps = newFormat.getByteBuffer("csd-1");
        if (sps == null || pps == null) {
            return;
        }
        //rawSps.position(4);
        //rawPps.position(4);
        int spsLen = sps.remaining();
        int ppsLen = pps.remaining();
        int length = spsLen + ppsLen;
        mExtradata = new byte[length];
        sps.get(mExtradata, 0, spsLen);
        pps.get(mExtradata, spsLen, ppsLen);
        mSentExtra = false;
    }

    @Override
    public void onFrameEncode(MediaCodec.BufferInfo bufferInfo, ByteBuffer buffer) {
        if (bufferInfo == null || buffer == null) {
            return;
        }

        calcSamplingFps();

        boolean keyFrame = (bufferInfo.flags & MediaCodec.BUFFER_FLAG_KEY_FRAME) != 0;
        boolean eos = (bufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0;
        if (bufferInfo.size == 0 && !eos) {
            Log.w(TAG, "info size 0, drop it");
            buffer = null;
        } else {
            if (!mRecording) {
                mVideoPtsOffset = 0;
                mAudioPtsOffset = 0;
                mKeyFrameCnt = 0;
                mSentExtra = false;
                return;
            }
            if (bufferInfo.presentationTimeUs != 0) { // maybe 0 if eos
                resetVideoPts(bufferInfo);
            }
            if (DEBUG) {
                Log.d(TAG, "[" + Thread.currentThread().getId() + "] Got buffer"
                        + ", info: flags=" + bufferInfo.flags
                        + ", size=" + bufferInfo.size
                        + ", pts=" + bufferInfo.presentationTimeUs);
            }
//            if (!eos && mCallback != null) {
//                mCallback.onRecording(bufferInfo.presentationTimeUs);
//            }
        }

        if (buffer != null) {
            buffer.position(bufferInfo.offset);
            buffer.limit(bufferInfo.offset + bufferInfo.size);

            long pts = bufferInfo.presentationTimeUs / 1000;
            int len = buffer.remaining();
            int extraLen = 0;
            if (keyFrame || !mSentExtra) {
                extraLen = mExtradata.length;
                len += extraLen;
                mSentExtra = true;
                if (keyFrame) {
                    mKeyFrameCnt++;
                }
                if (DEBUG) {
                    Log.i(TAG, "put sps pps to iframe");
                }
            }

            byte[] data = new byte[len];
            if (extraLen > 0) {
                System.arraycopy(mExtradata, 0, data, 0, extraLen);
            }
            buffer.get(data, extraLen, buffer.remaining());

            JNIBridge.nanPushVideoFrame(data, pts, keyFrame);
        }
    }

    private void resetVideoPts(MediaCodec.BufferInfo buffer) {
        if (mVideoPtsOffset == 0) {
            mVideoPtsOffset = buffer.presentationTimeUs;
            buffer.presentationTimeUs = 0;
        } else {
            buffer.presentationTimeUs -= mVideoPtsOffset;
        }
    }

    private void resetAudioPts(MediaCodec.BufferInfo buffer) {
        if (mAudioPtsOffset == 0) {
            mAudioPtsOffset = buffer.presentationTimeUs;
            buffer.presentationTimeUs = 0;
        } else {
            buffer.presentationTimeUs -= mAudioPtsOffset;
        }
    }

    private int mVideoFrameCount;
    private long mLastTimeMs;
    private double mSamplingFps;

    /**
     * 计算帧率
     */
    private void calcSamplingFps() {
        if (!DEBUG) {
            return;
        }
        if (mVideoFrameCount == 0) {
            mLastTimeMs = System.currentTimeMillis();
            mVideoFrameCount++;
        } else {
            if (++mVideoFrameCount >= 50) {
                long curTs = System.currentTimeMillis();
                long diffTimeMillis = curTs - mLastTimeMs;
                mSamplingFps = (double) mVideoFrameCount * 1000 / diffTimeMillis;
                mVideoFrameCount = 0;
                Log.d(TAG, "fps:" + mSamplingFps);
            }
        }
    }

    public interface IStateListener {
        void onError(int error);
        void onTokenWillExpire();
    }
}
