package com.nan.gbd.library.utils;

import android.text.TextPaint;
import android.text.TextUtils;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.Iterator;
import java.util.TimeZone;

/**
 * 字符串工具类
 *
 * @author NanGBD
 * @date 2019.7.1
 */
public class StringUtils {
	public static final String DEFAULT_DATE_PATTERN = "yyyy-MM-dd";
	public static final String DEFAULT_DATETIME_PATTERN = "yyyy-MM-dd hh:mm:ss";
	public static final String DEFAULT_DATETIME_PATTERN_WITH_T = "yyyy-MM-dd'T'HH:mm:ss";
	public static final String DEFAULT_FILE_PATTERN = "yyyy-MM-dd-HH-mm-ss";

	private static final double KB = 1024.0;
	private static final double MB = 1048576.0;
	private static final double GB = 1073741824.0;
	public static final SimpleDateFormat DATE_FORMAT_PART = new SimpleDateFormat("HH:mm");

	public static String currentTimeString() {
		return DATE_FORMAT_PART.format(Calendar.getInstance().getTime());
	}

	public static char chatAt(String pinyin, int index) {
		if (pinyin != null && pinyin.length() > 0)
			return pinyin.charAt(index);
		return ' ';
	}

	/** 获取字符串宽度 */
	public static float getTextWidth(String Sentence, float Size) {
		if (isEmpty(Sentence))
			return 0;
		TextPaint tp = new TextPaint();
		tp.setTextSize(Size);
		return tp.measureText(Sentence.trim()) + (int) (Size * 0.1);
	}

	/**
	 * 格式化日期字符串
	 * 
	 * @param date
	 * @param pattern
	 * @return
	 */
	public static String formatDate(Date date, String pattern) {
		SimpleDateFormat format = new SimpleDateFormat(pattern);
		return format.format(date);
	}

	/**
	 * 格式化日期字符串
	 *
	 * @param date
	 * @param pattern
	 * @return
	 */
	public static String formatDate(long date, String pattern) {
		SimpleDateFormat format = new SimpleDateFormat(pattern);
		return format.format(new Date(date));
	}

	/**
	 * 格式化日期字符串, 格式为yyyy-MM-dd
	 * 
	 * @param date
	 * @return 例如2011-03-24
	 */
	public static String formatDate(Date date) {
		return formatDate(date, DEFAULT_DATE_PATTERN);
	}

	/**
	 * 格式化日期字符串, 格式为yyyy-MM-dd
	 *
	 * @param date
	 * @return 例如2011-03-24
	 */
	public static String formatDate(long date) {
		return formatDate(new Date(date), DEFAULT_DATE_PATTERN);
	}

	/**
	 * 获取当前时间, 格式为yyyy-MM-dd
	 *
	 * @return 例如2011-03-24
	 */
	public static String getDate() {
		return formatDate(new Date(), DEFAULT_DATE_PATTERN);
	}

	/** 生成一个文件名，不含后缀 */
	public static String createFileName() {
		Date date = new Date(System.currentTimeMillis());
		SimpleDateFormat format = new SimpleDateFormat(DEFAULT_FILE_PATTERN);
		return format.format(date);
	}

	/**
	 * 获取当前时间, 格式为yyyy-MM-dd hh:mm:ss
	 * 
	 * @return 例如2011-11-30 16:06:54
	 */
	public static String getDateTime() {
		return formatDate(new Date(), DEFAULT_DATETIME_PATTERN);
	}

	/**
	 * 格式化日期时间字符串, 格式为yyyy-MM-dd hh:mm:ss
	 * 
	 * @param date
	 * @return 例如2011-11-30 16:06:54
	 */
	public static String getDateTime(Date date) {
		return formatDate(date, DEFAULT_DATETIME_PATTERN);
	}

	/**
	 * 格式化日期时间字符串, 格式为yyyy-MM-dd hh:mm:ss
	 *
	 * @param date
	 * @return 例如2011-11-30 16:06:54
	 */
	public static String getDateTime(long date) {
		return formatDate(new Date(date), DEFAULT_DATETIME_PATTERN);
	}

	/**
	 * 格林威时间转换
	 * 
	 * @param gmt
	 * @return
	 */
	public static String getGMTDate(String gmt) {
		TimeZone timeZoneLondon = TimeZone.getTimeZone(gmt);
		return formatDate(Calendar.getInstance(timeZoneLondon).getTimeInMillis());
	}

	/**
	 * 拼接数组
	 * 
	 * @param array
	 * @param separator
	 * @return
	 */
	public static String join(final ArrayList<String> array, final String separator) {
		StringBuilder result = new StringBuilder();
		if (array != null && array.size() > 0) {
			for (String str : array) {
				result.append(str);
				result.append(separator);
			}
			result.delete(result.length() - 1, result.length());
		}
		return result.toString();
	}

	public static String join(final Iterator<String> iter, final String separator) {
		StringBuilder result = new StringBuilder();
		if (iter != null) {
			while (iter.hasNext()) {
				String key = iter.next();
				result.append(key);
				result.append(separator);
			}
			if (result.length() > 0)
				result.delete(result.length() - 1, result.length());
		}
		return result.toString();
	}

