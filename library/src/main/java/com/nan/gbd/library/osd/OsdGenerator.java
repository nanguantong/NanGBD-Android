package com.nan.gbd.library.osd;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.util.Log;

import com.nan.gbd.library.JNIBridge;
import com.nan.gbd.library.utils.StringUtils;

import java.util.LinkedHashMap;
import java.util.Map;

/**
 * https://github.com/Zihao-Wu/YuvWater
 * @author NanGBD
 * @date 2020.6.20
 */
public class OsdGenerator {
    private static final String TAG = OsdGenerator.class.getSimpleName();

    public String[] CHARS = new String[] { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "-", " ", ":", "年", "月", "日", "_",
            "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
            "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
    };

    public Map<String, byte[]> mMapChars = new LinkedHashMap<>();

    private Paint mPaint;
    private OsdConfig mOsdConfig;

    /**
     * 初始化水印生成器
     * @param osdConfig 水印配置项
     */
    public void init(OsdConfig osdConfig) {
        init(osdConfig, null, null);
    }

    /**
     * 初始化水印生成器
     * @param osdConfig 水印配置项
     * @param context   应用上下文对象
     * @param fontPath  asserts路径下的字体文件, 如 fonts/test.ttf
     */
    public void init(OsdConfig osdConfig, Context context, String fontPath) {
        if (null == osdConfig)
            return;
        mPaint = new Paint(Paint.ANTI_ALIAS_FLAG); // 防止边缘锯齿
        mPaint.setFilterBitmap(true); // 对位图进行滤波处理
        mPaint.setColor(Color.WHITE);
        mPaint.setTextSize(osdConfig.getFontSize());
        mPaint.setFakeBoldText(false);
        mPaint.setTextAlign(Paint.Align.LEFT);

        Typeface tf;
        if (null == context || StringUtils.isEmpty(fontPath)) {
            tf = Typeface.create("宋体", Typeface.NORMAL);
        } else {
            tf = Typeface.createFromAsset(context.getAssets(), fontPath);
        }
        mPaint.setTypeface(tf);

        this.mOsdConfig = osdConfig;
        genChars();
        Log.d(TAG, "w*h=" + osdConfig.getCharWidth() * osdConfig.getCharHeight() +
                " w=" + osdConfig.getCharWidth() + " h=" + osdConfig.getCharHeight());
    }

    /**
     * 生成水印字符
     */
    private void genChars() {
        int charWidth = mOsdConfig.getCharWidth();
        int charHeight = mOsdConfig.getCharHeight();

        mMapChars.clear();
        for (String content : CHARS) {
            Rect r = new Rect();
            mPaint.getTextBounds(content, 0, content.length(), r);

            Bitmap srcBit = Bitmap.createBitmap(charWidth, charHeight, Bitmap.Config.ARGB_4444);
            Canvas canvas = new Canvas(srcBit);
            canvas.drawColor(Color.BLACK);
            canvas.drawText(content, (charWidth - r.width()) / 2.f, (r.height() + charHeight) / 2.f, mPaint);

            // 如果要颜色调用bitmapToNV12
            byte[] nv12 = JNIBridge.bitmapToGrayNV(srcBit, charWidth, charHeight);

            StringBuilder sb = new StringBuilder();
            for (int j = 0; j < charHeight; j++) {
                StringBuilder line = new StringBuilder();
                for (int k = 0; k < charWidth; k++) {
                    int index = j * charWidth + k;
                    //黑色为0，其它为1
                    if (nv12[index] == 16) {
                        nv12[index] = 0;
                    } else {
                         //其它为白色
                        nv12[index] = 1;
                    }
                    line.append(nv12[index]);
                }
                sb.append(line).append("\n");
            }
            //Log.d(TAG, " \n" + sb);

            //String str = Arrays.toString(nv12);
            //Log.d(TAG, "c=" + content + " len=" + nv12.length + " " + str);

            mMapChars.put(content, nv12);
        }
        mOsdConfig.setChars(mMapChars);
    }

    private Map<String, byte[]> getChars() {
        return mMapChars;
    }
}
