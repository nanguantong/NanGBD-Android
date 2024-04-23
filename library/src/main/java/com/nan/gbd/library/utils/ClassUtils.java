package com.nan.gbd.library.utils;

import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.util.HashMap;
import java.util.Map;

/**
 * 模板类解析类
 *
 * @author NanGBD
 * @date 2019.7.1
 */
public class ClassUtils {
    private static Map<String, Class<?>> classGenricTypeCache = new HashMap<>(16);

    /**
     * 返回定义类时的泛型参数的类型. <br>
     * 如:定义一个BookManager类<br>
     * <code>{@literal public BookManager extends GenricManager<Book,Address>}{...} </code>
     * <br>
     * 调用getSuperClassGenricType(getClass(),0)将返回Book的Class类型<br>
     * 调用getSuperClassGenricType(getClass(),1)将返回Address的Class类型
     *
     * @param clazz 从哪个类中获取
     * @param index 泛型参数索引,从0开始
     */
    public static Class<?> getSuperClassGenricType(Class<?> clazz, int index) throws IndexOutOfBoundsException {
        String cacheKey = clazz.getName() + index;
        Class<?> cachedClass = classGenricTypeCache.get(cacheKey);
        if (cachedClass != null) {
            return cachedClass;
        }

        Type genType = clazz.getGenericSuperclass();

        // 没有泛型参数
        if (!(genType instanceof ParameterizedType)) {
            throw new RuntimeException("class " + clazz.getName() + " 没有指定父类泛型");
        } else {
            Type[] params = ((ParameterizedType) genType).getActualTypeArguments();

            if (index >= params.length || index < 0) {
                throw new RuntimeException("泛型索引不正确，index:" + index);
            }
            if (!(params[index] instanceof Class)) {
                throw new RuntimeException(params[index] + "不是Class类型");
            }

            Class<?> retClass = (Class<?>) params[index];
            // 缓存起来
            classGenricTypeCache.put(cacheKey, retClass);

            return retClass;
        }
    }
}
