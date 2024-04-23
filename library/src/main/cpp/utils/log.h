//
// Created by nanuns on 2018/9/11.
//

#ifndef NANGBD_LOG_H
#define NANGBD_LOG_H

#include <android/log.h>

#define TAG "nangbd"
#define DEBUG false
#define LOGD(...) if(DEBUG){__android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__);}
#define LOGI(...) if(true){__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__);}
#define LOGW(...) if(true){__android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__);}
#define LOGE(...) if(true){__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__);}

#endif // NANGBD_LOG_H