	/**
	 * 判断字符串是否为空
	 * 
	 * @param str
	 * @return
	 */
	public static boolean isEmpty(String str) {
		return str == null || str.length() == 0 || str.equalsIgnoreCase("null");
	}

	public static boolean isNotEmpty(String str) {
		return !isEmpty(str);
	}

	/**
	 * 去除首尾空格
	 *
	 * @param str
	 * @return
	 */
	public static String trim(String str) {
		return str == null ? "" : str.trim();
	}

	/**
	 * 转换时间显示
	 * 
	 * @param time 毫秒
	 * @return
	 */
	public static String generateTime(long time) {
		int totalSeconds = (int) (time / 1000);
		int seconds = totalSeconds % 60;
		int minutes = (totalSeconds / 60) % 60;
		int hours = totalSeconds / 3600;

		return hours > 0 ? String.format("%02d:%02d:%02d", hours, minutes, seconds) :
				String.format("%02d:%02d", minutes, seconds);
	}

	public static boolean isBlank(String s) {
		return TextUtils.isEmpty(s);
	}

	/** 根据秒速获取时间格式 */
	public static String generateTime(int totalSeconds) {
		int seconds = totalSeconds % 60;
		int minutes = (totalSeconds / 60) % 60;
		return String.format("%02d:%02d", minutes, seconds);
	}

	/**
	 * 转换文件大小
	 * 
	 * @param size
	 * @return
	 */
	public static String generateFileSize(long size) {
		String fileSize;
		if (size < KB)
			fileSize = size + "B";
		else if (size < MB)
			fileSize = String.format("%.1f", size / KB) + "KB";
		else if (size < GB)
			fileSize = String.format("%.1f", size / MB) + "MB";
		else
			fileSize = String.format("%.1f", size / GB) + "GB";

		return fileSize;
	}

	/** 查找字符串，找到返回，没找到返回空 */
	public static String findString(String search, String start, String end) {
		int start_len = start.length();
		int start_pos = StringUtils.isEmpty(start) ? 0 : search.indexOf(start);
		if (start_pos > -1) {
			int end_pos = StringUtils.isEmpty(end) ? -1 : search.indexOf(end,
					start_pos + start_len);
			if (end_pos > -1)
				return search.substring(start_pos + start.length(), end_pos);
		}
		return "";
	}

	/**
	 * 截取字符串
	 * 
	 * @param search 待搜索的字符串
	 * @param start  起始字符串 例如：<title>
	 * @param end    结束字符串 例如：</title>
	 * @param defaultValue
	 * @return
	 */
	public static String substring(String search, String start, String end, String defaultValue) {
		int start_len = start.length();
		int start_pos = StringUtils.isEmpty(start) ? 0 : search.indexOf(start);
		if (start_pos > -1) {
			int end_pos = StringUtils.isEmpty(end) ? -1 : search.indexOf(end,
					start_pos + start_len);
			if (end_pos > -1)
				return search.substring(start_pos + start.length(), end_pos);
			else
				return search.substring(start_pos + start.length());
		}
		return defaultValue;
	}

	/**
	 * 截取字符串
	 * 
	 * @param search 待搜索的字符串
	 * @param start  起始字符串 例如：<title>
	 * @param end    结束字符串 例如：</title>
	 * @return
	 */
	public static String substring(String search, String start, String end) {
		return substring(search, start, end, "");
	}

	/**
	 * 拼接字符串
	 * 
	 * @param strs
	 * @return
	 */
	public static String concat(String... strs) {
		StringBuffer result = new StringBuffer();
		if (strs != null) {
			for (String str : strs) {
				if (str != null)
					result.append(str);
			}
		}
		return result.toString();
	}

	/**
	 * Helper function for making null strings safe for comparisons, etc.
	 * 
	 * @return (s == null) ? "" : s;
	 */
	public static String makeSafe(String s) {
		return (s == null) ? "" : s;
	}

	public static byte[] int2Bytes(byte[] bytes, int offset, int val) {
		bytes[offset + 0] = (byte) (val >> 24);
		bytes[offset + 1] = (byte) (val >> 16);
		bytes[offset + 2] = (byte) (val >> 8);
		bytes[offset + 3] = (byte) (val);
		return bytes;
	}

	public static byte[] long2Bytes(byte[] bytes, int offset, long val) {
		bytes[offset + 0] = (byte) (val >> 56);
		bytes[offset + 1] = (byte) (val >> 48);
		bytes[offset + 2] = (byte) (val >> 40);
		bytes[offset + 3] = (byte) (val >> 32);
		bytes[offset + 4] = (byte) (val >> 24);
		bytes[offset + 5] = (byte) (val >> 16);
		bytes[offset + 6] = (byte) (val >> 8);
		bytes[offset + 7] = (byte) val;
		return bytes;
	}

	public final static String UTF8 = "UTF-8";
	public static String encodeUrl(String input) {
		try {
			return URLEncoder.encode(input, UTF8);
		} catch (UnsupportedEncodingException e) {
			throw new RuntimeException(e);
		}
	}
}
