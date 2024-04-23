//
// Created by nanuns on 2018/8/1.
//

extern "C"
{
#include <libavcodec/jni.h>
}
#include <jni.h>
#include "utils/log.h"
#include "osd/osd.h"
#include "ua/sip_ua.h"
#include "gb_muxer.h"
#include "gb_demuxer.h"

static JavaVM* gJavaVM = NULL;
static JNIEnv* gJINEnv = NULL;
static jclass gJNIBridge = NULL;
static jmethodID gGbCallback = NULL;

GB28181Muxer *gb_muxer = NULL;
GB28181Demuxer *gb_demuxer = NULL;
std::mutex gb_mtx, gb_dmtx;
osd_info_t g_osd_info;

int startStream(const task_t* task);
int startVoice(const task_t* task);
int stopStream(int cid);
int stopVoice(int cid);

static jclass ClassString;
static jmethodID CtorIDString;
static jmethodID GetBytesIDString;


static jstring char2jstring(JNIEnv* env, const char* str, const char* charset) {
    jstring ret = NULL;
    jbyteArray bytes = env->NewByteArray(strlen(str));
    env->SetByteArrayRegion(bytes, 0, strlen(str), (jbyte*) str);
    jstring encoding = (env)->NewStringUTF(charset); // "GB2312"
    ret = (jstring) (env)->NewObject(ClassString, CtorIDString, bytes, encoding);

    env->DeleteLocalRef(bytes);
    env->DeleteLocalRef(encoding);
    return ret;
}

static char* jstring2char(JNIEnv* env, jstring jstr, const char* charset) {
    char* ret = NULL;
    jstring encoding = env->NewStringUTF(charset); // "GB2312"
    jbyteArray arr = (jbyteArray) env->CallObjectMethod(jstr, GetBytesIDString, encoding);
    jsize len = env->GetArrayLength(arr);
    jbyte* ba = env->GetByteArrayElements(arr, JNI_FALSE);
    if (len > 0) {
        ret = (char*) malloc(len + 1);
        memcpy(ret, ba, len);
        ret[len] = 0;
    }

    env->ReleaseByteArrayElements(arr, ba, 0);
    env->DeleteLocalRef(encoding);
    return ret;
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGD("JNI_OnLoad");

    JNIEnv* env = NULL;
    jint result = JNI_ERR;
    gJavaVM = vm;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("JNI env not got");
        return result;
    }
    gJINEnv = env;

    jclass clazz = env->FindClass("com/nan/gbd/library/JNIBridge");
    if (clazz == NULL) {
        LOGE("Unable to find jni class");
        return result;
    }
    gJNIBridge = (jclass) env->NewGlobalRef(clazz);
    gGbCallback = env->GetStaticMethodID(gJNIBridge, "onGBDeviceCallback", "(II[BI)I");
    if (gGbCallback == NULL) {
        LOGE("Unable to find jni callback");
        return result;
    }
    gJINEnv->DeleteLocalRef(clazz);
    av_jni_set_java_vm(vm, reserved);

    ClassString = env->FindClass("java/lang/String");
    CtorIDString = env->GetMethodID(ClassString, "<init>","([BLjava/lang/String;)V");
    GetBytesIDString = env->GetMethodID(ClassString, "getBytes", "(Ljava/lang/String;)[B");

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved) {
    LOGI("JNI_OnUnload");
    if (gJINEnv == NULL) {
        return;
    }
    gJINEnv->DeleteGlobalRef(gJNIBridge);
}


