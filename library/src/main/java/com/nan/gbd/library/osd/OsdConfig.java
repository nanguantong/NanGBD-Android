package com.nan.gbd.library.osd;

import java.util.Arrays;
import java.util.Map;
import java.util.Vector;

/**
 * @author NanGBD
 * @date 2020.6.25
 */
public class OsdConfig {
    public static class Osd {
        public String text; // 水印内容
        /**
         * （0, 0）代表视频左上角
         * 取值为正代表从视频左上角开始，取值为负数代表从视频右下角开始
         */
        public int x;       // 水印在视频左上的x偏移
        public int y;       // 水印在视频左上的y偏移

        /**
         * 如果大于0, 则text表示日期格式:
         * c++:
         *     %Y-%m-%d %H:%M:%S
         *     %Y年%m月%d日 %H:%M:%S
         */
        public int dateLen;
    }

    private boolean enable;      // 是否开启水印
    private int fontSize = 15;   // 水印文字大小
    private int charWidth = 17;  // 水印单个字符宽度
    private int charHeight = 23; // 水印单个字符高度
    private Vector<Osd> osds;    // osd水印列表
    private Map<String, byte[]> chars; // 预生成的字符集yuv数据列表

    public boolean isEnable() {
        return enable;
    }

    public void setEnable(boolean enable) {
        this.enable = enable;
    }

    public int getFontSize() {
        return fontSize;
    }

    public void setFontSize(int fontSize) {
        this.fontSize = fontSize;
    }

    public int getCharWidth() {
        return charWidth;
    }

    public void setCharWidth(int charWidth) {
        this.charWidth = charWidth;
    }

    public int getCharHeight() {
        return charHeight;
    }

    public void setCharHeight(int charHeight) {
        this.charHeight = charHeight;
    }

    public Vector<Osd> getOsds() {
        return osds;
    }

    public Osd[] getOsdsArray() {
        if (null == osds) {
            return null;
        }
        Object[] objArray = osds.toArray();
        if (null == objArray) {
            return null;
        }
        Osd[] os = Arrays.copyOf(objArray, objArray.length, Osd[].class);
        return os;
    }

    public void setOsds(Vector<Osd> osds) {
        this.osds = osds;
    }

    public boolean addOsd(Osd osd) {
        if (null == osd || null == osd.text || osd.text.isEmpty()) {
            return false;
        }
        if (null == osds) {
            osds = new Vector<>();
        }
        for (Osd o : osds) {
            if (osd.text.equals(o.text) && osd.x == o.x && osd.y == o.y) {
                // 不要重复添加
                return false;
            }
        }
        osds.add(osd);
        return true;
    }

    public Map<String, byte[]> getChars() {
        return chars;
    }

    public void setChars(Map<String, byte[]> chars) {
        this.chars = chars;
    }

    public byte[][] getCharsArray() {
        if (null == chars) {
            return null;
        }
        Object[] objArray = chars.values().toArray();
        byte[][] os = Arrays.copyOf(objArray, objArray.length, byte[][].class);
        return os;
    }

}
