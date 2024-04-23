package com.nan.gbd.library.utils;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * 签名工具类
 *
 * @author NanGBD
 * @date 2019.7.1
 */
public class SignUtils {
    /**
     * 构建签名
     *
     * @param paramsMap 参数
     * @param secret 密钥
     * @return
     * @throws IOException
     */
    public static String createSign(Map<String, ?> paramsMap, String secret) {
        Set<String> keySet = paramsMap.keySet();
        List<String> paramNames = new ArrayList<String>(keySet);

        Collections.sort(paramNames);

        StringBuilder paramNameValue = new StringBuilder();

        for (String paramName : paramNames) {
            paramNameValue.append(paramName).append(paramsMap.get(paramName));
        }

        String source = secret + paramNameValue.toString() + secret;

        return MD5Utils.encryptUpper(source);
    }
}