extern "C"
JNIEXPORT jint JNICALL
Java_com_nan_gbd_library_JNIBridge_nanOsdInit(JNIEnv *env, jclass type,
                                              jboolean enable, jint char_width, jint char_height,
                                              jobjectArray osds, jobjectArray chars) {
    if (NULL == osds || NULL == chars) {
        return -1;
    }
    jsize osds_num = env->GetArrayLength(osds);
    if (0 == osds_num) {
        return -1;
    }
    jsize chars_num = env->GetArrayLength(chars);
    if (0 == chars_num) {
        return -1;
    }

    g_osd_info.osds.clear();
    g_osd_info.chars.clear();
    g_osd_info.enable = enable;
    g_osd_info.char_width = char_width;
    g_osd_info.char_height = char_height;

    for (int i = 0; i < osds_num; i++) {
        // https://www.jianshu.com/p/ea48e15bb9f8 JNI开发之传递自定义对象
        jobject o = env->GetObjectArrayElement(osds, i);
        if (NULL == o) {
            continue;
        }
        jclass c = env->GetObjectClass(o);
        if (NULL == c) {
            continue;
        }
        jfieldID f_text = env->GetFieldID(c, "text","Ljava/lang/String;");
        jfieldID f_x = env->GetFieldID(c, "x","I");
        jfieldID f_y = env->GetFieldID(c, "y","I");
        jfieldID f_date_len = env->GetFieldID(c, "dateLen","I");

        osd_t osd;
        jstring s_text = (jstring) env->GetObjectField(o, f_text);
        const char *c_text = env->GetStringUTFChars(s_text, 0);
        osd.text = c_text;
        osd.x = env->GetIntField(o, f_x);
        osd.y = env->GetIntField(o, f_y);
        osd.date_len = env->GetIntField(o, f_date_len);
        g_osd_info.osds.push_back(osd);

        env->ReleaseStringUTFChars(s_text, c_text);
    }

    // https://blog.csdn.net/u013795543/article/details/104101490
    // 传递二维byte数组
    for (int i = 0; i < chars_num; i++) {
        jbyteArray o = (jbyteArray) env->GetObjectArrayElement(chars, i);
        if (NULL == o) {
            continue;
        }

        jbyte *bytes = env->GetByteArrayElements(o, NULL);
        int len = env->GetArrayLength(o);

//        char *chars = new char[len + 1];
//        memset(chars, 0, len + 1);
//        memcpy(chars, bytes, len);
//        chars[len] = 0;
//        g_osd_info.chars.emplace_back(chars);

        string str;
        str.assign(reinterpret_cast<char *>(bytes), len);
        g_osd_info.chars.emplace_back(str);

        env->ReleaseByteArrayElements(o, bytes, 0);
    }
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_nan_gbd_library_JNIBridge_nanOsdUninit(JNIEnv *env, jclass type) {
    g_osd_info.osds.clear();
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_nan_gbd_library_JNIBridge_nanUaInit(JNIEnv *env, jclass type,
                                             jstring server_ip, jint server_port, jstring server_id, jstring server_domain,
                                             jint gm_level, jstring name, jstring manufacturer, jstring model, jstring firmware, jstring serial_number,
                                             jint channel,jstring username, jstring userid, jstring password, jint local_port, jstring protocol,
                                             jint expiry, jint heartbeat_interval, jint heartbeat_count,
                                             jdouble longitude, jdouble latitude,
                                             jstring video_id, jstring audio_id, jstring alarm_id, jstring charset,
                                             jint max_invite_count, jint max_voice_count, jint pos_capability, jint global_auth) {
    int ret = -1;
    if (NULL == server_ip || NULL == server_id || NULL == server_domain || NULL == name ||
        NULL == manufacturer || NULL == model || NULL == firmware || NULL == serial_number ||
        NULL == username || NULL == userid || NULL == password || NULL == protocol ||
        NULL == video_id || NULL == audio_id || NULL == alarm_id || NULL == charset) {
        LOGE("invalid sip params");
        return ret;
    }
    UserArgs *args;
    ua_config_t config;
    config.server_ip = (char *) env->GetStringUTFChars(server_ip, 0);
    config.server_port = server_port;
    config.server_id = (char *) env->GetStringUTFChars(server_id, 0);
    config.server_domain = (char *) env->GetStringUTFChars(server_domain, 0);

    config.gm_level = gm_level;
#if !ENABLE_35114
    config.gm_level = 0;
#endif
    config.charset = (char *) env->GetStringUTFChars(charset, 0);

//    const jchar *j_name = NULL, *j_manufacturer = NULL, *j_model = NULL;
    if (strcmp(config.charset, "GB2312") == 0) {
        config.name = jstring2char(env, name, config.charset);
        config.manufacturer = jstring2char(env, manufacturer, config.charset);
        config.model = jstring2char(env, model, config.charset);

//        jsize len = env->GetStringLength(name);
//        j_name = env->GetStringChars(name, 0);
//        config.name = (char*) malloc(len + 1);
//        wchar_t *w_buf = new wchar_t[len];
//        wcsncpy(w_buf, reinterpret_cast<const wchar_t *>(j_name), len); w_buf[len] = '\0';
//        len = wcstombs(config.name, w_buf, len);
//
//        len = env->GetStringLength(manufacturer);
//        j_manufacturer = env->GetStringChars(manufacturer, 0);
//        config.manufacturer = (char*) malloc(len + 1);
//        len = wcstombs(config.manufacturer, (const wchar_t*) j_manufacturer, len);
//
//        len = env->GetStringLength(model);
//        j_model = env->GetStringChars(model, 0);
//        config.model = (char*) malloc(len + 1);
//        len = wcstombs(config.model, (const wchar_t*) j_model, len);
    } else { // UTF-8
        config.name = (char *) env->GetStringUTFChars(name, 0);
        config.manufacturer = (char *) env->GetStringUTFChars(manufacturer, 0);
        config.model = (char *) env->GetStringUTFChars(model, 0);
    }
    config.firmware = (char *) env->GetStringUTFChars(firmware, 0);
    config.serial_number = (char *) env->GetStringUTFChars(serial_number, 0);
    config.channel = channel;

    config.username = (char *) env->GetStringUTFChars(username, 0);
    config.userid = (char *) env->GetStringUTFChars(userid, 0);
    config.password = (char *) env->GetStringUTFChars(password, 0);
    config.local_port = local_port;
    config.protocol = (char *) env->GetStringUTFChars(protocol, 0);

    config.expiry = expiry;
    config.heartbeat_interval = heartbeat_interval;
    config.heartbeat_count = heartbeat_count;

    config.longitude = longitude;
    config.latitude = latitude;

    config.video_id = (char *) env->GetStringUTFChars(video_id, 0);
    config.audio_id = (char *) env->GetStringUTFChars(audio_id, 0);
    config.alarm_id = (char *) env->GetStringUTFChars(alarm_id, 0);

    config.invite_count = max_invite_count > MAX_INVITE_COUNT ? MAX_INVITE_COUNT : max_invite_count;
    config.voice_count = max_voice_count > MAX_VOICE_COUNT ? MAX_VOICE_COUNT : max_voice_count;
    config.pos_capability = pos_capability;
    config.global_auth = global_auth;

    config.on_callback_status = onCallbackStatus;
    config.on_callback_invite = onCallbackInvite;
    config.on_callback_cancel = onCallbackCancel;
    config.on_callback_record = onCallbackRecord;
    config.on_callback_gm_signature = onCallbackGmSignature;
    config.on_callback_gm_encrypt = onCallbackGmEncrypt;

    ret = ua_init(&config);
    if (ret != 0) {
        goto clean;
    }

    if (NULL == gb_muxer) {
        args = (UserArgs *) calloc(1, sizeof(UserArgs));
        args->mtx_sender = new std::mutex;
        args->senders = new std::map<int, StreamSender>;
        args->invite_count = max_invite_count;
        args->voice_count = max_voice_count;
        gb_muxer = new GB28181Muxer(args);
    }
    if (NULL == gb_demuxer) {
        gb_demuxer = new GB28181Demuxer(args);
    }

clean:
    env->ReleaseStringUTFChars(server_ip,     config.server_ip);
    env->ReleaseStringUTFChars(server_id,     config.server_id);
    env->ReleaseStringUTFChars(server_domain, config.server_domain);
    if (strcmp(config.charset, "GB2312") == 0) {
//        env->ReleaseStringChars(name,          j_name);
//        env->ReleaseStringChars(manufacturer,  j_manufacturer);
//        env->ReleaseStringChars(model,         j_model);
        free(config.name);
        free(config.manufacturer);
        free(config.model);
    } else {
        env->ReleaseStringUTFChars(name,          config.name);
        env->ReleaseStringUTFChars(manufacturer,  config.manufacturer);
        env->ReleaseStringUTFChars(model,         config.model);
    }
    env->ReleaseStringUTFChars(firmware,      config.firmware);
    env->ReleaseStringUTFChars(serial_number, config.serial_number);
    env->ReleaseStringUTFChars(username,      config.username);
    env->ReleaseStringUTFChars(userid,        config.userid);
    env->ReleaseStringUTFChars(password,      config.password);
    env->ReleaseStringUTFChars(protocol,      config.protocol);
    env->ReleaseStringUTFChars(video_id,      config.video_id);
    env->ReleaseStringUTFChars(audio_id,      config.audio_id);
    env->ReleaseStringUTFChars(alarm_id,      config.alarm_id);
    env->ReleaseStringUTFChars(charset,       config.charset);

    return ret;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_nan_gbd_library_JNIBridge_nanUaRegister(JNIEnv *env, jclass type) {
    return ua_register();
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_nan_gbd_library_JNIBridge_nanUaUnregister(JNIEnv *env, jclass type) {
    return ua_unregister();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_nan_gbd_library_JNIBridge_nanUaIsRegistered(JNIEnv *env, jclass type) {
    return ua_isregistered() > 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_nan_gbd_library_JNIBridge_nanUaOnConfigChanged(JNIEnv *env, jclass type, jboolean changed) {
    ua_on_config_changed(changed ? 1 : 0);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_nan_gbd_library_JNIBridge_nanUaGetDeviceCert(JNIEnv *env, jclass type) {
    int ret;
    char cert[65535];

    ret = ua_get_device_cert(cert, sizeof(cert));
    if (ret != 0) {
        return NULL;
    }

    jbyteArray bytes = env->NewByteArray(strlen(cert));
    env->SetByteArrayRegion(bytes, 0, strlen(cert), (jbyte*)cert);

    jstring encoding = (env)->NewStringUTF("utf-8");
    jstring str = (jstring) (env)->NewObject(ClassString, CtorIDString, bytes, encoding);

    env->DeleteLocalRef(bytes);
    env->DeleteLocalRef(encoding);

    return str;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_nan_gbd_library_JNIBridge_nanUaSetServerCert(JNIEnv *env, jclass type,
                                                      jstring server_id, jstring cert, jboolean is_file, jstring passin) {
    int ret = -1;
    if (NULL == server_id || NULL == cert) {
        LOGE("invalid cert params");
        return ret;
    }
    const char *s_server_id = (char *) env->GetStringUTFChars(server_id, 0);
    const char *s_cert = (char *) env->GetStringUTFChars(cert, 0);
    char *s_passin = NULL;
    if (NULL != passin) {
        s_passin = (char *) env->GetStringUTFChars(passin, 0);
    }

    ret = ua_set_server_cert(s_server_id, s_cert, is_file ? 1 : 0, s_passin);

    env->ReleaseStringUTFChars(server_id, s_server_id);
    env->ReleaseStringUTFChars(cert, s_cert);
    if (NULL != s_passin) {
        env->ReleaseStringUTFChars(passin, s_passin);
    }

    return ret;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_nan_gbd_library_JNIBridge_nanUaUninit(JNIEnv *env, jclass type) {
    ua_uninit();
    delete gb_muxer; gb_muxer = NULL;
    delete gb_demuxer; gb_demuxer = NULL;
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_nan_gbd_library_JNIBridge_nanUaSetMobilePos(JNIEnv *env, jclass type,
                                                     jstring time, jdouble longitude, jdouble latitude,
                                                     jdouble speed, jdouble direction, jdouble altitude) {
    if (NULL == time) {
        LOGE("invalid pos params");
        return -1;
    }

    const char *s_time = env->GetStringUTFChars(time, 0);

    ua_mobilepos_t mobilepos;
    strcpy(mobilepos.time, s_time);
    mobilepos.longitude = longitude;
    mobilepos.latitude = latitude;
    mobilepos.speed = speed;
    mobilepos.direction = direction;
    mobilepos.altitude = altitude;

    env->ReleaseStringUTFChars(time, s_time);

    return ua_set_mobilepos(&mobilepos);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_nan_gbd_library_JNIBridge_nanUaSetAlarm(JNIEnv *env, jclass type,
                                                 jstring priority, jstring method, jstring time, jstring desc,
                                                 jdouble longitude, jdouble latitude,
                                                 jint alarm_type, jint event_type) {
    if (NULL == priority || NULL == method || NULL == time) {
        LOGE("invalid alarm params");
        return -1;
    }

    const char *s_priority = env->GetStringUTFChars(priority, 0);
    const char *s_method = env->GetStringUTFChars(method, 0);
    const char *s_time = env->GetStringUTFChars(time, 0);

    ua_alarm_t alarm;
    strcpy(alarm.priority, s_priority);
    strcpy(alarm.method, s_method);
    strcpy(alarm.time, s_time);
    strcpy(alarm.priority, s_priority);
    alarm.longitude = longitude;
    alarm.latitude = latitude;
    alarm.type = alarm_type;
    alarm.event_type = event_type;

    env->ReleaseStringUTFChars(priority, s_priority);
    env->ReleaseStringUTFChars(method, s_method);
    env->ReleaseStringUTFChars(time, s_time);

    if (NULL != desc) {
        const char *s_desc = env->GetStringUTFChars(desc, 0);
        strcpy(alarm.desc, s_desc);
        env->ReleaseStringUTFChars(desc, s_desc);
    }

    return ua_set_alarm(&alarm);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_nan_gbd_library_JNIBridge_nanInitMuxer(JNIEnv *env, jclass type,
                                                jstring mediaPath, jstring mediaName, jint recDuration,
                                                jboolean enableVideo, jboolean enableAudio,
                                                jint videoEncodec, jint videoRotate,
                                                jint inWidth, jint inHeight, jint outWidth, jint outHeight,
                                                jint framerate, jint gop, jlong videoBitrate, jint videoBitrateMode,
                                                jint videoProfile, jint audioFrameLen, jint threadNum, jint queueMax,
                                                jdouble videoBitrateFactor, jint videoBitrateAdjustPeriod, jint streamStatPeriod) {
    LOGI("start init muxer, tid:%d", gettid());
    std::lock_guard<std::mutex> lk(gb_mtx);
    if (gb_muxer == NULL) {
        return -1;
    }
    UserArgs* args = gb_muxer->getArgs();
    if (args == NULL) {
        return -1;
    }

    const char *media_path = env->GetStringUTFChars(mediaPath, 0);
    const char *media_name = env->GetStringUTFChars(mediaName, 0);
    args->rec_duration = recDuration < 0 ? 0 : (recDuration > MAX_REC_DURATION_SEC ? MAX_REC_DURATION_SEC : recDuration);

    if (media_path != NULL && strlen(media_path) > 0) {
        free(args->media_path);
        args->media_path = strdup(media_path);
        ua_set_rec_path(media_path);
        LOGI("media path: %s", args->media_path);
    }
    // test
    if (media_name != NULL && strlen(media_name) > 0) {
    }

    args->enable_video = enableVideo;
    args->enable_audio = enableAudio;
    args->v_encodec =  (VideoCodec) videoEncodec;
    args->v_rotate = videoRotate;
    args->in_width = inWidth;
    args->in_height = inHeight;
    args->out_width = outWidth;
    args->out_height = outHeight;
    args->frame_rate = framerate > 0 ? framerate : 15;
    args->gop = gop > 0 ? gop : 5;
    args->v_bitrate = videoBitrate;
    args->v_bitrate_mode = videoBitrateMode;
    args->v_profile = (VideoProfile) videoProfile;
    args->v_color_format = COLOR_FormatYUV420SemiPlanar; // COLOR_FormatYUV420Flexible
    args->a_frame_len = audioFrameLen;
    args->thread_num = threadNum;
    args->queue_max = queueMax;
    args->v_bitrate_factor = videoBitrateFactor >= 0 ? videoBitrateFactor : BITRATE_FACTOR;
    args->v_bitrate_adjust_period = (videoBitrateAdjustPeriod >= 0 ? videoBitrateAdjustPeriod : BITRATE_ADJUST_PERIOD_SEC);
    args->stream_stat_period = (streamStatPeriod >= 0 ? streamStatPeriod : STREAM_STAT_PERIOD_SEC);

    env->ReleaseStringUTFChars(mediaPath, media_path);
    env->ReleaseStringUTFChars(mediaName, media_name);

    return gb_muxer->initMuxer();
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_nan_gbd_library_JNIBridge_nanEndMuxer(JNIEnv *env, jclass type) {
    LOGI("start end muxer, tid:%d", gettid());
    if (gb_muxer == NULL) {
        return -1;
    }
    UserArgs* args = gb_muxer->getArgs();
    if (args == NULL) {
        return -1;
    }
    LOGI("start end muxer sender");
    args->mtx_sender->lock();
    for (auto &it : *args->senders) {
        StreamSender *sender = &it.second;
        gb_muxer->endSender(sender);
    }
    args->senders->clear();
    args->mtx_sender->unlock();

    std::lock_guard<std::mutex> lk(gb_mtx);
    return gb_muxer->endMuxer();
}


extern "C"
JNIEXPORT jint JNICALL
Java_com_nan_gbd_library_JNIBridge_nanPushVideoFrame(JNIEnv *env, jclass type, jbyteArray data, jlong ts, jboolean keyframe) {
    if (gb_muxer == NULL) {
        return -1;
    }
    jbyte *b = env->GetByteArrayElements(data, NULL);
    int len = env->GetArrayLength(data);
    int ret = gb_muxer->pushVideoFrame(reinterpret_cast<uint8_t *>(b), len, ts, keyframe);
    env->ReleaseByteArrayElements(data, b, 0);
    return ret;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_nan_gbd_library_JNIBridge_nanPushAudioFrame(JNIEnv *env, jclass type, jbyteArray data, jlong pts) {
    if (gb_muxer == NULL) {
        return -1;
    }
    jbyte *b = env->GetByteArrayElements(data, NULL);
    int len = env->GetArrayLength(data);
    int ret = gb_muxer->pushAudioFrame(reinterpret_cast<uint8_t *>(b), len, pts);
    env->ReleaseByteArrayElements(data, b, 0);
    return ret;
}

static JNIEnv* getJNIEnv(JavaVM* pJavaVM)
{
    JavaVMAttachArgs attachArgs;
    attachArgs.version = JNI_VERSION_1_6;
    attachArgs.name = "NativeCallBack";
    attachArgs.group = NULL;
    JNIEnv* env;
    if (pJavaVM->AttachCurrentThread(&env, &attachArgs) != JNI_OK) {
        env = NULL;
    }
    return env;
}

static int jniCallback(int event_type, int key, const char* param, int param_len)
{
    bool needDetach = false;
    JNIEnv *jniEnv;
    jbyteArray paramArray = NULL;

    // 1.获取当前Java线程下的env；若成功则说明当前执行的线程是Java线程，否则就是C++线程，需要调用getJNIEnv来获取Java环境
    // 2.此处需要重新通过全局的jvm获取env，因为这是不同的线程，对应不同的env
    jint ret = gJavaVM->GetEnv(reinterpret_cast<void **> (&jniEnv), JNI_VERSION_1_6);
    if (ret != JNI_OK) {
        //c++ thread; need acquire java environment by the function of getJNIEnv
        jniEnv = getJNIEnv(gJavaVM);
        needDetach = true;
    }

    // 3.获取Java环境失败
    if (jniEnv == NULL) {
        LOGE("getJNIEnv failed");
        return -1;
    }

    // 4.通过全局引用获取类对象
//    jclass clazz = jniEnv->GetObjectClass(gJNIBridge);
//    if (clazz == NULL) {
//        LOGE("find class error");
//        if (needDetach) {
//            gJavaVM->DetachCurrentThread();
//        }
//        return -1;
//    }

    if (param != NULL && param_len > 0) {
        paramArray = jniEnv->NewByteArray(param_len);
        jniEnv->SetByteArrayRegion(paramArray, 0, param_len, (jbyte*)param);
    }
    // 5.调用Java层的静态方法，在该线程中调用容易被优化，以至于找不到方法，
    // 因此需要在调用之前，在Java层调用下该函数，并拿到该方法引用
    ret = jniEnv->CallStaticIntMethod(gJNIBridge, gGbCallback, event_type, key, paramArray, param_len);
    if (paramArray != NULL) {
        jniEnv->DeleteLocalRef(paramArray);
    }

    // 6.如果是C++运行线程，才需要DetachCurrentThread，否则会引起detaching thread with interp frames
    if (needDetach) {
        gJavaVM->DetachCurrentThread();
    }
    jniEnv = NULL;
    return ret;
}

int onCallbackStatus(int event_type, int key, const char* param, int param_len)
{
    int ret = jniCallback(event_type, key, param, param_len);
    if (ret != 0) {
        LOGE("callback %d failed", event_type);
    }
    return ret;
}

// 停止invite task
int onCallStop(int cid, bool is_voice, bool stop)
{
    int ret = 0;
    LOGI("invite call stop %d cid:%d, tid:%d", stop, cid, gettid());
    if (gb_muxer == NULL) {
        return -1;
    }
    UserArgs* args = gb_muxer->getArgs();
    if (args == NULL) {
        return -1;
    }
    ua_call_stop(cid);
    if (stop) {
        if (is_voice) {
            ret = stopVoice(cid);
        } else {
            ret = stopStream(cid);
        }
    }
    return ret;
}

// 批量停止invite task
int onCallsStop(std::vector<int> cids, bool is_voice, bool stop)
{
    for (int cid : cids) {
        onCallStop(cid, is_voice, stop);
    }
    return 0;
}

int startStream(const task_t* task)
{
    int ret = -1;
    UserArgs* args = gb_muxer->getArgs();
    StreamSender sender;

    args->mtx_sender->lock();
    do {
        auto it = args->senders->find(task->cid);
        if (it != args->senders->end()) {
            LOGW("repeat invite %d %d", task->cid, task->did);
            break;
        }
        if (args->senders->size() >= args->invite_count) {
            LOGE("over max invite count %d", args->invite_count);
            break;
        }

        strcpy(sender.server_ip, task->remote_ip);
        sender.server_port = task->remote_port;
        sender.local_port = task->local_port;
        if (task->transport == ua_transport::UDP) {
            sender.transport = PROTO_UDP;
        } else {
            sender.transport = PROTO_TCP;
        }
        sender.ssrc = atoi(task->y);
        sender.payload_type = task->pl_type;

        // 根据cid打开对应的sender
        ret = gb_muxer->initSender(&sender);
    } while (0);
    args->mtx_sender->unlock();

    if (ret != 0) {
        LOGE("init sender port %d failed", task->local_port);
        goto err;
    }

    ret = onCallbackStatus(EVENT_START_VIDEO, 0, NULL, 0);
    if (ret != 0) {
        goto err;
    }

    args->mtx_sender->lock();
    (*args->senders)[task->cid] = sender;
    args->mtx_sender->unlock();

    {
        std::lock_guard<std::mutex> lk(gb_mtx);
        ret = gb_muxer->startMuxer();
    }
    if (ret == 0) {
        return 0;
    }

err:
    onCallbackStatus(EVENT_STOP_VIDEO, 0, NULL, 0);
    return ret;
}

int startVoice(const task_t* task)
{
    int ret = -1;

    LOGI("start voice, tid:%d", gettid());
    ret = onCallbackStatus(EVENT_START_VOICE, task->cid, NULL, 0);
    if (ret != 0) {
        LOGE("start voice failed ret %d", ret);
        goto err;
    }

    {
        std::lock_guard<std::mutex> lk(gb_dmtx);
        ret = gb_demuxer->startVoice(task->cid, task->pl_type, task->transport == ua_transport::UDP,
                                     task->remote_ip, task->remote_port, task->local_ip, task->local_port);
    }
    LOGI("start voice ret %d", ret);
    if (ret == 0) {
        return 0;
    }

err:
    onCallbackStatus(EVENT_STOP_VOICE, task->cid, NULL, 0);
    return ret;
}

int stopStream(int cid)
{
    int ret = -1, size;
    UserArgs* args = gb_muxer->getArgs();

    args->mtx_sender->lock();
    // 根据cid关闭对应的sender,如果都关闭了则endMuxer
    auto it = args->senders->find(cid);
    if (it != args->senders->end()) {
        gb_muxer->endSender(&it->second);
        args->senders->erase(it);
    }
    size = args->senders->size();
    args->mtx_sender->unlock();

    LOGI("remaining senders %d", size);
    if (size == 0) {
        LOGI("invite cancel to stop video start");
        ret = onCallbackStatus(EVENT_STOP_VIDEO, 0, NULL, 0);
        LOGI("invite cancel to stop video end ret %d", ret);
    }
    return ret;
}

int stopVoice(int cid)
{
    int ret = -1;

    {
        std::lock_guard<std::mutex> lk(gb_dmtx);
        ret = gb_demuxer->endVoice(cid);
    }

    LOGI("stop voice start");
    ret = onCallbackStatus(EVENT_STOP_VOICE, cid, NULL, 0);
    LOGI("stop voice end ret %d", ret);
    return ret;
}

/**
 * 回调函数通知外部
 *  如果是取流则 initMuxer (cpp) / startMuxer (java)
 *  如果是语音则 startVoice
 * @param task
 * @return 0成功
 */
int onCallbackInvite(const task_t* task)
{
    int ret = -1;

    if (gb_muxer == NULL) {
        return -1;
    }
    UserArgs* args = gb_muxer->getArgs();
    if (args == NULL) {
        return -1;
    }
    if (task == NULL) {
        return -1;
    }

    LOGI("invite type %d start cid:%d, did:%d, tid:%d", task->tk_type, task->cid, task->did, gettid());

    if (task->tk_type == task_type::TT_STREAM) {
        ret = startStream(task);
    } else if (task->tk_type == task_type::TT_VOICE) {
        ret = startVoice(task);
    }
    return ret;
}

int onCallbackCancel(const task_t* task)
{
    int ret = -1, size = 0;
    LOGI("invite cancel %d cid:%d, did:%d, tid:%d", task->tk_type, task->cid, task->did, gettid());
    if (gb_muxer == NULL) {
        return -1;
    }
    UserArgs* args = gb_muxer->getArgs();
    if (args == NULL) {
        return -1;
    }
    if (task == NULL) {
        return -1;
    }

    if (task->tk_type == task_type::TT_STREAM) {
        ret = stopStream(task->cid);
    } else if (task->tk_type == task_type::TT_VOICE) {
        ret = stopVoice(task->cid);
    }
    return ret;
}

/**
 * 回调函数通知外部
 * @param start
 * @return
 */
int onCallbackRecord(int start)
{
    int ret = -1;
    LOGI("%s record, tid:%d", start ? "start" : "stop", gettid());
    if (gb_muxer == NULL) {
        return -1;
    }
    UserArgs* args = gb_muxer->getArgs();
    if (args == NULL) {
        return -1;
    }

    {
        std::lock_guard<std::mutex> lk(gb_mtx);
        ret = start ? gb_muxer->startRecord() : gb_muxer->stopRecord();
    }

    ret = onCallbackStatus(start ? EVENT_START_RECORD : EVENT_STOP_RECORD, 0, NULL, 0);
    LOGI("%s record ret %d", start ? "start" : "stop", ret);
    return ret;
}

int onCallbackGmSignature(int start)
{
    if (gb_muxer == NULL) {
        return -1;
    }
    return 0;
}

int onCallbackGmEncrypt(int start)
{
    if (gb_muxer == NULL) {
        return -1;
    }
    return 0;
}


extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_nan_gbd_library_JNIBridge_argbIntToNV21Byte(JNIEnv *env, jclass type, jintArray ints,
                                                     jint width, jint height) {
    int frame_size = width * height;
    jint *argb = env->GetIntArrayElements(ints, NULL);
    int ret_len = frame_size * 3 / 2;
    jbyte *yuv420sp = (jbyte *) malloc(ret_len + 1 * sizeof(jbyte));
    int y_index = 0;
    int uv_index = frame_size;
    int a, R, G, B, Y, U, V;
    int index = 0;
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            a = (argb[index] & 0xff000000) >> 24; // a is not used obviously
            R = (argb[index] & 0xff0000) >> 16;
            G = (argb[index] & 0xff00) >> 8;
            B = argb[index] & 0xff;

            Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
            U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
            V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;

            yuv420sp[y_index++] = (jbyte) ((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));
            if (j % 2 == 0 && index % 2 == 0) {
                yuv420sp[uv_index++] = (jbyte) ((V < 0) ? 0 : ((V > 255) ? 255 : V));
                yuv420sp[uv_index++] = (jbyte) ((U < 0) ? 0 : ((U > 255) ? 255 : U));
            }
            index++;
        }
    }
    env->ReleaseIntArrayElements(ints, argb, JNI_ABORT);
    jbyteArray ret = env->NewByteArray(ret_len);
    env->SetByteArrayRegion(ret, 0, ret_len, yuv420sp);

    free(yuv420sp);
    return ret;
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_nan_gbd_library_JNIBridge_argbIntToNV12Byte(JNIEnv *env, jclass type, jintArray ints,
                                                     jint width, jint height) {
    int frame_size = width * height;
    jint *argb = env->GetIntArrayElements(ints, NULL);
    int res_len = frame_size * 3 / 2;
    jbyte *yuv420sp = (jbyte *) malloc(res_len + 1 * sizeof(jbyte));
    int y_index = 0;
    int uv_index = frame_size;
    int a, R, G, B, Y, U, V;
    int index = 0;
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            a = (argb[index] & 0xff000000) >> 24; // a is not used obviously
            R = (argb[index] & 0xff0000) >> 16;
            G = (argb[index] & 0xff00) >> 8;
            B = argb[index] & 0xff;

            Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
            U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
            V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;

            yuv420sp[y_index++] = (jbyte) ((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));
            if (j % 2 == 0 && index % 2 == 0) {
                yuv420sp[uv_index++] = (jbyte) ((U < 0) ? 0 : ((U > 255) ? 255 : U));
                yuv420sp[uv_index++] = (jbyte) ((V < 0) ? 0 : ((V > 255) ? 255 : V));
            }
            index++;
        }
    }
    env->ReleaseIntArrayElements(ints, argb, JNI_ABORT);
    jbyteArray ret = env->NewByteArray(res_len);
    env->SetByteArrayRegion(ret, 0, res_len, yuv420sp);

    free(yuv420sp);
    return ret;
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_nan_gbd_library_JNIBridge_argbIntToGrayNVByte(JNIEnv *env, jclass type,
                                                       jintArray ints, jint width, jint height) {
    int frame_size = width * height;
    jint *argb = env->GetIntArrayElements(ints, NULL);
    int ret_len = frame_size;
    jbyte *yuv420sp = (jbyte *) malloc(ret_len + 1 * sizeof(jbyte));
    int y_index = 0;
    int R, G, B, Y;
    int index = 0;
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            R = (argb[index] & 0xff0000) >> 16;
            G = (argb[index] & 0xff00) >> 8;
            B = argb[index] & 0xff;

            Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;

            yuv420sp[y_index++] = (jbyte) ((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));
            index++;
        }
    }
    env->ReleaseIntArrayElements(ints, argb, JNI_ABORT);
    jbyteArray ret = env->NewByteArray(ret_len);
    env->SetByteArrayRegion(ret, 0, ret_len, yuv420sp);

    free(yuv420sp);
    return ret;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_nan_gbd_library_JNIBridge_nv21ToNv12(JNIEnv *env, jclass type, jbyteArray nv21Src,
                                              jbyteArray nv12Dest, jint width, jint height) {
    jbyte *nv21_src = env->GetByteArrayElements(nv21Src, NULL);
    jbyte *nv12_dest = env->GetByteArrayElements(nv12Dest, NULL);

    int frame_size = width * height;
    int end = frame_size + frame_size / 2;
    memcpy(nv21_src, nv12_dest, frame_size);

    for (int j = frame_size; j < end; j += 2) { //u
        nv12_dest[j] = nv21_src[j + 1];
        nv12_dest[j + 1] = nv21_src[j];
    }
    env->ReleaseByteArrayElements(nv21Src, nv21_src, 0);
    env->ReleaseByteArrayElements(nv12Dest, nv12_dest, 0);
}