# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}

# Uncomment this to preserve the line number information for
# debugging stack traces.
#-keepattributes SourceFile,LineNumberTable

# If you keep the line number information, uncomment this to
# hide the original source file name.
#-renamesourcefileattribute SourceFile


#一颗星表示只保持该包下的类名，而子包下的类名还是会被混淆（类中的内容也会被混淆）
#-keep class com.nan.gbd.library.*
#两颗星表示把本包和所含子包下的类名都保持（类中的内容也会被混淆）
#-keep class com.nan.gbd.library.media.**

#https://www.guardsquare.com/en/products/proguard/manual/usage
#https://juejin.cn/post/6844903986428903437

#指定压缩级别
-optimizationpasses 5

# 混合时不使用大小写混合，混合后的类名为小写
-dontusemixedcaseclassnames

# 指定不忽略非公共库的类
-dontskipnonpubliclibraryclasses

#不跳过非公共库的类成员
-dontskipnonpubliclibraryclassmembers

# 不做预校验，preverify是proguard的四个步骤之一，Android不需要preverify，去掉这一步能够加快混淆速度，如果需要预校验，是-dontoptimize
-dontpreverify

#混淆时记录日志
-verbose

# 避免混淆Annotation、内部类、泛型、匿名类
-keepattributes *Annotation*,InnerClasses,Signature,EnclosingMethod

# 混淆采用的算法，后面的参数是一个过滤器
# 这个过滤器是谷歌推荐的算法，一般不做更改
-optimizations !code/simplification/arithmetic,!field/*,!class/merging/*

#把混淆类中的方法名也混淆了
-useuniqueclassmembernames

#优化时允许访问并修改有修饰符的类和类的成员
#-allowaccessmodification

#重命名抛出异常时的文件名称,将文件来源重命名为"SourceFile"字符串
-renamesourcefileattribute SourceFile
#抛出异常时保留代码行号
-keepattributes SourceFile,LineNumberTable

# 对于带有回调函数的onXXEvent、**On*Listener的，不能被混淆
-keepclassmembers class * {
    void *(**On*Event);
    void *(**On*Listener);
    void *(**on*Event);
    void *(**on*Listener);
}
# 保留本地native方法不被混淆
-keepclasseswithmembernames class * {
    native <methods>;
}

# 保留枚举类不被混淆
-keepclassmembers enum * {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}
#-keep enum * { *; }

# 保留Parcelable序列化类不被混淆
-keep class * implements android.os.Parcelable {
    public static final android.os.Parcelable$Creator *;
}

#保持所有实现 Serializable 接口的类成员
-keepclassmembers class * implements java.io.Serializable {
  static final long serialVersionUID;
  private static final java.io.ObjectStreamField[] serialPersistentFields;
  private void writeObject(java.io.ObjectOutputStream);
  private void readObject(java.io.ObjectInputStream);
  java.lang.Object writeReplace();
  java.lang.Object readResolve();
}

#Fragment不需要在AndroidManifest.xml中注册，需要额外保护下
-keep public class * extends android.support.v4.app.Fragment
-keep public class * extends android.app.Fragment
-keep public class * extends android.support.v4.**
-keep public class * extends android.support.v7.**
-keep public class * extends android.support.annotation.**

# 保持测试相关的代码
-dontnote junit.framework.**
-dontnote junit.runner.**
-dontwarn android.test.**
-dontwarn android.support.test.**
-dontwarn org.junit.**



## Square okhttp3 specific rules ##
-keep class okhttp3.** { *; }
-keep interface okhttp3.** { *; }
-dontwarn okhttp3.**
-dontnote okhttp3.**

# Okio
-keep class sun.misc.Unsafe { *; }
-dontwarn java.nio.file.*
-dontwarn org.codehaus.mojo.animal_sniffer.IgnoreJRERequirement


## Square Otto specific rules ##
## https://square.github.io/otto/ ##
-keepclassmembers class ** {
    @com.squareup.otto.Subscribe public *;
    @com.squareup.otto.Produce public *;
}


## Square fastjson specific rules ##
-dontwarn com.alibaba.fastjson.**
-keep class com.alibaba.** { *; }
-keep class com.alibaba.fastjson.** { *; }


-keep public class com.nan.gbd.library.media.GBMediaRecorder
-keep public class com.nan.gbd.library.media.MediaRecorderNative
-keep class com.nan.gbd.library.auth.**{*;}
-keep class com.nan.gbd.library.bus.**{*;}
-keep class com.nan.gbd.library.gps.**{*;}
-keep class com.nan.gbd.library.http.request.AuthParam {*;}
-keep class com.nan.gbd.library.http.request.RequestMethod {*;}
-keep class com.nan.gbd.library.http.response.BaseResponse {*;}
-keep interface com.nan.gbd.library.media.GBMediaRecorder$On* {*;}
-keep interface com.nan.gbd.library.media.IMediaRecorder {*;}
-keep class com.nan.gbd.library.osd.**{*;}
-keep class com.nan.gbd.library.sip.**{*;}
-keep enum com.nan.gbd.library.source.MediaSourceType {*;}
-keep class com.nan.gbd.library.utils.** {*;}
-keepclasseswithmembers class com.nan.gbd.library.media.GBMediaRecorder {public *;}
-keepclasseswithmembers class com.nan.gbd.library.JNIBridge {public *;}

