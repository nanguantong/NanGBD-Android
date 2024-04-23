//
// Created by nanuns on 2018/6/10.
//

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <map>
#include <vector>
#include <list>
#include <osip2/osip_mt.h>
#include <eXosip2/eXosip.h>
#include "../utils/util.h"
#include "../utils/log.h"
#include "gm_ssl.h"
#include "sip_ua.h"

#define STR_0D0A "\r\n"

#define UA_CMD_REGISTER      ('1')
#define UA_CMD_CALL_START    ('2')
#define UA_CMD_CALL_ANSWER   ('3')
#define UA_CMD_CALL_KEEP     ('4')
#define UA_CMD_CALL_STOP     ('5')
#define UA_CMD_UNREGISTER    ('6')

#define BUFFER_LEN  1024
#define DEVICE_DESC_LEN 128
#define DEVICE_ID_LEN 20
#define BODY_LEN 4096

#define REQUEST 1
#define RESPONSE 0

#define REG_INIT 0
#define REG_OK 1
#define REG_PROCESSING 2
#define REG_FAILED -1

#define SIP_TYPE_XML "Application/MANSCDP+xml"

// 订阅类型
typedef enum {
    SUBSCRIBE_CATALOG,
    SUBSCRIBE_MOBILEPOS,
    SUBSCRIBE_ALARM
} subscribe_type;

typedef struct subscribe_s {
    subscribe_type type; // 订阅类型
    pthread_t thread;    // 订阅线程
    int expiry;          // 订阅有效期
    int did;             // 订阅did
    int interval;        // 通知时间间隔(秒)
    uint32_t sn;         // 通知序列号
} subscribe_t;

// 设备目录
typedef struct catalog_s {
    subscribe_t subscribe;
} catalog_t;

// 移动位置
typedef struct mobilepos_s {
    subscribe_t subscribe;
    std::atomic<ua_mobilepos_t> pos; // 移动位置信息
} mobilepos_t;

// 报警
typedef struct alarm_s {
    subscribe_t subscribe;
    bool reset;
	alarm_duty_status duty_status;
    ua_alarm_query_t query;
    std::list<ua_alarm_t> *alarms; // 报警信息
    std::mutex lock;
} alarm_t;

typedef struct broadcast {
    ua_transport transport = ua_transport::UDP; // TODO
    osip_call_id_t *msg_cid;
    char src_device_id[32];
} broadcast_t;

typedef struct ua_core {
    /* config */
    ua_config_t *config_ua;
    char *proxy;
    char *outboundproxy;
    char *nonce;
    char *from;
    char *to;
    char *contact;
    char *localip;
    char *firewallip;

    /* dynamic */
    struct eXosip_t *context;
    pthread_t ua_thread_event;
    pthread_t ua_thread_keepalive;
    std::atomic_int running;
    std::atomic_int registed;
    std::atomic_int config_changed;
    volatile int keepalive_retries;
    osip_call_id_t *keepalive_cid;
    int regid;
    int cmd;
    gm_t gm;

    std::mutex lock;
    std::condition_variable reg_cond;
    std::condition_variable unreg_cond;

    int free_port;
    std::map<int, task_t> *tasks; // 包括取流与语音任务

    catalog_t catalog; // 设备目录
    mobilepos_t mobilepos; // 移动位置
    alarm_t alarm; // 报警信息

    std::vector<broadcast_t> *voices;

    int is_recording; // 是否正在录制
    char *rec_path;   // 录像目录
} uacore_t;

static uacore_t g_core;
static ua_config_t* g_config_ua = NULL;
static gm_t* g_gm = NULL;
static uint32_t g_sn = 0;
static int g_ssrc_index = 1;

/*UA Functions*/
int   ua_add_outboundproxy(osip_message_t *msg, const char *outboundproxy);
int   ua_add_auth_info(uacore_t *core);
int   ua_register(uacore_t *core);
int   ua_cmd_register(uacore_t *core);
int   ua_cmd_unregister(uacore_t *core, int need_lock = 1, int sync = 1);
int   ua_cmd_call_start(uacore_t *core, const char *from, const char *to, const char *route,
                        const char *subject, const char *sdp, osip_message_t **msg);
int   ua_cmd_call_ring(uacore_t *core, task_t *task);
int   ua_cmd_call_answer(uacore_t *core, task_t *task, const char *body);
int   ua_cmd_call_keep(uacore_t *core, task_t* task);
int   ua_cmd_call_stop(uacore_t *core, task_t *task);
void  ua_check_task(uacore_t *core, int64_t cur_ts);
task_t* ua_notify_callid(uacore_t *core, eXosip_event_t *je, bool inited = false, bool voice = false);
void  ua_release_call(uacore_t *core, eXosip_event_t* je);
void  ua_release_broadcast(uacore_t *core, broadcast_t* broadcast);
int   ua_msg_send_response(uacore_t *core, eXosip_event_t* je, char* body = NULL, int code = 200);
int   ua_notify_call_ack(uacore_t *core, eXosip_event_t *je);
int   ua_notify_call_keep(uacore_t *core, eXosip_event_t *je);
int   ua_notify_keepalive(uacore_t *core);
int   ua_notify_subscribe(uacore_t *core, subscribe_type type, subscribe_t *subscribe);
int   ua_notify_media_status(uacore_t *core);
void* ua_event_thread(void *arg);
void* ua_keepalive_thread(void *arg);
void* ua_catalog_thread(void *arg);
void* ua_pos_thread(void *arg);
void* ua_alarm_thread(void *arg);
void* ua_subscribe_notify(void *arg, subscribe_type type);
int   ua_printf_msg(eXosip_event_t* je, int ch);
/*GB28181 UAC Functions*/
int   ua_device_response(char* cmd_type, char* sn, char* device_id, char* body);
int   ua_catalog_response(char* sn, char* device_id, char* body);
int   ua_device_info_response(char* sn, char* device_id, char* body);
int   ua_device_config_response(char* sn, char* device_id, char* body);
int   ua_device_status_response(char* sn, char* device_id, char* body);
int   ua_device_record_info_response(char* sn, char* device_id, const record_info_t* rec_query, char* body);
int   ua_subscribe_response(uacore_t *core, eXosip_event_t *je, char* body, int code = 200);
int   ua_subscribe_catalog_request(uint32_t sn, char* device_id, char* body);
int   ua_subscribe_mobilepos_request(uint32_t sn, char* device_id, char* body);
int   ua_subscribe_alarm_request(uint32_t sn, char* device_id, char* body);
int   ua_gm_add_note_header(uacore_t *core, const char* method, osip_message_t* msg);
int   ua_gm_gen_note(const char* method, osip_message_t* msg);
int   ua_gm_sign_sign1(const char *username, const char *random1, const char *random2, const char *serverid, char *sign1);
int   ua_gm_verify_sign2(const char *username, const char *random1, const char *random2, const char *cryptkey, const char *sign2);

/*Util Functions*/
int   get_str(const char* data, const char* s_mark, bool with_s_make,
              const char* e_mark, bool with_e_make, char* dest, int* dest_len = NULL);
int   get_free_port(uacore_t *core, int transport);
void  get_streamid(char id[], int size);

int   init(uacore_t *core);
int   quit(uacore_t *core);

static void quit_handler(int signum) {
    LOGD("quitting %d", signum);
}

static const char* get_subscribe_type_name(subscribe_type type)
{
    switch (type) {
        case SUBSCRIBE_CATALOG:   return "catalog";
        case SUBSCRIBE_MOBILEPOS: return "mobilepos";
        case SUBSCRIBE_ALARM:     return "alarm";
    }
    return NULL;
}

static record_type get_record_type(const char *type)
{
    if (osip_strcasecmp(type, "all") == 0) {
        return REC_TYPE_ALL;
    } else if (osip_strcasecmp(type, "time") == 0) {
        return REC_TYPE_TIME;
    } else if (osip_strcasecmp(type, "alarm") == 0) {
        return REC_TYPE_ALARM;
    } else if (osip_strcasecmp(type, "manual") == 0) {
        return REC_TYPE_MANUAL;
    }
    return REC_TYPE_ALL;
}

static const char* get_record_type_name(record_type type)
{
    switch (type) {
        case REC_TYPE_ALL:    return "all";
        case REC_TYPE_TIME:   return "time";
        case REC_TYPE_ALARM:  return "alarm";
        case REC_TYPE_MANUAL: return "manual";
    }
    return "all";
}

static const char* get_duty_status_name(alarm_duty_status status)
{
    switch (status) {
        case ALARM_STATUS_ONDUTY:  return "ONDUTY";
        case ALARM_STATUS_OFFDUTY: return "OFFDUTY";
        case ALARM_STATUS_ALARM:   return "ALARM";
    }
    return "";
}

int init(uacore_t *core)
{
    memset(core, 0, sizeof(uacore_t));
    core->config_ua = (ua_config_t*)calloc(1, sizeof(ua_config_t));
    core->config_ua->server_ip = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->config_ua->server_id = (char*)calloc(1, sizeof(char) * DEVICE_ID_LEN);
    core->config_ua->server_domain = (char*)calloc(1, sizeof(char) * DEVICE_ID_LEN);
    core->config_ua->name = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->config_ua->manufacturer = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->config_ua->model = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->config_ua->firmware = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->config_ua->serial_number = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->config_ua->username = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->config_ua->userid = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->config_ua->password = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->config_ua->protocol = (char*)calloc(1, sizeof(char) * 6);
    core->config_ua->video_id = (char*)calloc(1, sizeof(char) * DEVICE_ID_LEN);
    core->config_ua->audio_id = (char*)calloc(1, sizeof(char) * DEVICE_ID_LEN);
    core->config_ua->alarm_id = (char*)calloc(1, sizeof(char) * DEVICE_ID_LEN);
    core->config_ua->charset = (char*)calloc(1, sizeof(char) * 10);

    core->proxy = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->outboundproxy = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->nonce = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->from = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->to = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->contact = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->localip = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->firewallip = (char*)calloc(1, sizeof(char) * DEVICE_DESC_LEN);
    core->free_port = DEFAULT_MIN_PORT;
    core->tasks = new std::map<int, task_t>;
    core->voices = new std::vector<broadcast_t>;
    core->alarm.alarms = new std::list<ua_alarm_t>;
    return 0;
}

int quit(uacore_t *core)
{
    if (NULL != core->config_ua) {
        ua_config_t* config = core->config_ua;
        if (NULL != config->server_ip) { free(config->server_ip); config->server_ip = NULL; }
        if (NULL != config->server_id) { free(config->server_id); config->server_id = NULL; }
        if (NULL != config->server_domain) { free(config->server_domain); config->server_domain = NULL; }
        if (NULL != config->name) { free(config->name); config->name = NULL; }
        if (NULL != config->manufacturer) { free(config->manufacturer); config->manufacturer = NULL; }
        if (NULL != config->model) { free(config->model); config->model = NULL; }
        if (NULL != config->firmware) { free(config->firmware); config->firmware = NULL; }
        if (NULL != config->serial_number) { free(config->serial_number); config->serial_number = NULL; }
        if (NULL != config->username) { free(config->username); config->username = NULL; }
        if (NULL != config->userid) { free(config->userid); config->userid = NULL; }
        if (NULL != config->password) { free(config->password); config->password = NULL; }
        if (NULL != config->protocol) { free(config->protocol); config->protocol = NULL; }
        if (NULL != config->video_id) { free(config->video_id); config->video_id = NULL; }
        if (NULL != config->audio_id) { free(config->audio_id); config->audio_id = NULL; }
        if (NULL != config->alarm_id) { free(config->alarm_id); config->alarm_id = NULL; }
        if (NULL != config->charset) { free(config->charset); config->charset = NULL; }
        free(core->config_ua); core->config_ua = NULL;
    }
    if (NULL != core->proxy) { free(core->proxy); core->proxy = NULL; }
    if (NULL != core->outboundproxy) { free(core->outboundproxy); core->outboundproxy = NULL; }
    if (NULL != core->nonce) { free(core->nonce); core->nonce = NULL; }
    if (NULL != core->from) { free(core->from); core->from = NULL; }
    if (NULL != core->to) { free(core->to); core->to = NULL; }
    if (NULL != core->contact) { free(core->contact); core->contact = NULL; }
    if (NULL != core->localip) { free(core->localip); core->localip = NULL; }
    if (NULL != core->firewallip) { free(core->firewallip); core->firewallip = NULL; }
    if (NULL != core->keepalive_cid) { osip_call_id_free(core->keepalive_cid); core->keepalive_cid = NULL; }
    if (NULL != core->tasks) {
        for (auto &it : *core->tasks) {
            task_t *task = &it.second;
            if (NULL != task->opaque) {
                broadcast_t* broadcast = (broadcast_t*) task->opaque;
                ua_release_broadcast(core, broadcast);
            }
        }
        delete(core->tasks); core->tasks = NULL;
    }
    if (NULL != core->voices) { delete(core->voices); core->voices = NULL; }
    if (NULL != core->alarm.alarms) { delete(core->alarm.alarms); core->alarm.alarms = NULL; }

    if (NULL != core->gm.device_key_pass) { free(core->gm.device_key_pass); core->gm.device_key_pass = NULL; }
    if (NULL != core->gm.device_ec_prikey) {
        EC_KEY_free(core->gm.device_ec_prikey);
        core->gm.device_ec_prikey = NULL;
    }
    if (NULL != core->gm.server_pkey) {
        //EC_KEY_free(core->gm.server_ec_pubkey);
        //core->gm.server_ec_pubkey = NULL;
        EVP_PKEY_free(core->gm.server_pkey);
        core->gm.server_pkey = NULL;
    }
    if (NULL != core->gm.server_cert) {
        X509_free(core->gm.server_cert);
        core->gm.server_cert = NULL;
    }

    if (NULL != core->context) {
        eXosip_quit(core->context);
        osip_free(core->context);
        core->context = NULL;
    }

    if (core->is_recording) {
        core->is_recording = 0;
        if (core->config_ua->on_callback_record != NULL) {
            core->config_ua->on_callback_record(0);
        }
    }
    if (NULL != core->rec_path) {
        free(core->rec_path);
        core->rec_path = NULL;
    }

    return 0;
}

int ua_printf_msg(eXosip_event_t* je, int ch)
{
    char *dest = NULL;
    size_t len = 0;
    int ret = 0;
    if (ch == REQUEST) {
        ret = osip_message_to_str(je->request, &dest, &len);
    }
    else if (ch == RESPONSE) {
        ret = osip_message_to_str(je->response, &dest, &len);
    }
    if (ret != 0) {
        LOGE("%s msg(%d) error %d", ch == REQUEST ? "REQUEST" : "RESPONSE", je->type, ret);
        return -1;
    }
    LOGD("===%s: %s\n", ch == REQUEST ? "REQUEST" : "RESPONSE", dest);

    osip_free(dest);
    return 0;
}

/***************************** command *****************************/
int ua_add_outboundproxy(osip_message_t *msg, const char *outboundproxy)
{
    int ret = 0;
    char head[BUFFER_LEN] = { 0 };

    if (NULL == null_if_empty(outboundproxy)) {
        return -1;
    }
    snprintf(head, sizeof(head)-1, "<%s;lr>", outboundproxy);

    osip_list_special_free(&msg->routes, reinterpret_cast<void (*)(void *)>(osip_route_free));
    ret = osip_message_set_route(msg, head);
    return ret;
}

int ua_add_auth_info(uacore_t *core)
{
    int ret = -1;
    if (NULL == core || NULL == core->config_ua) {
        return ret;
    }
    eXosip_clear_authentication_info(core->context);
    if (core->config_ua->gm_level > 0) {
        ret = eXosip_add_authentication_info_withgm(core->context, core->config_ua->username,
                null_if_empty(core->config_ua->userid) ?
                core->config_ua->userid : core->config_ua->username,
                core->config_ua->password, NULL, NULL,
                core->config_ua->server_id, ua_gm_sign_sign1, ua_gm_gen_note);
    } else {
        ret = eXosip_add_authentication_info(core->context, core->config_ua->username,
                null_if_empty(core->config_ua->userid) ?
                core->config_ua->userid : core->config_ua->username,
                core->config_ua->password, NULL /*MD5*/, NULL);
    }
    if (0 != ret) {
        LOGW("add auth info failed, %d", ret);
        return ret;
    }
    return 0;
}

int ua_register(uacore_t *core)
{
    int ret = -1;
    osip_message_t *msg = NULL;

    do {
        if (core->regid > 0) { // refresh register
            ret = eXosip_register_build_register(core->context, core->regid, core->config_ua->expiry, &msg);
            if (0 != ret) {
                LOGE("register %d refresh build failed %d", core->regid, ret);
                break;
            }
        } else { // new register
            core->regid = eXosip_register_build_initial_register(core->context, core->from, core->proxy,
                                                                 core->contact, core->config_ua->expiry, &msg);
            if (core->regid <= 0) {
                LOGE("register build failed %d", core->regid);
                ret = -1;
                break;
            }
            if (core->config_ua->gm_level > 0) {
                static char time_buf[24];
                char auth[256] = {0};
                snprintf(auth, sizeof(auth), R"(Capability algorithm="%s", keyversion="%s")",
                         GM_ALGORITHM, get_time_with_t(time_buf, 24));
                osip_message_set_header(msg, "Authorization", auth);
            }
            ua_add_outboundproxy(msg, core->outboundproxy);
        }
        ret = eXosip_register_send_register(core->context, core->regid, msg);
        if (0 != ret) {
            LOGE("register %d send failed", core->regid);
            break;
        }
    } while (0);

    return ret;
}

int ua_cmd_register(uacore_t *core)
{
    int ret = -1;
    osip_message_t *msg = NULL;

    if (core->registed == REG_OK) {
        LOGI("registered");
        return 0;
    }
    if (core->registed == REG_PROCESSING) {
        LOGW("is registering");
        return 0;
    }
    core->registed = REG_PROCESSING;

    LOGI("registering");
    if (core->config_ua->on_callback_status != NULL) {
        core->config_ua->on_callback_status(EVENT_CONNECTING, 0, NULL, 0);
    }

    eXosip_lock(core->context);
    ret = ua_register(core);
    eXosip_unlock(core->context);

    if (0 != ret) {
        goto clear;
    }

    LOGI("send register %d", core->regid);
    if (core->config_ua->on_callback_status != NULL) {
        core->config_ua->on_callback_status(EVENT_REGISTERING, 0, NULL, 0);
    }

    {
        std::unique_lock<std::mutex> lock(core->lock);
        cv_status status = core->reg_cond.wait_for(lock, std::chrono::milliseconds(15000));
        if (status == cv_status::timeout) {
            LOGW("register timeout");
            ret = -1;
            goto clear;
        }
    }

    if (0 == ret) {
        // 防止注册响应多次且未完成时，同时调用注销，造成OSIP_WRONG_STATE
        //osip_usleep(1000000); // TODO: delete
        if (core->regid > 0 && core->registed != REG_FAILED) {
            core->registed = REG_OK;
            LOGI("registered");
        } else {
            ret = -1;
            goto clear;
        }
    }

    return 0;

clear:
    eXosip_lock(core->context);
    if (core->regid > 0) {
        eXosip_register_remove(core->context, core->regid);
    }
    core->registed = REG_INIT;
    core->regid = 0;
    eXosip_unlock(core->context);
    if (core->config_ua->on_callback_status != NULL) {
        core->config_ua->on_callback_status(EVENT_REGISTER_AUTH_FAIL, 0, NULL, 0);
    }
    return ret;
}

int ua_cmd_unregister(uacore_t *core, int need_lock, int sync)
{
    int ret = -1;
    osip_message_t *msg = NULL;
    int expiry = 0; //unregister
    int regid = core->regid;

    LOGI("unregister %d", regid);
    if (!core->registed) {
        goto clear;
    }
    if (core->registed == REG_PROCESSING) {
        LOGW("unregister while registering");
    }
    core->registed = REG_INIT;
    core->regid = 0;

    if (need_lock) {
        eXosip_lock(core->context);
    }
    do {
        if (!core->config_ua->global_auth || core->config_ua->gm_level > 0) {
            // 35114必须清除,否则注册会复用存在的authentication里面的二次注册信息
            eXosip_clear_authentication_info(core->context);
        }
        ret = eXosip_register_build_register(core->context, regid, expiry, &msg);
        if (0 != ret) {
            LOGE("unregister %d build failed %d", regid, ret);
            ret = -1;
            break;
        }
        if (core->config_ua->gm_level > 0) {
            static char time_buf[24];
            char auth[256] = {0};
            snprintf(auth, sizeof(auth), R"(Capability algorithm="%s", keyversion="%s")",
                     GM_ALGORITHM, get_time_with_t(time_buf, 24));
            osip_message_set_header(msg, "Authorization", auth);
        }
        ret = eXosip_register_send_register(core->context, regid, msg);
        if (0 != ret) {
            LOGE("register %d send failed %d", regid, ret);
            break;
        }
        if (!core->config_ua->global_auth || core->config_ua->gm_level > 0) {
            ua_add_auth_info(core);
        }
    } while (0);
    if (need_lock) {
        eXosip_unlock(core->context);
    }

    if (0 != ret) {
        goto clear;
    }

    if (sync) {
        std::unique_lock<std::mutex> lock(core->lock);
        cv_status status = core->unreg_cond.wait_for(lock, std::chrono::milliseconds(15000));
        if (status == cv_status::timeout) {
            LOGI("unregister timeout");
            ret = -1;
        }
    }

clear:
    if (NULL != core->context) {
        if (need_lock) {
            eXosip_lock(core->context);
        }
        if (regid > 0) {
            eXosip_register_remove(core->context, regid);
        }
    }
    core->registed = REG_INIT;
    core->regid = 0;
    core->keepalive_retries = 0;
    // close calls
    if (NULL != core->tasks) {
        for (auto &it : *core->tasks) {
            task_t *task = &it.second;
            if (NULL != task->opaque) {
                broadcast_t* broadcast = (broadcast_t*) task->opaque;
                ua_release_broadcast(core, broadcast);
            }
            ua_cmd_call_stop(core, task);
            if (core->config_ua->on_callback_cancel != NULL) {
                core->config_ua->on_callback_cancel(task);
            }
        }
        core->tasks->clear();
    }
    if (need_lock && NULL != core->context) {
        eXosip_unlock(core->context);
    }

    LOGI("unregistered");
    return ret;
}

// 返回cid
int ua_cmd_call_start(uacore_t *core, const char *from, const char *to, const char *route,
                      const char *subject, const char *sdp, osip_message_t **msg) {
    int ret = 0;

    if (core == NULL) {
        return -1;
    }

    ret = eXosip_call_build_initial_invite(core->context, msg, to, from, route, subject);
    if (0 != ret) {
        LOGE("call from %s to %s build invite failed %d", from, to, ret);
        return ret;
    }
    if (sdp != NULL) {
        ret = osip_message_set_body(*msg, sdp, strlen(sdp));
        if (0 != ret) {
            LOGE("call from %s to %s set body failed %d", from, to, ret);
            return ret;
        }
        osip_message_set_content_type(*msg, "application/sdp");
    }
    ////ua_gm_add_note_header(core, "INVITE", *msg);
    ua_add_outboundproxy(*msg, core->outboundproxy);
    ret = eXosip_call_send_initial_invite(core->context, *msg);
    if (ret < 0) {
        LOGE("call from %s to %s send invite failed %d", from, to, ret);
        return ret;
    }
    return ret;
}

int ua_cmd_call_ring(uacore_t *core, task_t* task)
{
    int ret = 0;
    int code = 180;
    osip_message_t *msg = NULL;

    ret = eXosip_call_build_answer(core->context, task->tid, code, &msg);
    if (0 != ret) {
        LOGE("call %d %d build ring failed %d", task->cid, task->did, ret);
        return ret;
    }
    ////ua_gm_add_note_header(core, "INVITE", msg);
    ret = eXosip_call_send_answer(core->context, task->tid, code, msg);
    if (0 != ret) {
        LOGE("call %d %d send ring failed %d", task->cid, task->did, ret);
        return ret;
    }
    return ret;
}

int ua_cmd_call_answer(uacore_t *core, task_t* task, const char *body)
{
    int ret = -1;
    int body_len = 0;
    int code;
    osip_message_t *msg = NULL;

    if (core == NULL || task == NULL) {
        return -1;
    }

    if (NULL != body) body_len = strlen(body);
    code = (body_len > 0 ? 200 : 486);

    ret = eXosip_call_build_answer(core->context, task->tid, code, &msg);
    if (0 != ret) {
        LOGE("call %d %d build answer failed %d", task->cid, task->did, ret);
        return ret;
    }

    /* UAS call timeout */
    osip_message_set_supported(msg, "timer");
    if (body_len > 0) {
        ret = osip_message_set_body(msg, body, body_len);
        if (0 != ret) {
            LOGE("call %d %d set body failed %d", task->cid, task->did, ret);
            return ret;
        }
        osip_message_set_content_type(msg, "application/sdp");
    }
    ////ua_gm_add_note_header(core, "INVITE", msg);
    ret = eXosip_call_send_answer(core->context, task->tid, code, msg);
    if (0 != ret) {
        LOGE("call %d %d send answer failed %d", task->cid, task->did, ret);
        return ret;
    }
    return ret;
}

int ua_cmd_call_keep(uacore_t *core, task_t* task)
{
    int ret = -1;
    osip_message_t *msg = NULL;

    ret = eXosip_call_build_request(core->context, task->did, "INVITE", &msg);
    if (0 != ret) {
        LOGE("call %d %d build keep failed %d", task->cid, task->did, ret);
        return ret;
    }

    ret = eXosip_call_send_request(core->context, task->did, msg);
    if (0 != ret){
        LOGE("call %d %d send keep failed %d", task->cid, task->did, ret);
        return ret;
    }
    return ret;
}

int ua_cmd_call_stop(uacore_t *core, task_t* task)
{
    int ret;
    if (NULL == task) {
        return 0;
    }
    LOGI("call stop %d %d acked %d", task->cid, task->did, task->acked);
    if (!task->acked) {
        return 0;
    }
    task->acked = 0;
    ret = eXosip_call_terminate(core->context, task->cid, task->did);
    if (0 != ret) {
        LOGE("call %d %d send stop failed", task->cid, task->did);
    }
    return ret;
}


int ua_msg_send_response(uacore_t *core, eXosip_event_t* je, char* body, int code)
{
    int ret = 0;
    osip_message_t* msg = NULL;

    ret = eXosip_message_build_answer(core->context, je->tid, code, &msg);
    if (0 != ret) {
        LOGE("message %d build failed %d", je->tid, ret);
        return ret;
    }
    if (body && strlen(body) > 0) {
        ret = osip_message_set_body(msg, body, strlen(body));
        if (0 != ret) {
            LOGE("message %d set body failed %d", je->tid, ret);
            return ret;
        }
    }
    ////ua_gm_add_note_header(core, "MESSAGE", msg);
    ret = eXosip_message_send_answer(core->context, je->tid, code, msg);
    if (0 != ret) {
        LOGE("message %d send ring failed %d", je->tid, ret);
        return ret;
    }
    return ret;
}

/*****************************ua notify *****************************/
int ua_notify_call_ack(uacore_t *core, eXosip_event_t *je)
{
    int ret = 0;
    osip_message_t *msg = NULL;

    ret = eXosip_call_build_ack(core->context, je->tid, &msg);
    if (0 != ret) {
        LOGE("call %d build ack failed", je->cid);
        return ret;
    }
    ////ua_gm_add_note_header(core, "ACK", msg);
    ua_add_outboundproxy(msg, core->outboundproxy);
    ret = eXosip_call_send_ack(core->context, je->tid, msg);
    if (0 != ret) {
        LOGE("call %d send ack failed", je->cid);
        return ret;
    }
    return ret;
}

int ua_notify_call_keep(uacore_t *core, eXosip_event_t *je)
{
    int ret = 0;
    int code = 200;
    osip_message_t *msg = NULL;

    LOGI("call keep %d", je->cid);
    eXosip_call_build_answer(core->context, je->tid, code, &msg);
    if (NULL == msg) {
        LOGE("call %d build answer failed", je->cid);
        return ret;
    }
    ////ua_gm_add_note_header(core, "INVITE", msg);
    ret = eXosip_call_send_answer(core->context, je->tid, code, msg);
    if (0 != ret) {
        LOGE("call %d send answer failed", je->cid);
    }
    return ret;
}

int ua_notify_keepalive(uacore_t *core)
{
    int ret;
    osip_message_t* msg = NULL;
    char body[1024];
    memset(body, 0, sizeof(body));
    snprintf(body, sizeof(body),
            "<?xml version=\"1.0\"?>" STR_0D0A
            "<Notify>" STR_0D0A
            "<CmdType>Keepalive</CmdType>" STR_0D0A
            "<SN>%u</SN>" STR_0D0A
            "<DeviceID>%s</DeviceID>" STR_0D0A
            "<Status>OK</Status>" STR_0D0A
            "</Notify>" STR_0D0A, ++g_sn, core->config_ua->username);
    do {
        ret = eXosip_message_build_request(core->context, &msg, "MESSAGE", core->to, core->from, core->proxy);
        if (OSIP_SUCCESS != ret) {
            LOGE("build keepalive request failed %d", ret);
            break;
        }

        osip_call_id_free(core->keepalive_cid);
        osip_call_id_clone(msg->call_id, &core->keepalive_cid);
        LOGI("keepalive callid %s", msg->call_id->number);
        ret = osip_message_set_body(msg, body, strlen(body));
        if (OSIP_SUCCESS != ret) {
            LOGE("message keepalive set body failed %d", ret);
            break;
        }
        ret = osip_message_set_content_type(msg, SIP_TYPE_XML);
        ////ua_gm_add_note_header(core, "MESSAGE", msg);
        ua_add_outboundproxy(msg, core->outboundproxy);
        ret = eXosip_message_send_request(core->context, msg);
        if (ret > 0) {
            ret = OSIP_SUCCESS;
        } else {
            LOGE("message keepalive send failed %d", ret);
            ret = -1;
            break;
        }
    } while (0);
    return ret;
}

int ua_notify_subscribe(uacore_t *core, subscribe_type type, subscribe_t *subscribe)
{
    int ret;
    char body[BODY_LEN] = {0};
    osip_message_t *msg = NULL;
    osip_header_t *sub_state = NULL;
    char *ss_expires;

    if (SUBSCRIBE_ALARM == type) {
        std::unique_lock<std::mutex> lock(core->alarm.lock);
        if (NULL == core->alarm.alarms || core->alarm.alarms->empty()) {
            return 0;
        }
    }

    const char *type_name = get_subscribe_type_name(type);

    ret = eXosip_insubscription_build_notify(core->context, subscribe->did,
                                             EXOSIP_SUBCRSTATE_ACTIVE, DEACTIVATED, &msg);
    if (ret != 0) {
        LOGE("create notify for %s subscription %d failed, %d", type_name, subscribe->did, ret);
        return ret;
    }
    osip_message_header_get_byname(msg, "Subscription-State", 0, &sub_state);
    if (sub_state == NULL) {
        return -1;
    }
    // 注意: 在刷新订阅的时候 dialog->remote_contact_uri 可能变成了内网ip
    if (osip_strcasecmp(msg->req_uri->host, core->config_ua->server_ip) != 0) {
        LOGD("dialog req uri %s change to %s", msg->req_uri->host, core->config_ua->server_ip);
        // char *dest = NULL; osip_uri_to_str(msg->req_uri, &dest);
        osip_free(msg->req_uri->host);
        osip_uri_set_host(msg->req_uri, osip_strdup(core->config_ua->server_ip));
    }

    // Subscription-State active;expires=600
    ss_expires = strstr(sub_state->hvalue + 6, "expires=");
    if (ss_expires != NULL) {
        ss_expires += strlen("expires=");
        subscribe->expiry = osip_atoi(ss_expires);
        LOGD("subscribe %s notify %s %s %d", type_name, sub_state->hname, sub_state->hvalue, subscribe->expiry);
    }

    switch (type) {
        case SUBSCRIBE_CATALOG:
            ua_subscribe_catalog_request(subscribe->sn++, core->config_ua->username, body);
            break;
        case SUBSCRIBE_MOBILEPOS:
            ua_subscribe_mobilepos_request(subscribe->sn++, core->config_ua->username, body);
            break;
        case SUBSCRIBE_ALARM:
            ua_subscribe_alarm_request(subscribe->sn++, core->config_ua->username, body);
            break;
    }

    osip_message_set_body(msg, body, strlen(body));
    ////ua_gm_add_note_header(core, "NOTIFY", msg);
    ua_add_outboundproxy(msg, core->outboundproxy);
    ret = eXosip_insubscription_send_request(core->context, subscribe->did, msg);
    if (ret != 0) {
        LOGE("send %s subscription %d notify failed %d", type_name, subscribe->did, ret);
    }
    return ret;
}

int ua_notify_media_status(uacore_t *core)
{
    int ret;
    osip_message_t* msg = NULL;
    char body[1024];
    memset(body, 0, sizeof(body));
    snprintf(body, sizeof(body),
             "<?xml version=\"1.0\"?>" STR_0D0A
             "<Notify>" STR_0D0A
             "<CmdType>MediaStatus</CmdType>" STR_0D0A
             "<SN>%u</SN>" STR_0D0A
             "<DeviceID>%s</DeviceID>" STR_0D0A
             "<NotifyType>121</NotifyType>" STR_0D0A
             "</Notify>" STR_0D0A, ++g_sn, core->config_ua->username);
    do {
        ret = eXosip_message_build_request(core->context, &msg, "MESSAGE", core->to, core->from, core->proxy);
        if (OSIP_SUCCESS != ret) {
            LOGE("build media status request failed %d", ret);
            break;
        }

        ret = osip_message_set_body(msg, body, strlen(body));
        if (OSIP_SUCCESS != ret) {
            LOGE("message media status set body failed %d", ret);
            break;
        }
        ret = osip_message_set_content_type(msg, SIP_TYPE_XML);
        ////ua_gm_add_note_header(core, "MESSAGE", msg);
        ua_add_outboundproxy(msg, core->outboundproxy);
        ret = eXosip_message_send_request(core->context, msg);
        if (ret > 0) {
            ret = OSIP_SUCCESS;
        } else {
            LOGE("message media status send failed %d", ret);
            ret = -1;
            break;
        }
    } while (0);
    return ret;
}

static int64_t start_ts = 1681466400000; // 2023-04-14 18:00:00
//static int64_t use_duration = 2592000000L; // 30*24*60*60*1000L;
//static int64_t use_duration = 7776000000L; // 90*24*60*60*1000L;
//static int64_t use_duration = 15552000000L; // 180*24*60*60*1000L;
//static int64_t use_duration = 31536000000L; // 365*24*60*60*1000L;
static int64_t use_duration = 0;

// 检查过时的任务并清理
void ua_check_task(uacore_t *core, int64_t cur_ts)
{
    if (use_duration > 0 && cur_ts > start_ts + use_duration) return;
    for (auto it = core->tasks->begin(); it != core->tasks->end();) {
        task_t *t = &it->second;
        if (!t->acked && t->invite_time + 15000 < cur_ts) {
            LOGW("clear outdated invite %d %d", t->cid, t->did);
            if (t->tk_type == task_type::TT_VOICE) {
                broadcast_t* broadcast = (broadcast_t*) t->opaque;
                ua_release_broadcast(core, broadcast);
            }
            core->tasks->erase(it++);
        } else {
            it++;
        }
    }
}

task_t* ua_notify_callid(uacore_t *core, eXosip_event_t *je, bool inited, bool voice)
{
    auto it = core->tasks->find(je->cid);
    if (it != core->tasks->end()) {
        return &it->second;
    }
    if (!inited) {
        return NULL;
    }

    int64_t cur_ts = get_cur_time();
    ua_check_task(core, cur_ts);

    if (voice) {
        if (core->voices->size() > core->config_ua->voice_count) {
            LOGW("only support %d concurrent voice", core->config_ua->voice_count);
            for (auto it = core->tasks->begin(); it != core->tasks->end(); it++) {
                task_t *t = &it->second;
                if (t->tk_type == task_type::TT_VOICE) {
                    LOGI("\texisted voice task %d %d acked %d", t->cid, t->did, t->acked);
                }
            }
            return NULL;
        }
    } else {
        if (core->tasks->size() >= core->config_ua->invite_count) {
            LOGW("only support %d concurrent invite", core->config_ua->invite_count);
            for (auto it = core->tasks->begin(); it != core->tasks->end(); it++) {
                task_t *t = &it->second;
                if (t->tk_type == task_type::TT_STREAM) {
                    LOGI("\texisted task %d %d acked %d", t->cid, t->did, t->acked);
                }
            }
            return NULL;
        }
    }

    task_t task;
    memset(&task, 0, sizeof(task_t));
    task.cid = je->cid;
    task.did = je->did;
    task.tid = je->tid;
    task.invite_time = cur_ts;
    LOGI("add task %d %d", task.cid, task.did);
    return &((*core->tasks)[task.cid] = task);
}

void ua_release_call(uacore_t *core, eXosip_event_t* je)
{
    if (je->cid < 0 /*|| je->did < 0*/) {
        return;
    }
    auto it = core->tasks->find(je->cid);
    if (it != core->tasks->end()) {
        task_t *task = &it->second;
        //ua_cmd_call_stop(core, task);
        if (task->tk_type == task_type::TT_VOICE) {
            broadcast_t* broadcast = (broadcast_t*) task->opaque;
            ua_release_broadcast(core, broadcast);
        }
        if (core->config_ua->on_callback_cancel != NULL) {
            core->config_ua->on_callback_cancel(task);
        }
        core->tasks->erase(it);
    }
}

void ua_release_broadcast(uacore_t *core, broadcast_t* broadcast)
{
    if (NULL == broadcast) {
        return;
    }
    if (NULL != broadcast->msg_cid) {
        LOGI("release broadcast %s@%s", broadcast->msg_cid->host, broadcast->msg_cid->number);
        for (auto it = core->voices->begin(); it != core->voices->end(); it++) {
            if (0 == osip_call_id_match(broadcast->msg_cid, it->msg_cid)) {
                LOGI("release broadcast voice ok");
                core->voices->erase(it);
                break;
            }
        }
        osip_call_id_free(broadcast->msg_cid);
        broadcast->msg_cid = NULL;
    }
}

/*****************************ua response *****************************/
int get_str(const char* data, const char* s_mark, bool with_s_make, const char* e_mark, bool with_e_make, char* dest, int* dest_len)
{
    const char* start = strstr(data, s_mark);
    if (start != NULL) {
        int s_mark_len = strlen(s_mark);
        const char* end = strstr(start + s_mark_len, e_mark);
        if (end != NULL) {
            int s_pos = with_s_make ? 0 : s_mark_len;
            int e_pos = with_e_make ? strlen(e_mark) : 0;
            int len = (end + e_pos) - (start + s_pos);
            if (NULL != dest_len) {
                *dest_len = len;
            }
            strncpy(dest, start + s_pos, len);
            dest[len] = '\0';
        }
        return 0;
    }
    return -1;
}

int get_free_port(uacore_t *core, int transport)
{
    if (core->free_port >= DEFAULT_MAX_PORT) {
        core->free_port = DEFAULT_MIN_PORT;
    }
    int port = eXosip_find_free_port(core->context, core->free_port, transport);
    if (port > 0) {
        core->free_port += 2;
    }
    return port;
}

void get_streamid(char id[], int size)
{
    if (g_ssrc_index == 9999) {
        g_ssrc_index = 1;
    }
    snprintf(id, size - 1, "%04d", g_ssrc_index++);
}

int ua_device_response(char* cmd_type, char* sn, char* device_id, char* body)
{
    snprintf(body, BODY_LEN, "<?xml version=\"1.0\"?>" STR_0D0A
                                "<Response>" STR_0D0A
                                "<CmdType>%s</CmdType>" STR_0D0A
                                "<SN>%s</SN>" STR_0D0A
                                "<DeviceID>%s</DeviceID>" STR_0D0A
                                "<Result>OK</Result>" STR_0D0A
                                "</Response>" STR_0D0A,
            cmd_type, sn, device_id);
    return 0;
}

int ua_catalog_response(char* sn, char* device_id, char* body)
{
    int device_num = 2;
    char alarm_item[1024]; alarm_item[0] = '\0';
    if (strlen(g_core.config_ua->alarm_id) > 0) {
        device_num += 1;

        sprintf(alarm_item,
                "<Item>" STR_0D0A
                "<DeviceID>%s</DeviceID>" STR_0D0A
                "<Name>Alarm</Name>" STR_0D0A
                "<Manufacturer>%s</Manufacturer>" STR_0D0A
                "<Parental>0</Parental>" STR_0D0A
                "<ParentID>%s</ParentID>" STR_0D0A
                "<SafetyWay>0</SafetyWay>" STR_0D0A
                "<Status>ON</Status>" STR_0D0A
                "</Item>" STR_0D0A,
                g_core.config_ua->alarm_id, g_core.config_ua->manufacturer, device_id);
    }

    ua_mobilepos_t mp = g_core.mobilepos.pos.load();
    snprintf(body, BODY_LEN, "<?xml version=\"1.0\" encoding=\"%s\"?>" STR_0D0A
                                "<Response>" STR_0D0A
                                "<CmdType>Catalog</CmdType>" STR_0D0A
                                "<SN>%s</SN>" STR_0D0A
                                "<DeviceID>%s</DeviceID>" STR_0D0A
                                "<SumNum>%d</SumNum>" STR_0D0A
                                "<DeviceList Num=\"%d\">" STR_0D0A
                                "<Item>" STR_0D0A
                                "<DeviceID>%s</DeviceID>" STR_0D0A
                                "<Name>%s</Name>" STR_0D0A
                                "<Manufacturer>%s</Manufacturer>" STR_0D0A
                                "<Model>%s</Model>" STR_0D0A
                                "<Owner></Owner>" STR_0D0A
                                "<CivilCode></CivilCode>" STR_0D0A
                                "<Address></Address>" STR_0D0A
                                "<Parental>0</Parental>" STR_0D0A
                                "<ParentID>%s</ParentID>" STR_0D0A
                                "<SafetyWay>0</SafetyWay>" STR_0D0A
                                "<RegisterWay>1</RegisterWay>" STR_0D0A
                                "<Secrecy>0</Secrecy>" STR_0D0A
                                "<Status>ON</Status>" STR_0D0A
                                "<Longitude>%.3g</Longitude>" STR_0D0A
                                "<Latitude>%.3g</Latitude>" STR_0D0A
#if EXT_FILED // 康拓普
                                "<SerialNumber>%s</SerialNumber>" STR_0D0A
#endif
                                "</Item>" STR_0D0A
                                "<Item>" STR_0D0A
                                "<DeviceID>%s</DeviceID>" STR_0D0A
                                "<Name>%s</Name>" STR_0D0A
                                "<Manufacturer>%s</Manufacturer>" STR_0D0A
                                "<Model>%s</Model>" STR_0D0A
                                "<Owner></Owner>" STR_0D0A
                                "<CivilCode></CivilCode>" STR_0D0A
                                "<Address></Address>" STR_0D0A
                                "<Parental>0</Parental>" STR_0D0A
                                "<ParentID>%s</ParentID>" STR_0D0A
                                "<SafetyWay>0</SafetyWay>" STR_0D0A
                                "<RegisterWay>1</RegisterWay>" STR_0D0A
                                "<Secrecy>0</Secrecy>" STR_0D0A
                                "<Status>ON</Status>" STR_0D0A
                                "<Longitude>%.3g</Longitude>" STR_0D0A
                                "<Latitude>%.3g</Latitude>" STR_0D0A
#if EXT_FILED // 康拓普
                                "<SerialNumber>%s</SerialNumber>" STR_0D0A
#endif
                                "</Item>" STR_0D0A
                                "%s" // alarm channel
                                "</DeviceList>" STR_0D0A
                                "</Response>" STR_0D0A,
            g_core.config_ua->charset, sn, device_id, device_num, device_num,
            // video channel
            g_core.config_ua->video_id, g_core.config_ua->name, g_core.config_ua->manufacturer, g_core.config_ua->model,
            g_core.config_ua->server_id, mp.longitude, mp.latitude,
#if EXT_FILED // 康拓普
            g_core.config_ua->serial_number,
#endif
            // audio channel
            g_core.config_ua->audio_id, "AudioOut", g_core.config_ua->manufacturer, "", device_id,
            mp.longitude, mp.latitude
#if EXT_FILED // 康拓普
            , g_core.config_ua->serial_number
#endif
            // alarm channel
            , alarm_item
            );
    return 0;
}

int ua_device_info_response(char* sn, char* device_id, char* body)
{
    snprintf(body, BODY_LEN, "<?xml version=\"1.0\" encoding=\"%s\"?>" STR_0D0A
                                "<Response>" STR_0D0A
                                "<CmdType>DeviceInfo</CmdType>" STR_0D0A
                                "<SN>%s</SN>" STR_0D0A
                                "<DeviceID>%s</DeviceID>" STR_0D0A
                                "<Result>OK</Result>" STR_0D0A
                                "<DeviceName>%s</DeviceName>" STR_0D0A
                                "<Manufacturer>%s</Manufacturer>" STR_0D0A
                                "<Model>%s</Model>" STR_0D0A
                                "<Firmware>%s</Firmware>" STR_0D0A
                                "<Channel>%d</Channel>" STR_0D0A
#if EXT_FILED // 康拓普
                                "<Name>%s</Name>" STR_0D0A
                                 "<SerialNumber>%s</SerialNumber>" STR_0D0A
#endif
                                //"<MaxCamera>1</MaxCamera>" STR_0D0A
                                //"<MaxAlarm>0</MaxAlarm>" STR_0D0A
                                "</Response>" STR_0D0A,
            g_core.config_ua->charset, sn, device_id,
            g_core.config_ua->name, g_core.config_ua->manufacturer,
            g_core.config_ua->model, g_core.config_ua->firmware, g_core.config_ua->channel
#if EXT_FILED // 康拓普
            , g_core.config_ua->name, g_core.config_ua->serial_number
#endif
            );
    return 0;
}

int ua_device_config_response(char* sn, char* device_id, char* body)
{
    snprintf(body, BODY_LEN, "<?xml version=\"1.0\" encoding=\"%s\"?>" STR_0D0A
                                "<Response>" STR_0D0A
                                "<CmdType>ConfigDownload</CmdType>" STR_0D0A
                                "<SN>%s</SN>" STR_0D0A
                                "<DeviceID>%s</DeviceID>" STR_0D0A
                                "<Result>OK</Result>" STR_0D0A
                                "<BasicParam>" STR_0D0A
                                "<Name>%s</Name>" STR_0D0A
                                "<Expiration>%d</Expiration>" STR_0D0A
                                "<HeartBeatInterval>%d</HeartBeatInterval>" STR_0D0A
                                "<HeartBeatCount>%d</HeartBeatCount>" STR_0D0A
                                "<PositionCapability>%d</PositionCapability>" STR_0D0A
                                //"<Longitude>0</Longitude>" STR_0D0A
                                //"<Latitude>0</Latitude>" STR_0D0A
                                "</Response>" STR_0D0A,
            g_core.config_ua->charset, sn, device_id,
            g_core.config_ua->name, g_core.config_ua->expiry,
            g_core.config_ua->heartbeat_interval, g_core.config_ua->heartbeat_count,
            g_core.config_ua->pos_capability);
    return 0;
}

int ua_device_status_response(char* sn, char* device_id, char* body)
{
    char curtime[72] = {0};
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    sprintf(curtime, "%d-%d-%dT%02d:%02d:%02d",
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    // 是否有报警通道且报警状态是否开启
    char alarm_status[1024]; alarm_status[0] = '\0';
    if (strlen(g_core.config_ua->alarm_id) > 0) {
        const char *duty_status = get_duty_status_name(g_core.alarm.duty_status);
        sprintf(alarm_status,
                "<Alarmstatus Num=\"%d\">" STR_0D0A
                "<Item>" STR_0D0A
                "<DeviceID>%s</DeviceID>" STR_0D0A     // 报警设备编码(必选)
                "<DutyStatus>%s</DutyStatus>" STR_0D0A // 报警设备状态(必选)
                "</Item>" STR_0D0A
                "</Alarmstatus>" STR_0D0A, 1, g_core.config_ua->alarm_id, duty_status);
    }

    snprintf(body, BODY_LEN, "<?xml version=\"1.0\"?>" STR_0D0A
                                "<Response>" STR_0D0A
                                "<CmdType>DeviceStatus</CmdType>" STR_0D0A
                                "<SN>%s</SN>" STR_0D0A
                                "<DeviceID>%s</DeviceID>" STR_0D0A
                                "<Result>OK</Result>" STR_0D0A
                                "<Online>ONLINE</Online>" STR_0D0A // OFFLINE
                                //"<Reason></Reason>" STR_0D0A // 不正常工作原因(可选)
                                "<Status>OK</Status>" STR_0D0A
                                "<DeviceTime>%s</DeviceTime>" STR_0D0A
                                "<Encode>ON</Encode>" STR_0D0A
                                "<Record>%s</Record>" STR_0D0A
                                "%s" // AlarmStatus
                                "</Response>" STR_0D0A,
            sn, device_id, curtime, g_core.is_recording ? "ON" : "OFF", alarm_status);
    return 0;
}

int ua_device_record_info_response(char* sn, char* device_id, const record_info_t* rec_query, char* body)
{
    int sum_num = 0, list_num = 0;

    // 检索本地目录文件
    if (rec_query->start_time[0] != '\0' && rec_query->end_time[0] != '\0') {

        do {
            struct tm start_day_tm, end_day_tm;
            struct tm start_sec_tm, end_sec_tm;
            strptime(rec_query->start_time,"%Y-%m-%d", &start_day_tm); // 2010-11-11
            strptime(rec_query->end_time,"%Y-%m-%d", &end_day_tm);     // 2010-12-11
            strptime(rec_query->start_time,"%Y-%m-%dT%H:%M:%S", &start_sec_tm); // 2010-11-11T19:46:17
            strptime(rec_query->end_time,"%Y-%m-%dT%H:%M:%S", &end_sec_tm);     // 2010-12-11T19:46:17

            time_t start_sec_ts = mktime(&start_sec_tm);
            time_t end_sec_ts = mktime(&end_sec_tm);
            if (start_sec_ts < end_sec_ts) {
                LOGW("record start ts %s less than end ts %s", rec_query->start_time, rec_query->end_time);
                break;
            }

            // TODO: 查询对应录像。这里可以自行开发，有需要的朋友可以联系我们

//            if (rec_query->file_path[0] == '\0') {
//                char new_path[1024];
//                std::list<char[64]> record_files;
//
//                /**
//                 * 同年-同月-同天
//                 * 2010-11-11
//                 * 2010-11-11
//                 *
//                 * 同年-同月-天大
//                 * 2010-11-11
//                 * 2010-11-21
//                 *
//                 * 同年-月大
//                 * 2010-11-11
//                 * 2010-12-11 2010-12-21 2010-12-01
//                 *
//                 *
//                 * 不同年-月等
//                 * 2010-11-11
//                 * 2011-11-11 2011-11-21 2011-11-01
//                 *
//                 * 不同年-月大
//                 * 2010-11-11
//                 * 2011-12-11 2011-12-21 2011-12-01
//                 *
//                 * 不同年-月小
//                 * 2010-11-11
//                 * 2011-02-11 2011-02-21 2011-02-01
//                 *
//                 */
//
//                // 以天为最小单位，计算 [start_time, end_time] 之间所有目录下的满足条件的文件
//                time_t start_day_ts = mktime(&start_day_tm);
//                time_t end_day_ts = mktime(&end_day_tm);
//
//                double diff_sec = difftime(end_sec_ts, start_sec_ts); // 时间差(单位:秒)
//                int diff_day = diff_sec / (24 * 3600);
//                if (diff_day > 365) {
//                    LOGW("record more than default one year");
//                    diff_day = 365; // 最大检索1年的录像列表
//                }
//                for (int i = 0; i <= diff_day; i++) {
//                    // 获取后一天时间
//                    // https://blog.csdn.net/liuhai200913/article/details/51025168
//                    time_t next_day = start_day_ts + i * 24 * 3600;
//                    struct tm *t = localtime(&next_day); // gmtime
//
//                    snprintf(new_path, sizeof(new_path) - 1, "%s/%4d/%02d/%02d/",
//                             g_core.rec_path, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
//
//                    if (i == 0) { // 同一天时比较时分秒
//                        // 直接进入录制目录查找对应条件的文件
//                        find_files(g_core.rec_path, rec_query->start_time, rec_query->end_time,
//                                   rec_query->secrecy, get_record_type_name(rec_query->type));
//                    } else { // 不同一天时全部满足
//                        // 直接进入录制目录查找文件
//                        find_files(g_core.rec_path, rec_query->secrecy, get_record_type_name(rec_query->type));
//                    }
//                }
//            } else {
//                // 进入指定目录查找对应条件的文件
//                find_files(rec_query->file_path, rec_query->start_time, rec_query->end_time,
//                           rec_query->secrecy, get_record_type_name(rec_query->type));
//            }
        } while (0);
    }

    snprintf(body, BODY_LEN, "<?xml version=\"1.0\"?>" STR_0D0A
                         "<Response>" STR_0D0A
                         "<CmdType>RecordInfo</CmdType>" STR_0D0A
                         "<SN>%s</SN>" STR_0D0A
                         "<DeviceID>%s</DeviceID>" STR_0D0A
                         "<Name>%s</Name>" STR_0D0A
                         //"<Result>OK</Result>" STR_0D0A
                         //"<Result>ERROR</Result>" STR_0D0A
                         //"<Reason>Cannot get record file</Reason>" STR_0D0A
                         "<SumNum>%d</SumNum>" STR_0D0A
                         "<RecordList Num=\"%d\">"

                         "</RecordList>" STR_0D0A
                         "</Response>" STR_0D0A,
             sn, device_id, g_core.config_ua->name, sum_num, list_num);
    return 0;
}

int ua_subscribe_response(uacore_t *core, eXosip_event_t *je, char* body, int code)
{
    int ret = 0;
    osip_message_t* msg = NULL;
    ret = eXosip_insubscription_build_answer(core->context, je->tid, code, &msg);
    if (ret != 0) {
        LOGE("subscription %d build answer failed %d", je->tid, ret);
        return ret;
    }
    if (body && strlen(body) > 0) {
        ret = osip_message_set_body(msg, body, strlen(body));
        if (0 != ret) {
            LOGE("message %d set body failed %d", je->tid, ret);
            return ret;
        }
    }
    ////ua_gm_add_note_header(core, "SUBSCRIBE", msg);
    ret = eXosip_insubscription_send_answer(core->context, je->tid, code, msg);
    if (0 != ret) {
        LOGE("subscription %d send answer failed %d", je->tid, ret);
    }
    return ret;
}

int ua_subscribe_catalog_request(uint32_t sn, char* device_id, char* body)
{
    int device_num = 2;
    char alarm_item[1024]; alarm_item[0] = '\0';
    if (strlen(g_core.config_ua->alarm_id) > 0) {
        device_num += 1;

        sprintf(alarm_item,
                "<Item>" STR_0D0A
                "<DeviceID>%s</DeviceID>" STR_0D0A
                "<Name>Alarm</Name>" STR_0D0A
                "<Manufacturer>%s</Manufacturer>" STR_0D0A
                "<Parental>0</Parental>" STR_0D0A
                "<ParentID>%s</ParentID>" STR_0D0A
                "<SafetyWay>0</SafetyWay>" STR_0D0A
                "<Status>ON</Status>" STR_0D0A
                "</Item>" STR_0D0A,
                g_core.config_ua->alarm_id, g_core.config_ua->manufacturer, device_id);
    }

    snprintf(body, BODY_LEN, "<?xml version=\"1.0\" encoding=\"%s\"?>" STR_0D0A
                        "<Notify>" STR_0D0A
                        "<CmdType>Catalog</CmdType>" STR_0D0A
                        "<SN>%d</SN>" STR_0D0A
                        "<DeviceID>%s</DeviceID>" STR_0D0A
                        "<SumNum>%d</SumNum>" STR_0D0A
                        "<DeviceList Num=\"%d\">" STR_0D0A
                        "<Item>" STR_0D0A
                        "<DeviceID>%s</DeviceID>" STR_0D0A
                        "<Name>%s</Name>" STR_0D0A
                        "<Manufacturer>%s</Manufacturer>" STR_0D0A
                        "<Model>%s</Model>" STR_0D0A
                        "<Owner></Owner>" STR_0D0A
                        "<CivilCode></CivilCode>" STR_0D0A
                        "<Address></Address>" STR_0D0A
                        "<Parental>0</Parental>" STR_0D0A
                        "<ParentID>%s</ParentID>" STR_0D0A
                        "<SafetyWay>0</SafetyWay>" STR_0D0A
                        "<RegisterWay>1</RegisterWay>" STR_0D0A
                        "<Secrecy>0</Secrecy>" STR_0D0A
                        "<Status>ON</Status>" STR_0D0A
                        "<Event>ON</Event>" STR_0D0A
#if EXT_FILED // 康拓普
                        "<SerialNumber>%s</SerialNumber>" STR_0D0A
 #endif
                        "</Item>" STR_0D0A
                        "<Item>" STR_0D0A
                        "<DeviceID>%s</DeviceID>" STR_0D0A
                        "<Name>%s</Name>" STR_0D0A
                        "<Manufacturer>%s</Manufacturer>" STR_0D0A
                        "<Model>%s</Model>" STR_0D0A
                        "<Owner></Owner>" STR_0D0A
                        "<CivilCode></CivilCode>" STR_0D0A
                        "<Address></Address>" STR_0D0A
                        "<Parental>0</Parental>" STR_0D0A
                        "<ParentID>%s</ParentID>" STR_0D0A
                        "<SafetyWay>0</SafetyWay>" STR_0D0A
                        "<RegisterWay>1</RegisterWay>" STR_0D0A
                        "<Secrecy>0</Secrecy>" STR_0D0A
                        "<Status>ON</Status>" STR_0D0A
                        "<Event>ON</Event>" STR_0D0A
#if EXT_FILED // 康拓普
                        "<SerialNumber>%s</SerialNumber>" STR_0D0A
#endif
                        "</Item>" STR_0D0A
                        "%s" // alarm channel
                        "</DeviceList>" STR_0D0A
                        "</Notify>" STR_0D0A,
            g_core.config_ua->charset, sn, device_id, device_num, device_num,
            // video channel
            g_core.config_ua->video_id, g_core.config_ua->name, g_core.config_ua->manufacturer, g_core.config_ua->model,
            g_core.config_ua->server_id,
#if EXT_FILED // 康拓普
            g_core.config_ua->serial_number,
#endif
            // audio channel
            g_core.config_ua->audio_id, "AudioOut", g_core.config_ua->manufacturer, "", device_id
#if EXT_FILED // 康拓普
            , g_core.config_ua->serial_number
#endif
            // alarm channel
            , alarm_item
            );
    return 0;
}

int ua_subscribe_mobilepos_request(uint32_t sn, char* device_id, char* body)
{
    ua_mobilepos_t mp = g_core.mobilepos.pos.load();
    snprintf(body, BODY_LEN, "<?xml version=\"1.0\"?>" STR_0D0A
                                "<Notify>" STR_0D0A
                                "<CmdType>MobilePosition</CmdType>" STR_0D0A
                                "<SN>%u</SN>" STR_0D0A
                                "<DeviceID>%s</DeviceID>" STR_0D0A
                                "<Time>%s</Time>" STR_0D0A
                                "<Longitude>%.3g</Longitude>" STR_0D0A
                                "<Latitude>%.3g</Latitude>" STR_0D0A
                                "<Speed>%.3g</Speed>" STR_0D0A
                                "<Direction>%.3g</Direction>" STR_0D0A
                                "<Altitude>%.3g</Altitude>" STR_0D0A
                                "</Notify>" STR_0D0A,
             sn, device_id, mp.time, mp.longitude, mp.latitude,
             mp.speed, mp.direction, mp.altitude);
    return 0;
}

int ua_subscribe_alarm_request(uint32_t sn, char* device_id, char* body)
{
    std::unique_lock<std::mutex> lock(g_core.alarm.lock);
    if (NULL == g_core.alarm.alarms || g_core.alarm.alarms->empty()) {
        return -1;
    }
    ua_alarm_t alarm = g_core.alarm.alarms->front();

    snprintf(body, BODY_LEN, "<?xml version=\"1.0\" encoding=\"%s\"?>" STR_0D0A
                         "<Notify>" STR_0D0A
                         "<CmdType>Alarm</CmdType>" STR_0D0A
                         "<SN>%u</SN>" STR_0D0A
                         "<DeviceID>%s</DeviceID>" STR_0D0A
                         "<AlarmPriority>%s</AlarmPriority>" STR_0D0A
                         "<AlarmMethod>%s</AlarmMethod>" STR_0D0A
                         "<AlarmTime>%s</AlarmTime>" STR_0D0A,
             g_core.config_ua->charset, sn, device_id, alarm.priority, alarm.method, alarm.time);
    if (strlen(alarm.desc) > 0) {
        snprintf(body + strlen(body), BODY_LEN, "<AlarmDescription>%s</AlarmDescription>" STR_0D0A, alarm.desc);
    }
    if (alarm.longitude > 0) {
        snprintf(body + strlen(body), BODY_LEN, "<Longitude>%.3g</Longitude>" STR_0D0A, alarm.longitude);
    }
    if (alarm.latitude > 0) {
        snprintf(body + strlen(body), BODY_LEN, "<Latitude>%.3g</Latitude>" STR_0D0A, alarm.latitude);
    }
    if (alarm.type > 0) {
        snprintf(body + strlen(body), BODY_LEN, "<Info>" STR_0D0A
                                            "<AlarmType>%d</AlarmType>" STR_0D0A, alarm.type);
        if (alarm.event_type > 0 && alarm.type == video_alarm_type::INTRUSION) {
            snprintf(body + strlen(body), BODY_LEN, "<AlarmTypeParam>" STR_0D0A
                                                "<EventType>%d</EventType>" STR_0D0A
                                                "</AlarmTypeParam>" STR_0D0A, alarm.event_type);
        }
        snprintf(body + strlen(body), BODY_LEN, "</Info>" STR_0D0A);
    }
    snprintf(body + strlen(body), BODY_LEN, "</Notify>" STR_0D0A);

    g_core.alarm.alarms->pop_front();
    return 0;
}

int ua_gm_add_note_header(uacore_t *core, const char* method, osip_message_t* msg)
{
    int ret = -1;
    if (!core->config_ua->gm_level) {
        return ret;
    }
    return ua_gm_gen_note(method, msg);
}

int ua_gm_gen_note(const char* method, osip_message_t* msg)
{
    int ret = -1;
    if (NULL == method || NULL == msg) {
        LOGE("gm generate note with invalid param");
        return ret;
    }

    static char time_buf[24];
    const char* date = get_time_with_t(time_buf, 24);
    char *from = NULL, *to = NULL, *callid = NULL, *buf = NULL, *nonce = NULL;
    uint8_t dgst[1024] = { 0 };
    int body_len = 0, buf_len, dgst_len, nonce_len;

    osip_body_t *body = NULL;
    osip_message_get_body(msg, 0, &body);
    if (NULL != body) {
        body_len = body->length;
    }

    osip_from_to_str(osip_message_get_from(msg), &from);
    osip_to_to_str(osip_message_get_to(msg), &to);
    osip_call_id_to_str(osip_message_get_call_id(msg), &callid);

    if (NULL == from || NULL == to || NULL == callid) {
        LOGE("gm generate note need from to callid");
        goto clear;
    }

    // nonce = method + from + to + callId + date + vkek + content
    buf_len = strlen(method) + strlen(from) + strlen(to) + strlen(callid) +
            strlen(date) + strlen(g_core.gm.vkek) + body_len;
    buf = (char*) osip_malloc(buf_len + 1);
    snprintf(buf, buf_len + 1, "%s%s%s%s%s%s%s", method, from, to, callid,
             date, g_core.gm.vkek, body_len > 0 ? body->body : "");
    buf[buf_len] = '\0';
    ret = gm_sm3_hash((uint8_t *) buf, buf_len, dgst, dgst_len);
    if (ret != 0) {
        LOGE("sm3 hash failed");
        goto clear;
    }

    nonce_len = AV_BASE64_SIZE(dgst_len);
    nonce = (char*) osip_malloc(nonce_len + 1);
    av_base64_encode(nonce, nonce_len, dgst, dgst_len);
    snprintf(buf, buf_len, "Digest nonce=\"%s\", algorithm=SM3", nonce);
    LOGD("nonce dgst %s len %d,\n nonce %s len %d", dgst, dgst_len, nonce, nonce_len);

    osip_message_replace_header(msg, "Note", buf);
    osip_message_replace_header(msg, "Date", date);

clear:
    osip_free(from);
    osip_free(to);
    osip_free(callid);
    osip_free(buf);
    osip_free(nonce);
    return ret;
}

int ua_gm_sign_sign1(const char *username, const char *random1,
                     const char *random2, const char *serverid, char *sign1)
{
    int ret = -1;
    char sign[BODY_LEN] = {0}, buf[1024] = {0};
    int type = NID_undef, sign_len, sign1_len;

    if (NULL == random1 || NULL == random2 || NULL == serverid || NULL == sign1) {
        LOGE("sign invalid params");
        return -1;
    }

    LOGI("gm sign1: %s %s %s %s", username, random1, random2, serverid);
    snprintf(buf, sizeof(buf), "%s%s%s", random2, random1, serverid);
    ret = gm_sm2_sign(g_core.gm.device_ec_prikey, (uint8_t *) buf, (int) strlen(buf),
                      (uint8_t *) sign, sign_len);
    if (ret != 0) {
        LOGE("sign failed %d", ret);
        return ret;
    }
    sign[sign_len] = '\0';

    sign1_len = AV_BASE64_SIZE(sign_len);
    av_base64_encode(sign1, sign1_len, (uint8_t *) sign, sign_len);
    sign1[sign1_len] = '\0';

    strcpy(g_core.gm.random2, random2);

    return ret;
}

int ua_gm_verify_sign2(const char *username, const char *random1,
                       const char *random2, const char *cryptkey, const char *sign2)
{
    int ret = -1;
    uint8_t sign[1024] = {0};
    char buf[1024] = {0};
    int type = NID_undef, sign2_len;

    if (NULL == random1 || NULL == random2 || NULL == cryptkey || NULL == sign2) {
        LOGE("sign2 invalid params");
        return -1;
    }

    sign2_len = AV_BASE64_DECODE_SIZE(strlen(sign2));
    sign2_len = av_base64_decode(sign, sign2, sign2_len);

    //for (int i = 0; i < sign2_len; i++) LOGI("%02x", sign[i]);
    LOGD("gm sign2: %s %s %s %s, len:%d", username, random1, random2, cryptkey, sign2_len);
    snprintf(buf, sizeof(buf), "%s%s%s%s", random1, random2, username, cryptkey);
    ret = gm_sm2_verify(g_core.gm.server_ec_pubkey, (uint8_t *) buf, (int) strlen(buf), sign, sign2_len);

    return ret;
}


/* event ua_keepalive loop */
void* ua_keepalive_thread(void *arg)
{
    uacore_t *core = (uacore_t *)arg;
    LOGI("start keepalive, tid:%d", gettid());
    while (core && core->running && core->config_ua) {
        if (core->registed != REG_OK) {
            osip_usleep(1000000);
            continue;
        }
        if (DEBUG) {
            struct eXosip_stats stats;
            memset(&stats, 0, sizeof(struct eXosip_stats));
            eXosip_lock(core->context);
            eXosip_set_option(core->context, EXOSIP_OPT_GET_STATISTICS, &stats);
            eXosip_unlock(core->context);
            LOGD("eXosip stats: inmemory=(tr:%i//reg:%i) average=(tr:%f//reg:%f)",
                 stats.allocated_transactions, stats.allocated_registrations,
                 stats.average_transactions, stats.average_registrations);
        }
        eXosip_lock(core->context);
        ua_notify_keepalive(core);
        eXosip_unlock(core->context);
        osip_usleep(core->config_ua->heartbeat_interval * 1000000);
    }
    LOGI("stop keepalive");
    return NULL;
}

/* event ua_catalog loop */
void* ua_catalog_thread(void *arg)
{
	return ua_subscribe_notify(arg, subscribe_type::SUBSCRIBE_CATALOG);
}

/* event ua_mobilepos loop */
void* ua_pos_thread(void *arg)
{
	return ua_subscribe_notify(arg, subscribe_type::SUBSCRIBE_MOBILEPOS);
}

/* event ua_alarm loop */
void* ua_alarm_thread(void *arg)
{
	return ua_subscribe_notify(arg, subscribe_type::SUBSCRIBE_ALARM);
}

void* ua_subscribe_notify(void *arg, subscribe_type type)
{
    int ret;
    uacore_t *core = (uacore_t *)arg;
    subscribe_t *subscribe = NULL;
    const char *ctype = get_subscribe_type_name(type);

    switch (type) {
        case SUBSCRIBE_CATALOG:
            subscribe = &core->catalog.subscribe;
            break;
        case SUBSCRIBE_MOBILEPOS:
            subscribe = &core->mobilepos.subscribe;
            break;
        case SUBSCRIBE_ALARM:
            subscribe = &core->alarm.subscribe;
            break;
        default:
            return NULL;
    }

    LOGI("start %s, tid:%d, registed:%d, did:%d, expiry:%d, interval:%d",
         ctype, gettid(), (int) core->registed, subscribe->did,
         subscribe->expiry, subscribe->interval);
    while (core && core->running && (core->registed == REG_OK) && core->config_ua &&
           subscribe->did > 0 && subscribe->expiry > 0 && subscribe->interval > 0) {
        ret = -1;
        eXosip_lock(core->context);
        if (SUBSCRIBE_CATALOG == type) {
            LOGE("sssss %d", core->config_changed.load());
            // NOTIFY在设备信息改变后发送，否则不发送
            if (core->config_changed) {
                ret = ua_notify_subscribe(core, type, subscribe);
            }
        } else {
            ret = ua_notify_subscribe(core, type, subscribe);
        }
        eXosip_unlock(core->context);
        if (SUBSCRIBE_CATALOG == type && ret == 0) {
            if (core->config_changed) {
                core->config_changed = 0;
            }
        }
        osip_usleep(subscribe->interval * 1000000);
    }
    eXosip_lock(core->context);
    eXosip_insubscription_remove(core->context, subscribe->did);
    eXosip_unlock(core->context);
    subscribe->expiry = 0;
    subscribe->did = 0;
    subscribe->interval = 0;
    subscribe->sn = 0;
    LOGI("stop %s", ctype);
    return NULL;
}

/* event ua_event loop */
void* ua_event_thread(void *arg)
{
    uacore_t *core = (uacore_t *)arg;
    int ret = 0;
    int status_code = -1;
    LOGI("start event, tid:%d", gettid());

    while (core->running) {
        eXosip_event_t *je = eXosip_event_wait(core->context, 0, 20);

        /* auto process,such as:register refresh,auth,call keep... */
        eXosip_lock(core->context);
        eXosip_automatic_action(core->context);

        if (NULL == je) {
            eXosip_unlock(core->context);
            osip_usleep(100000); // 10000
            continue;
        }

        if (NULL != je->request && NULL != je->request->call_id &&
            NULL != je->request->call_id->number && NULL != je->request->call_id->host) {
            LOGI("request type %d callid %s@%s", je->type, je->request->call_id->host, je->request->call_id->number);
        } else if (NULL != je->response && NULL != je->response->call_id &&
                   NULL != je->response->call_id->number && NULL != je->response->call_id->host) {
            LOGI("response type %d callid %s@%s", je->type, je->response->call_id->host, je->response->call_id->number);
        } else {
            LOGI("event type %d", je->type);
        }

        switch (je->type) {
            case EXOSIP_REGISTRATION_SUCCESS: {
                ua_printf_msg(je, REQUEST);
                // 通过expires判断是注册还是注销
                osip_header_t *header = NULL;
                osip_message_header_get_byname(je->response ? je->response : je->request, "expires", 0, &header);
                int is_register = header ? (atoi(header->hvalue) > 0 ? 1 : 0) : (UA_CMD_REGISTER == core->cmd);

                ret = 0;
                if (core->config_ua->gm_level > 0) {
                    int len, out_len, buf_len;
                    osip_header_t *security_info_header = NULL;
                    do {
                        ret = osip_message_header_get_byname(je->response, "SecurityInfo", 0, &security_info_header);
                        if (NULL == security_info_header) {
                            LOGE("no SecurityInfo header");
                            break;
                        }

                        ret = get_str(security_info_header->hvalue, "cryptkey=\"",
                                      false, "\"", false, core->gm.cryptkey, &len);
                        if (ret != 0) {
                            LOGE("no cryptkey");
                            break;
                        }
                        // base64Decode -> [decodeDERSM2Cipher -> sm2Decrypt] (SM2_decrypt with sm3)
                        uint8_t buf[1024];
                        out_len = AV_BASE64_DECODE_SIZE(len);
                        buf_len = av_base64_decode(buf, core->gm.cryptkey, out_len);
                        // 检查algorithm是否为sm3
                        ret = gm_sm2_decrypt(core->gm.device_ec_prikey, buf, buf_len, (uint8_t *) core->gm.vkek, out_len);
                        if (ret != 0) {
                            break;
                        }

                        if (!core->gm.unidirection) { // 双向注册
                            char random1[64], random2[64], deviceid[32];
                            ret = get_str(security_info_header->hvalue, "random1=\"",
                                          false, "\"", false, random1, &len);
                            if (ret != 0) {
                                LOGE("no random1");
                                break;
                            }
                            if (osip_strncasecmp(core->gm.random1, random1, len) != 0) {
                                LOGE("random1 mismatch");
                                // break;
                            }

                            ret = get_str(security_info_header->hvalue, "random2=\"",
                                          false, "\"", false, random2, &len);
                            if (ret != 0) {
                                LOGE("no random2");
                                break;
                            }
                            if (osip_strncasecmp(core->gm.random2, random2, len) != 0) {
                                LOGE("random2 mismatch");
                                break;
                            }

                            ret = get_str(security_info_header->hvalue, "deviceid=\"",
                                          false, "\"", false, deviceid, &len);
                            if (ret != 0) {
                                LOGE("no deviceid");
                                break;
                            }
                            if (osip_strncasecmp(core->config_ua->username, deviceid, len) != 0) {
                                LOGE("deviceid mismatch");
                                break;
                            }

                            ret = get_str(security_info_header->hvalue, "sign2=\"",
                                          false, "\"", false, core->gm.sign2, &len);
                            if (ret != 0) {
                                LOGE("no sign2");
                                break;
                            }

                            ret = ua_gm_verify_sign2(core->config_ua->username, random1, random2, core->gm.cryptkey, core->gm.sign2);
                            if (ret != 0) {
                                break;
                            }
                        }
                    } while (0);
                }

                if (is_register) {
                    if (ret == 0) {
                        LOGI("register %d success", je->rid);
                        {
                            std::unique_lock<std::mutex> lock(core->lock);
                            core->reg_cond.notify_all();
                        }
                        if (core->config_ua->on_callback_status != NULL) {
                            core->config_ua->on_callback_status(EVENT_REGISTER_OK, 0, NULL, 0);
                        }
                    } else {
                        LOGE("register %d failed", je->rid);
                        {
                            std::unique_lock<std::mutex> lock(core->lock);
                            core->registed = REG_FAILED;
                            core->reg_cond.notify_all();
                        }
                    }
                } else {
                    if (ret == 0) {
                        LOGI("unregister %d success", je->rid);
                        {
                            std::unique_lock<std::mutex> lock(core->lock);
                            core->registed = REG_INIT;
                            core->regid = 0;
                            core->unreg_cond.notify_all();
                        }
                        if (core->config_ua->on_callback_status != NULL) {
                            core->config_ua->on_callback_status(EVENT_UNREGISTER_OK, 0, NULL, 0);
                        }
                    } else {
                        LOGE("unregister %d failed", je->rid);
                        {
                            std::unique_lock<std::mutex> lock(core->lock);
                            core->unreg_cond.notify_all();
                        }
                        if (core->config_ua->on_callback_status != NULL) {
                            core->config_ua->on_callback_status(EVENT_UNREGISTER_FAIL, 0, NULL, 0);
                        }
                    }
                }
                break;
            }
            case EXOSIP_REGISTRATION_FAILURE: {
                ua_printf_msg(je, REQUEST);
                if (je->response != NULL && (je->response->status_code == 401 || je->response->status_code == 407)) {
                    ua_printf_msg(je, RESPONSE);
                    if (core->config_ua->gm_level > 0) {
                        do {
                            osip_www_authenticate_t *wwwauth = NULL;
                            ret = osip_message_get_www_authenticate(je->response, 0, &wwwauth);
                            if (NULL != wwwauth) {
                                if (strstr(wwwauth->auth_type, "Unidirection") != NULL) {
                                    core->gm.unidirection = true;
                                } else if (strstr(wwwauth->auth_type, "Bidirection")) {
                                    core->gm.unidirection = false;
                                } else {
                                    core->gm.unidirection = true;
                                }

                                if (NULL == wwwauth->algorithm) {
                                    LOGE("no algorithm");
                                    break;
                                }
                                char* tmp = osip_strdup_without_quote(wwwauth->algorithm);
                                strcpy(core->gm.algorithm, tmp);
                                osip_free(tmp);

                                if (NULL == wwwauth->random1) {
                                    LOGW("no random1");
                                    break;
                                }
                                tmp = osip_strdup_without_quote(wwwauth->random1);
                                strcpy(core->gm.random1, tmp);
                                osip_free(tmp);
                            } else {
                                LOGE("no WWW-Authenticate header");
                                break;
                            }
                            LOGI("recv gm reg %d ok", je->response->status_code);
                        } while (0);
                    }
                }
                else {
                    osip_header_t *header = NULL;
                    osip_message_header_get_byname(je->response ? je->response : je->request, "expires", 0, &header);
                    int is_register = header ? (atoi(header->hvalue) > 0 ? 1 : 0) : (UA_CMD_REGISTER == core->cmd);
                    if (is_register) {
                        LOGE("register %d failed - %s", je->rid, je->textinfo);
                        {
                            std::unique_lock<std::mutex> lock(core->lock);
                            core->registed = REG_FAILED;
                            core->reg_cond.notify_all();
                        }
                        if (core->config_ua->on_callback_status != NULL) {
                            core->config_ua->on_callback_status(EVENT_REGISTER_AUTH_FAIL, 0, NULL, 0);
                        }
                    }
                    else {
                        LOGE("unregister %d failed - %s", je->rid, je->textinfo);
                        {
                            std::unique_lock<std::mutex> lock(core->lock);
                            core->unreg_cond.notify_all();
                        }
                        if (core->config_ua->on_callback_status != NULL) {
                            core->config_ua->on_callback_status(EVENT_UNREGISTER_FAIL, 0, NULL, 0);
                        }
                    }
                }
                break;
            }
            case EXOSIP_CALL_INVITE: {
                if (core->registed != REG_OK) {
                    LOGE("call invite not registed");
                    break;
                }

                char *remote_ip = NULL, *remote_port = NULL;
                int local_port = 0;
                char *start_time = NULL, *end_time = NULL;
                char *to_user, *to_seq;
                char setup[16];
                osip_header_t *subject = NULL;
                ua_transport transport;
                task_t *task = NULL;
                char resp_body[BODY_LEN];

                resp_body[0] = '\0';
                ret = -1;
                do {
                    ua_printf_msg(je, REQUEST);
                    if (je->request->message == NULL) {
                        LOGE("call invite message null");
                        break;
                    }
                    task = ua_notify_callid(core, je, true);
                    if (task == NULL) {
                        break;
                    }
                    task->tk_type = task_type::TT_STREAM;
                    ret = ua_cmd_call_ring(core, task);
                    if (ret != 0) {
                        break;
                    }
                    LOGI("call invite %d %d incoming, answer...", je->cid, je->did);
                    ret = -1;

                    // Subject: 44080000441320000176:000000001,34020000002000000001:000000001
                    ret = osip_message_header_get_byname(je->request, "Subject", 0, &subject);
                    if (subject == NULL) {
                        LOGE("call invite no Subject header");
                        break;
                    }
                    to_user = subject->hvalue;
                    to_seq = strchr(to_user, ':');
                    if (osip_strncasecmp(core->config_ua->video_id, to_user, to_seq - to_user) == 0) {
                    } else if ((osip_strncasecmp(core->config_ua->audio_id, to_user, to_seq - to_user) == 0)) {
                    } else {
                        LOGE("call invite unknown subject header %s", subject->hvalue);
                        break;
                    }

                    sdp_message_t *sdp_msg = eXosip_get_remote_sdp(core->context, je->did);
                    if (!sdp_msg) {
                        LOGW("invite sdp message null");
                        break;
                    }

                    sdp_connection_t *connection = eXosip_get_video_connection(sdp_msg);
                    if (!connection) {
                        LOGE("call invite no connection");
                        break;
                    }
                    remote_ip = connection->c_addr;

                    sdp_media_t *video_sdp = eXosip_get_video_media(sdp_msg);
                    if (!video_sdp) {
                        LOGE("call invite no video media");
                        break;
                    }
                    remote_port = video_sdp->m_port;

                    /*media_type:rtp_over_udp/rtp_over_tcp*/
                    LOGD("media:%s,port:%s,number_of_port:%s,proto:%s,remote:%s:%s\n",
                         video_sdp->m_media, video_sdp->m_port, video_sdp->m_number_of_port,
                         video_sdp->m_proto, remote_ip, remote_port);

                    char *session = sdp_message_s_name_get(sdp_msg);
                    if (!session)
                        break;

                    start_time = sdp_message_t_start_time_get(sdp_msg, 0);
                    if (start_time == NULL) {
                        start_time = (char *) "0";
                    }
                    end_time = sdp_message_t_stop_time_get(sdp_msg, 0);
                    if (end_time == NULL) {
                        end_time = (char *) "0";
                    }

                    /*setup:active/passive*/
                    for (int i = 0; i < video_sdp->a_attributes.nb_elt; i++) {
                        sdp_attribute_t *attr = (sdp_attribute_t *) osip_list_get(&video_sdp->a_attributes, i);
                        LOGD("%s: %s\n", attr->a_att_field, attr->a_att_value);
                        if (osip_strcasecmp(attr->a_att_field, "setup") == 0) {
                            strcpy(setup, attr->a_att_value);
                        }
                    }

                    // int pos = 0;
                    // while(1) {
                    //     if(sdp_message_endof_media(sdp_msg, pos) == 0) {
                    //         char *payload = sdp_message_m_payload_get(sdp_msg, pos, 0);
                    //         if(payload != NULL) {}
                    //         pos++;
                    //     } else {
                    //         break;
                    //     }
                    // }

                    get_str(je->request->message, "y=", false, "\n", false, task->y);
                    task->y[10] = '\0';

                    if (osip_strcasecmp(video_sdp->m_proto, "RTP/AVP") == 0) {
                        transport = ua_transport::UDP;
                        local_port = get_free_port(core, IPPROTO_UDP);
                        snprintf(resp_body, sizeof(resp_body),
                                 "v=0" STR_0D0A
                                 "o=%s 0 0 IN IP4 %s" STR_0D0A
                                 "s=%s" STR_0D0A
                                 "c=IN IP4 %s" STR_0D0A
                                 "t=%s %s" STR_0D0A
                                 "m=video %d RTP/AVP 96" STR_0D0A
                                 "a=sendonly" STR_0D0A
                                 "a=rtpmap:96 PS/90000" STR_0D0A
                                 //"a=rtpmap:98 H264/90000" STR_0D0A
                                 //"a=rtpmap:97 MPEG4/90000" STR_0D0A
                                 "y=%s" STR_0D0A,
                                 //"f=" STR_0D0A, // f=v/2/6/25/2/1024a///
                                 core->config_ua->username, core->localip, session, core->localip,
                                 start_time, end_time, local_port, task->y);
                        ret = ua_cmd_call_answer(core, task, resp_body);
                    } else if (osip_strcasecmp(video_sdp->m_proto, "TCP/RTP/AVP") == 0) {
                        if (osip_strcasecmp(setup, "passive") == 0) {
                            transport = ua_transport::TCP_PASSIVE;
                        } else if (osip_strcasecmp(setup, "active") == 0) {
                            transport = ua_transport::TCP_ACTIVE;
                        } else {
                            LOGW("Invalid invite protocol %s", video_sdp->m_proto);
                            transport = ua_transport::TCP_PASSIVE;
                        }

                        local_port = get_free_port(core, IPPROTO_TCP);
                        snprintf(resp_body, sizeof(resp_body),
                                 "v=0" STR_0D0A
                                 "o=%s 0 0 IN IP4 %s" STR_0D0A
                                 "s=%s" STR_0D0A
                                 "c=IN IP4 %s" STR_0D0A
                                 "t=%s %s" STR_0D0A
                                 "m=video %d TCP/RTP/AVP 96" STR_0D0A
                                 "a=sendonly" STR_0D0A
                                 "a=rtpmap:96 PS/90000" STR_0D0A
                                 //"a=rtpmap:98 H264/90000" STR_0D0A
                                 //"a=rtpmap:97 MPEG4/90000" STR_0D0A
                                 //"a=connection:new" STR_0D0A
                                 "a=setup:active" STR_0D0A
                                 "y=%s" STR_0D0A,
                                 //"f=" STR_0D0A,
                                 core->config_ua->username, core->config_ua->server_ip, session, core->localip,
                                 start_time, end_time, local_port, task->y);
                        ret = ua_cmd_call_answer(core, task, resp_body);
                    } else {
                        ret = -1;
                        ua_cmd_call_answer(core, task, resp_body);
                        LOGE("unknown invite media proto %s", video_sdp->m_proto);
                    }
                } while (false);

                if (ret == 0) {
                    LOGI("call answer %d %d ok %s", je->cid, je->did, resp_body);
                    strcpy(task->remote_ip, remote_ip);
                    task->remote_port = atoi(remote_port);
                    task->local_port = local_port;
                    task->transport = transport;
                    task->start_time = atol(start_time);
                    task->end_time = atol(end_time);
                    task->pl_type = payload_type::PT_PS;
                } else {
                    LOGW("call answer %d %d failed %s, %d", je->cid, je->did, resp_body, ret);
                    if (task) {
                        core->tasks->erase(task->cid);
                    }
                }
                break;
            }
            case EXOSIP_CALL_REINVITE:
                if (core->registed != REG_OK) {
                    LOGE("call reinvite not registed");
                    break;
                }
                ua_printf_msg(je, REQUEST);
                ua_notify_callid(core, je);
                ua_notify_call_keep(core, je);
                break;
            case EXOSIP_CALL_PROCEEDING:
                LOGI("call proceeding %d %d", je->cid, je->did);
                ua_printf_msg(je, RESPONSE);
                break;
            case EXOSIP_CALL_RINGING:
                if (core->registed != REG_OK) {
                    LOGE("call ringing not registed");
                    break;
                }
                if (je->request) {
                    ua_printf_msg(je, REQUEST);
                    ua_notify_callid(core, je);
                    LOGI("call ring %d", je->cid);
                }
                break;
            case EXOSIP_CALL_ANSWERED:
                LOGI("call answered %d %d", je->cid, je->did);
                if (je->response) {
                    do {
                        status_code = osip_message_get_status_code(je->response);
                        if (status_code == 200) {
                            char *remote_ip = NULL, *remote_port = NULL;
                            char setup[16], rtpmap[16];

                            task_t* task = ua_notify_callid(core, je, false, true);
                            if (NULL == task) {
                                LOGE("no task, %d %d", je->cid, je->did);
                                break;
                            }
                            LOGI("recv response ok, %d %d", je->cid, je->did);
                            task->did = je->did;
                            task->tid = je->tid;

                            if (task->tk_type == task_type::TT_VOICE) {
                                sdp_message_t *sdp_msg = eXosip_get_remote_sdp(core->context, je->did);
                                if (!sdp_msg) {
                                    LOGW("broadcast sdp message null");
                                    break;
                                }

                                sdp_connection_t *connection = eXosip_get_audio_connection(sdp_msg);
                                if (!connection) {
                                    LOGE("broadcast no connection");
                                    break;
                                }
                                remote_ip = connection->c_addr;

                                sdp_media_t *audio_sdp = eXosip_get_audio_media(sdp_msg);
                                if (!audio_sdp) {
                                    LOGE("broadcast no audio media");
                                    break;
                                }
                                remote_port = audio_sdp->m_port;

                                for (int i = 0; i < audio_sdp->a_attributes.nb_elt; i++) {
                                    sdp_attribute_t *attr = (sdp_attribute_t *) osip_list_get(&audio_sdp->a_attributes, i);
                                    LOGD("%s: %s\n", attr->a_att_field, attr->a_att_value);
                                    if (osip_strcasecmp(attr->a_att_field, "setup") == 0) {
                                        strcpy(setup, attr->a_att_value);
                                    } else if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
                                        strcpy(rtpmap, attr->a_att_value);
                                    }
                                }
                                // 解析对端发送过来的音频数据格式, 比如:
                                //      rtpmap:8 PCMA/8000
                                //      rtpmap:96 PS/90000
                                payload_type pt = payload_type::PT_PCMA;
                                if (strstr(rtpmap, "PS/") != NULL) {
                                    pt = payload_type::PT_PS;
                                }

                                ret = ua_notify_call_ack(core, je); // 发送ack
                                if (0 != ret) {
                                    LOGE("broadcast send ack failed, %d %d", je->cid, je->did);
                                    break;
                                }

                                strcpy(task->remote_ip, remote_ip);
                                task->remote_port = atoi(remote_port);
                                strcpy(task->local_ip, core->localip);
                                task->pl_type = pt;
                                task->acked = 1;

                                // 本地开启对应接收端口的 udp 或者 tcp server, 并发送探测包到远端, 远端用此链路发送语音
                                if (core->config_ua->on_callback_invite != NULL) {
                                    core->config_ua->on_callback_invite(task);
                                }
                            }
                        } else if (status_code >= 400) {
                            LOGE("broadcast recv response error %d", status_code);
                        } else {
                            LOGW("broadcast recv response %d", status_code);
                        }
                    } while (0);

                    if (0 != ret) {
                        ua_release_call(core, je);
                        break;
                    }
                }
                else {
                    ua_printf_msg(je, REQUEST);
                    ua_notify_callid(core, je);
                    ua_notify_call_ack(core, je);
                }
                break;
            case EXOSIP_CALL_NOANSWER:
                ua_printf_msg(je, REQUEST);
                LOGE("call noanswer %d %d", je->cid, je->did);
                ua_notify_callid(core, je);
                break;
            case EXOSIP_CALL_REQUESTFAILURE:
            case EXOSIP_CALL_GLOBALFAILURE:
            case EXOSIP_CALL_SERVERFAILURE:
                if (je->response) {
                    status_code = osip_message_get_status_code(je->response);
                }
                LOGE("call %d %d failed %d", je->type, je->cid, status_code);
                break;
            case EXOSIP_CALL_ACK: {
                task_t *task = NULL;
                ret = -1;
                do {
                    if (core->registed != REG_OK) {
                        LOGE("call ack not registed");
                        break;
                    }
                    ua_printf_msg(je, REQUEST);
                    LOGI("call ack %d %d", je->cid, je->did);

                    if (je->request->message == NULL) {
                        LOGE("invite ack message null");
                        break;
                    }
                    task = ua_notify_callid(core, je);
                    if (task == NULL) {
                        break;
                    }
                    if (task->acked) {
                        ret = 0;
                        LOGI("acked ssrc:%s", task->y);
                        break;
                    }
                    task->acked = 1;
                    LOGI("push ps stream to server rtp port:%d, ssrc:%s", task->remote_port, task->y);

                    if (core->config_ua->on_callback_invite != NULL) {
                        ret = core->config_ua->on_callback_invite(task);
                    }
                } while (false);

                if (ret != 0) {
                    if (task) {
                        ua_cmd_call_stop(core, task);
                        core->tasks->erase(task->cid);
                    }
                }
                break;
            }
            case EXOSIP_CALL_CLOSED:
                if (je->request) {
                    ua_printf_msg(je, REQUEST);
                    LOGI("call closed %d %d", je->cid, je->did);
                } else {
                    ua_printf_msg(je, RESPONSE);
                    LOGI("other side closed %d %d", je->cid, je->did);
                }
                ua_release_call(core, je);
                break;
            case EXOSIP_CALL_RELEASED:
                if (je->request) {
                    ua_printf_msg(je, REQUEST);
                } else {
                    ua_printf_msg(je, RESPONSE);
                }
                LOGI("call release %d %d", je->cid, je->did);
                ua_release_call(core, je);
                break;
            case EXOSIP_CALL_CANCELLED:
                ua_printf_msg(je, REQUEST);
                LOGI("call cancel %d", je->cid);
                break;
            case EXOSIP_CALL_MESSAGE_NEW:
                ua_printf_msg(je, REQUEST);
                break;
            case EXOSIP_CALL_MESSAGE_ANSWERED: {
                LOGI("call msg answered %d %d", je->cid, je->did);
                break;
            }
            case EXOSIP_CALL_MESSAGE_REQUESTFAILURE:
                LOGE("call msg req failed %d %d", je->cid, je->did);
                break;

                /*UA eXosip message event*/
            case EXOSIP_MESSAGE_NEW:
                if (MSG_IS_REGISTER(je->request)) {
                    ua_printf_msg(je, RESPONSE);
                }
                else if (MSG_IS_MESSAGE(je->request)) {
                    ret = -1;
                    ua_printf_msg(je, REQUEST);

                    if (core->registed != REG_PROCESSING && core->registed != REG_OK) {
                        LOGE("recv MESSAGE not registed");
                        break;
                    }

                    char CmdType[32] = {0}, SN[32] = {0}, DeviceID[32] = {0}, SourceDeviceID[32], body[BODY_LEN] = {0};
                    bool is_broadcast = false;
                    osip_body_t* req_body = NULL;
                    osip_message_get_body(je->request, 0, &req_body);
                    get_str(req_body->body, "<CmdType>", false, "</CmdType>", false, CmdType);
                    get_str(req_body->body, "<SN>", false, "</SN>", false, SN);
                    get_str(req_body->body, "<DeviceID>", false, "</DeviceID>", false, DeviceID);
                    LOGD("message CmdType %s, SN %s, DeviceID %s", CmdType, SN, DeviceID);

                    is_broadcast = osip_strcasecmp(CmdType, "Broadcast") == 0;
                    if (CmdType[0] == '\0' || SN[0] == '\0' || (!is_broadcast && DeviceID[0] == '\0')) {
                        LOGE("message %s invalid body", CmdType);
                        break;
                    }

                    do {
                        if (osip_strcasecmp(CmdType, "Catalog") == 0) { // Query
                            ret = ua_catalog_response(SN, DeviceID, body);
                        } else if (osip_strcasecmp(CmdType, "DeviceInfo") == 0) { // Query
                            ret = ua_device_info_response(SN, DeviceID, body);
                        } else if (osip_strcasecmp(CmdType, "ConfigDownload") == 0) { // Query
                            ret = ua_device_config_response(SN, DeviceID, body);
                        } else if (osip_strcasecmp(CmdType, "DeviceStatus") == 0) { // Query
                            ret = ua_device_status_response(SN, DeviceID, body);
                        } else if (osip_strcasecmp(CmdType, "RecordInfo") == 0) { // Query
                            char val[1024];
                            record_info_t rec_info;

                            get_str(req_body->body, "<StartTime>", false, "</StartTime>", false, rec_info.start_time);
                            get_str(req_body->body, "<EndTime>", false, "</EndTime>", false, rec_info.end_time);
                            get_str(req_body->body, "<FilePath>", false, "</FilePath>", false, rec_info.file_path);
                            get_str(req_body->body, "<Address>", false, "</Address>", false, rec_info.address);
                            ret = get_str(req_body->body, "<Secrecy>", false, "</Secrecy>", false, val);
                            if (0 == ret) rec_info.secrecy = atoi(val);
                            ret = get_str(req_body->body, "<Type>", false, "</Type>", false, val);
                            if (0 == ret) rec_info.type = get_record_type(val);
                            get_str(req_body->body, "<RecorderID>", false, "</RecorderID>", false, rec_info.recorder_id);

                            ret = ua_device_record_info_response(SN, DeviceID, &rec_info, body);
                        } else if (is_broadcast) { // Notify
                            char TargetDeviceID[32] = {0};

                            get_str(req_body->body, "<SourceID>", false, "</SourceID>", false, SourceDeviceID);
                            get_str(req_body->body, "<TargetID>", false, "</TargetID>", false, TargetDeviceID);

                            LOGI("recv broadcast from %s to %s", SourceDeviceID, TargetDeviceID);
                            if (osip_strcasecmp(TargetDeviceID, core->config_ua->audio_id) != 0) {
                                LOGE("must be audio id %s", core->config_ua->audio_id);
                                break;
                            }
                            ua_check_task(core, get_cur_time());
                            if (core->voices->size() >= core->config_ua->voice_count) {
                                LOGE("broadcast overuse %d", core->config_ua->voice_count);
                                ret = -1;
                                break;
                            }
                            ret = ua_device_response(CmdType, SN, TargetDeviceID, body);
                        } else if (osip_strcasecmp(CmdType, "DeviceControl") == 0) { // Control
                            char cmd[16] = {0};
                            // 远程启动控制命令(可选)
                            ret = get_str(req_body->body, "<TeleBoot>", false, "</TeleBoot>", false, cmd);
                            if (0 == ret) {
                                if (osip_strcasecmp(cmd, "Boot") == 0) {
                                    LOGI("TeleBoot Boot");
                                    // TODO
                                } else {
                                    LOGE("TeleBoot invalid");
                                    ret = -1;
                                }
                                break;
                            }
                            // 强制关键帧命令,设备收到此命令应立刻发送一个IDR帧(可选)
                            ret = get_str(req_body->body, "<IFameCmd>", false, "</IFameCmd>", false, cmd);
                            if (0 == ret) {
                                if (osip_strcasecmp(cmd, "Send") == 0) {
                                    LOGI("IFameCmd Send");
                                    // TODO
                                } else {
                                    LOGE("IFameCmd invalid");
                                    ret = -1;
                                }
                                break;
                            }
                            // 录像控制命令(可选)
                            ret = get_str(req_body->body, "<RecordCmd>", false, "</RecordCmd>", false, cmd);
                            if (0 == ret) {
                                if (osip_strcasecmp(cmd, "Record") == 0) {
                                    LOGI("Record");
                                    if (!core->is_recording) {
                                        core->is_recording = 1;
                                        if (core->config_ua->on_callback_record != NULL) {
                                            core->config_ua->on_callback_record(1);
                                        }
                                    }
                                } else if (osip_strcasecmp(cmd, "StopRecord") == 0) {
                                    LOGI("StopRecord");
                                    if (core->is_recording) {
                                        core->is_recording = 0;
                                        if (core->config_ua->on_callback_record != NULL) {
                                            core->config_ua->on_callback_record(0);
                                        }
                                    }
                                } else {
                                    LOGE("RecordCmd invalid");
                                    ret = -1;
                                }
                                break;
                            }
                            // 报警复位命令(可选)
                            ret = get_str(req_body->body, "<AlarmCmd>", false, "</AlarmCmd>", false, cmd);
                            if (0 == ret) {
                                if (osip_strcasecmp(cmd, "ResetAlarm") == 0) {
                                    LOGI("AlarmCmd ResetAlarm");

                                    char info[1024] = {0};
                                    ret = get_str(req_body->body, "<Info>", false, "</Info>", false, info);
                                    if (0 == ret) {
                                        char method[8] = {0}, type[8] = {0};
                                        ret = get_str(req_body->body, "<AlarmMethod>", false, "</AlarmMethod>", false, method);
                                        if (0 == ret && strlen(core->alarm.query.method) > 0) {
                                            for (int i = 0; i < strlen(method); i++) {
                                                del_char(core->alarm.query.method, method[i]);
                                            }
                                        }
                                        ret = get_str(req_body->body, "<AlarmType>", false, "</AlarmType>", false, type);
                                        if (0 == ret && strlen(core->alarm.query.type) > 0) {
                                            for (int i = 0; i < strlen(type); i++) {
                                                del_char(core->alarm.query.type, type[i]);
                                            }
                                        }
                                    }
                                    core->alarm.reset = true;
                                    ret = 0;
                                } else {
                                    LOGE("AlarmCmd invalid");
                                    ret = -1;
                                }
                                break;
                            }

                            ret = 0;
                        } else if (osip_strcasecmp(CmdType, "DeviceConfig") == 0) { // Control
                            char name[64] = {0}, expiry[16] = {0}, heartbeat_interval[16] = {0}, heartbeat_count[16] = {0};
                            // BasicParam
                            ret = get_str(req_body->body, "<Name>", false, "</Name>", false, name);
                            if (0 == ret) {
                                strcpy(core->config_ua->name, name);
                            }
                            ret = get_str(req_body->body, "<Expiration>", false, "</Expiration>", false, expiry);
                            if (0 == ret) {
                                int val = atoi(expiry);
                                if (val >= 3600 && val <= 100000) {
                                    core->config_ua->expiry = val;
                                }
                            }
                            ret = get_str(req_body->body, "<HeartBeatInterval>", false, "</HeartBeatInterval>", false, heartbeat_interval);
                            if (0 == ret) {
                                int val = atoi(heartbeat_interval);
                                if (val >= 5 && val <= 255) {
                                    core->config_ua->heartbeat_interval = val;
                                } else {
                                    LOGW("HeartBeatInterval invalid");
                                }
                            }
                            ret = get_str(req_body->body, "<HeartBeatCount>", false, "</HeartBeatCount>", false, heartbeat_count);
                            if (0 == ret) {
                                int val = atoi(heartbeat_count);
                                if (val >= 3 && val <= 255) {
                                    core->config_ua->heartbeat_count = val;
                                } else {
                                    LOGW("HeartBeatCount invalid");
                                }
                            }
                        } else if (osip_strcasecmp(CmdType, "SignatureControl") == 0 ||
                                   osip_strcasecmp(CmdType, "EncryptionControl") == 0) { // Control
                            if (!core->config_ua->gm_level) {
                                LOGE("unsupported %s for no gm device", CmdType);
                                break;
                            }
                            char cmd[16] = {0};
                            ret = get_str(req_body->body, "<ControlCmd>", false, "</ControlCmd>", false, cmd);
                            if (0 != ret || strlen(cmd) == 0) {
                                LOGE("%s ControlCmd invalid", CmdType);
                                break;
                            }
                            bool is_sign_ctrl = (osip_strcasecmp(CmdType, "SignatureControl") == 0);
                            bool is_start = (osip_strcasecmp(cmd, "Start") == 0);
                            if (is_sign_ctrl) {
                                core->gm.signatured = is_start;
                                if (core->config_ua->on_callback_gm_signature != NULL) {
                                    core->config_ua->on_callback_gm_signature(is_start);
                                }
                            } else {
                                core->gm.encrypted = is_start;
                                if (core->config_ua->on_callback_gm_encrypt != NULL) {
                                    core->config_ua->on_callback_gm_encrypt(is_start);
                                }
                            }
                            ret = ua_device_response(CmdType, SN, DeviceID, body);
                        } else {
                            LOGW("not implemented message %s", CmdType);
                        }
                    } while (false);

                    if (ret == 0) {
                        ret = ua_msg_send_response(core, je);
                    } else {
                        ret = ua_msg_send_response(core, je, NULL, 400);
                        break;
                    }

                    if (ret == 0 && strlen(body) > 0) {
                        osip_message_t *msg = NULL;
                        ret = eXosip_message_build_request(core->context, &msg, "MESSAGE", core->to, core->from, core->proxy);
                        osip_message_set_body(msg, body, strlen(body));
                        osip_message_set_content_type(msg, SIP_TYPE_XML);
                        ////ua_gm_add_note_header(core, "MESSAGE", msg);
                        ua_add_outboundproxy(msg, core->outboundproxy);
                        ret = eXosip_message_send_request(core->context, msg);
                        if (ret > 0) {
                            LOGI("send message %s request ok, %s", CmdType, msg->call_id->number);
                            if (is_broadcast) {
                                broadcast_t bc;
                                strcpy(bc.src_device_id, SourceDeviceID);
                                osip_call_id_clone(msg->call_id, &bc.msg_cid);
                                core->voices->push_back(bc);
                                LOGI("broadcast message callid %s@%s", msg->call_id->host, msg->call_id->number);
                            }
                            ret = OSIP_SUCCESS;
                        }
                    }
                }
                break;
            case EXOSIP_MESSAGE_PROCEEDING:
                LOGI("msg proceeding %d %d %d", je->cid, je->tid, je->did);
                break;
            case EXOSIP_MESSAGE_ANSWERED: {
                LOGI("msg answered %d %d %d", je->cid, je->tid, je->did);
                if (MSG_IS_MESSAGE(je->request)) {
                    ret = -1;
                    if (0 == osip_call_id_match(je->request->call_id, core->keepalive_cid)) {
                        LOGI("keepalive ok");
                        core->keepalive_retries = 0;
                    } else {
                        broadcast_t* broadcast = NULL;
                        for (auto &it : *core->voices) {
                            if (0 == osip_call_id_match(je->request->call_id, it.msg_cid)) {
                                broadcast = &it;
                                break;
                            }
                        }
                        if (NULL != broadcast) {
                            char From[128] = {0}, To[128] = {0}, Subject[128] = {0}, sdp[BODY_LEN], y[16];
                            osip_message_t *msg = NULL;
                            int local_port = get_free_port(core, broadcast->transport == ua_transport::UDP ? IPPROTO_UDP : IPPROTO_TCP);

                            LOGI("broadcast message ok, %s@%s", je->request->call_id->host, je->request->call_id->number);

                            y[0] = '0';
                            strncpy(&y[1], &core->config_ua->username[3], 5); // substring(3, 8);
                            get_streamid(&y[6], sizeof(y) - 6);

                            sprintf(From, "sip:%s@%s:%d", core->config_ua->username, core->localip, core->config_ua->local_port);
                            sprintf(To, "sip:%s@%s:%d", broadcast->src_device_id, core->config_ua->server_ip, core->config_ua->server_port);
                            sprintf(Subject, "%s:%s,%s:%s", broadcast->src_device_id, y, core->config_ua->username, "1");

                            sdp[0] = '\0';
                            snprintf(sdp, sizeof(sdp),
                                     "v=0" STR_0D0A
                                     "o=%s 0 0 IN IP4 %s" STR_0D0A
                                     "s=Play" STR_0D0A
                                     "c=IN IP4 %s" STR_0D0A
                                     "t=0 0" STR_0D0A
                                     "m=audio %d RTP/AVP 8 96" STR_0D0A
                                     "a=recvonly" STR_0D0A
                                     "a=rtpmap:8 PCMA/8000" STR_0D0A
                                     "a=rtpmap:96 PS/90000" STR_0D0A
                                     "y=%s" STR_0D0A
                                     "f=v/////a/1/8/1" STR_0D0A,
                                     core->config_ua->username, core->localip, core->localip, local_port, y);

                            // send invite to sip server (proxy)
                            ret = ua_cmd_call_start(core, From, To, core->proxy, Subject, sdp, &msg);
                            if (ret < 0) {
                                ua_release_call(core, je);
                                break;
                            }
                            je->cid = ret;
                            ret = 0;
                            LOGI("send broadcast invite callid %s@%s, %d", msg->call_id->host, msg->call_id->number, je->cid);

                            task_t* task = ua_notify_callid(core, je, true, true);
                            if (NULL == task) {
                                LOGE("no broadcast task");
                                ua_release_call(core, je);
                                break;
                            }
                            task->tk_type = task_type::TT_VOICE;
                            task->transport = broadcast->transport;
                            task->local_port = local_port;
                            task->opaque = broadcast;
                            strcpy(task->y, y);
                        }
                    }
                }
                break;
            }
            case EXOSIP_MESSAGE_REQUESTFAILURE:
                LOGW("msg request failed %d %d %d", je->cid, je->tid, je->did);
                if (MSG_IS_MESSAGE(je->request)) {
                    if (0 == osip_call_id_match(je->request->call_id, core->keepalive_cid)) {
                        LOGW("keepalive retry %d", ++core->keepalive_retries);
                        if (core->keepalive_retries > core->config_ua->heartbeat_count) {
                            core->cmd = UA_CMD_UNREGISTER;
                            ua_cmd_unregister(core, 0, 0);
                            if (core->config_ua->on_callback_status != NULL) {
                                core->config_ua->on_callback_status(EVENT_DISCONNECT, 0, NULL, 0);
                            }
                        }
                    }
                }
                break;

            case EXOSIP_IN_SUBSCRIPTION_NEW: {
                if (core->registed != REG_PROCESSING && core->registed != REG_OK) {
                    LOGE("recv SUBSCRIBE not registed");
                    break;
                }

                if (MSG_IS_SUBSCRIBE(je->request)) {
                    ret = -1;
                    ua_printf_msg(je, REQUEST);
                    char CmdType[64] = {0}, SN[64] = {0}, DeviceID[32] = {0}, TeleBoot[64] = {0}, body[BODY_LEN] = {0};
                    subscribe_type type;

                    osip_body_t* req_body = NULL;
                    osip_message_get_body(je->request, 0, &req_body);
                    get_str(req_body->body, "<CmdType>", false, "</CmdType>", false, CmdType);
                    get_str(req_body->body, "<SN>", false, "</SN>", false, SN);
                    get_str(req_body->body, "<DeviceID>", false, "</DeviceID>", false, DeviceID);
                    LOGI("subscribe CmdType %s, SN %s, DeviceID %s, %d %d", CmdType, SN, DeviceID, je->tid, je->did);

                    do {
                        osip_header_t *header = NULL;
                        ret = osip_message_get_expires(je->request, 0, &header);
                        if (ret < 0) {
                            LOGE("not found expires header %d", ret);
                            break;
                        }

                        int expiry = atoi(header->hvalue);
                        if (osip_strcasecmp(CmdType, "Catalog") == 0) {
                            type = SUBSCRIBE_CATALOG;
                            core->catalog.subscribe.expiry = expiry;
                            ret = ua_device_response(CmdType, SN, DeviceID, body);
                        } else if (osip_strcasecmp(CmdType, "MobilePosition") == 0) {
                            type = SUBSCRIBE_MOBILEPOS;
                            core->mobilepos.subscribe.expiry = expiry;
                            ret = 0;
                        } else if (osip_strcasecmp(CmdType, "Alarm") == 0) { // Subscribe Query
                            type = SUBSCRIBE_ALARM;

                            get_str(req_body->body, "<StartAlarmPriority>", false, "</StartAlarmPriority>", false, core->alarm.query.start_priority);
                            get_str(req_body->body, "<EndAlarmPriority>", false, "</EndAlarmPriority>", false, core->alarm.query.end_priority);
                            get_str(req_body->body, "<AlarmMethod>", false, "</AlarmMethod>", false, core->alarm.query.method);
                            get_str(req_body->body, "<AlarmType>", false, "</AlarmType>", false, core->alarm.query.type);
                            get_str(req_body->body, "<StartAlarmTime>", false, "</StartAlarmTime>", false, core->alarm.query.start_time);
                            get_str(req_body->body, "<EndAlarmTime>", false, "</EndAlarmTime>", false, core->alarm.query.end_time);

                            core->alarm.subscribe.expiry = expiry;
                            ret = ua_device_response(CmdType, SN, DeviceID, body);
                        } else {
                            LOGW("not implemented subscription %s", CmdType);
                        }
                    } while (0);

                    if (osip_strcasecmp(DeviceID, core->config_ua->username) != 0) {
                        LOGW("mismatch device id %s with %s, %d", DeviceID, core->config_ua->username, je->tid);
                    }

                    if (ret == 0) {
                        ret = ua_subscribe_response(core, je, body, 200);
                    } else {
                        // 481 call/transaction does not exist 呼叫/事务不存在
                        // 487 request terminated 请求终止
                        ret = ua_subscribe_response(core, je, body, 487);
                        break;
                    }

                    if (ret == 0) {
                        if (SUBSCRIBE_CATALOG == type) {
                            subscribe_t* subscribe = &core->catalog.subscribe;
                            if (subscribe->expiry > 0 && subscribe->did <= 0) {
                                subscribe->interval = 5; // 随机指定一个间隔
                                subscribe->did = je->did;
                                ret = pthread_create(&subscribe->thread, NULL, ua_catalog_thread, core);
                            } else if (subscribe->expiry <= 0) {
                                pthread_kill(subscribe->thread, SIGINT);
                            }
                        } else if (SUBSCRIBE_MOBILEPOS == type) {
                            subscribe_t* subscribe = &core->mobilepos.subscribe;
                            if (subscribe->expiry > 0 && subscribe->did <= 0) {
                                char Interval[8] = {0};
                                get_str(req_body->body, "<Interval>", false, "</Interval>", false, Interval);

                                subscribe->interval = atoi(Interval);
                                subscribe->did = je->did;
                                ret = pthread_create(&subscribe->thread, NULL, ua_pos_thread, core);
                            } else if (subscribe->expiry <= 0) {
                                pthread_kill(subscribe->thread, SIGINT);
                            }
                        } else if (SUBSCRIBE_ALARM == type) {
                            subscribe_t* subscribe = &core->alarm.subscribe;
                            if (subscribe->expiry > 0 && subscribe->did <= 0) {
                                subscribe->interval = 2; // 随机指定一个间隔
                                subscribe->did = je->did;
                                core->alarm.duty_status = alarm_duty_status::ALARM_STATUS_ONDUTY;
                                ret = pthread_create(&subscribe->thread, NULL, ua_alarm_thread, core);
                            } else if (subscribe->expiry <= 0) {
                                core->alarm.duty_status = alarm_duty_status::ALARM_STATUS_OFFDUTY;
                                pthread_kill(subscribe->thread, SIGINT);
                            }
                        }
                    } else {
                        LOGE("subscribe response failed, %d, %d %d", ret,  je->tid, je->did);
                    }
                }
                break;
            }
            case EXOSIP_SUBSCRIPTION_NOTIFY:
                LOGD("subscribe notify %d %d", je->tid, je->did);
                break;
            case EXOSIP_NOTIFICATION_PROCEEDING:
                LOGD("notify proceeding %d %d", je->tid, je->did);
                break;
            case EXOSIP_NOTIFICATION_ANSWERED:
                LOGD("notify answered %d %d", je->tid, je->did);
                break;

            default:
                LOGI("recv unknown msg %d\n", je->type);
                break;
        }
        eXosip_event_free(je);
        eXosip_unlock(core->context);
    }

    pthread_detach(pthread_self());
    return 0;
}

int ua_init(ua_config_t* config)
{
    int ret = -1;
    int64_t cur_ts = get_cur_time();
    if (config == NULL) {
        LOGE("config is null");
        return ret;
    }
    LOGI("%s %s uainit gm level %d, %lld", UA_VERSION, SDK_VERSION, config->gm_level, cur_ts);
    if (use_duration > 0 && cur_ts > start_ts + use_duration) return -1;

#ifdef NO_USE_GM
    if (config->gm_level > 0) {
        LOGE("unsupported gm");
        return ret;
    }
#endif

    //TRACE_INITIALIZE(TRACE_LEVEL7, NULL);

    bool changed = true;
    ua_config_t* pconfig = g_core.config_ua;

    // 如果配置没变则直接返回,如果配置变化则重新初始化
    if (pconfig != NULL &&
        osip_strcasecmp(pconfig->server_ip, config->server_ip) == 0 &&
        pconfig->server_port == config->server_port &&
        osip_strcasecmp(pconfig->server_id, config->server_id) == 0 &&
        osip_strcasecmp(pconfig->server_domain, config->server_domain) == 0 &&
        osip_strcasecmp(pconfig->username, config->username) == 0 &&
        pconfig->local_port == config->local_port &&
        osip_strcasecmp(pconfig->protocol, config->protocol) == 0) {
        changed = false;
    }

    // 如果配置变化则重新初始化
    if (changed || pconfig == NULL) {
        ua_uninit();
        init(&g_core);
    }

    pconfig = g_core.config_ua;
    pconfig->gm_level = config->gm_level;

    if (g_core.context == NULL) {
        int val = 1;
        g_core.context = eXosip_malloc();
        if (eXosip_init(g_core.context)) {
            LOGE("init ua failed");
            goto error;
        }

        if (osip_strcasecmp(config->protocol, "UDP") == 0) {
            ret = eXosip_listen_addr(g_core.context, IPPROTO_UDP, NULL, config->local_port, AF_INET, 0);
        } else if (osip_strcasecmp(config->protocol, "TCP") == 0) {
            ret = eXosip_listen_addr(g_core.context, IPPROTO_TCP, NULL, config->local_port, AF_INET, 0);
        } else if (osip_strcasecmp(config->protocol, "TLS") == 0) {
            ret = eXosip_listen_addr(g_core.context, IPPROTO_TCP, NULL, config->local_port, AF_INET, 1);
        } else if (osip_strcasecmp(config->protocol, "DTLS") == 0) {
            ret = eXosip_listen_addr(g_core.context, IPPROTO_UDP, NULL, config->local_port, AF_INET, 1);
        } else {
            LOGE("wrong transport protocol %s", config->protocol);
            ret = -1;
        }
        if (ret) {
            LOGE("listen %d failed", config->local_port);
            goto error;
        }

        eXosip_set_user_agent(g_core.context, UA_VERSION);
        eXosip_set_option(g_core.context, EXOSIP_OPT_USE_RPORT, (void *) &val);
    }

    gm_init();
    eXosip_set_option(g_core.context, EXOSIP_OPT_REGISTER_WITH_DATE, (void *) &pconfig->gm_level);
    eXosip_set_option(g_core.context, EXOSIP_OPT_REGISTER_GM_LEVEL, (void *) &pconfig->gm_level);
    if (pconfig->gm_level > 0) {
        if (NULL == g_core.gm.device_ec_prikey) {
            g_core.gm.device_ec_prikey = gm_get_ecprikey(CERT_KEY_FILE, 1, g_core.gm.device_key_pass);
        }
    }

#if FIXED_COPYRIGHT
    if (NULL == config->name) strcpy(pconfig->name, "NanGBD");
    if (NULL == config->manufacturer) strcpy(pconfig->manufacturer, "nanuns");
    if (NULL == config->model) strcpy(pconfig->model, "");
    if (NULL == config->firmware) strcpy(pconfig->firmware, "");
    if (NULL == config->serial_number) strcpy(pconfig->serial_number, "");
#endif

    if (osip_strcasecmp(pconfig->server_ip, config->server_ip) != 0) {
        strcpy(pconfig->server_ip, config->server_ip);
    }
    pconfig->server_port = config->server_port;
    if (osip_strcasecmp(pconfig->server_id, config->server_id) != 0) {
        strcpy(pconfig->server_id, config->server_id);
    }
    if (osip_strcasecmp(pconfig->server_domain, config->server_domain) != 0) {
        strcpy(pconfig->server_domain, config->server_domain);
    }
    if (NULL != config->name && osip_strcasecmp(pconfig->name, config->name) != 0) {
        strcpy(pconfig->name, config->name);
    }
    if (NULL != config->manufacturer && osip_strcasecmp(pconfig->manufacturer, config->manufacturer) != 0) {
        strcpy(pconfig->manufacturer, config->manufacturer);
    }
    if (NULL != config->model && osip_strcasecmp(pconfig->model, config->model) != 0) {
        strcpy(pconfig->model, config->model);
    }
    if (NULL != config->firmware && osip_strcasecmp(pconfig->firmware, config->firmware) != 0) {
        strcpy(pconfig->firmware, config->firmware);
    }
    if (NULL != config->serial_number && osip_strcasecmp(pconfig->serial_number, config->serial_number) != 0) {
        strcpy(pconfig->serial_number, config->serial_number);
    }
    pconfig->channel = config->channel;

    if (osip_strcasecmp(pconfig->username, config->username) != 0 ||
        osip_strcasecmp(pconfig->password, config->password) != 0 ||
        osip_strcasecmp(pconfig->userid, config->userid) != 0) {
        strcpy(pconfig->username, config->username);
        strcpy(pconfig->password, config->password);
        strcpy(pconfig->userid, config->userid);

        LOGD("username: %s", pconfig->username);
        ret = ua_add_auth_info(&g_core); // 必须
        if (ret != 0) {
            LOGE("add auth info failed, %d", ret);
            goto error;
        }
    }
    pconfig->local_port = config->local_port;
    if (osip_strcasecmp(pconfig->protocol, config->protocol) != 0) {
        strcpy(pconfig->protocol, config->protocol);
    }
    pconfig->expiry = config->expiry;
    pconfig->heartbeat_interval = config->heartbeat_interval;
    pconfig->heartbeat_count = config->heartbeat_count;

    pconfig->longitude = config->longitude;
    pconfig->latitude = config->latitude;
    if (config->longitude != -99999 && config->latitude != -99999) {
        ua_mobilepos_t mp = g_core.mobilepos.pos.load();
        mp.longitude = config->longitude;
        mp.latitude = config->latitude;
        g_core.mobilepos.pos.store(mp);
    }

    if (osip_strcasecmp(pconfig->video_id, config->video_id) != 0) {
        strcpy(pconfig->video_id, config->video_id);
    }
    if (osip_strcasecmp(pconfig->audio_id, config->audio_id) != 0) {
        strcpy(pconfig->audio_id, config->audio_id);
    }
    if (osip_strcasecmp(pconfig->alarm_id, config->alarm_id) != 0) {
        strcpy(pconfig->alarm_id, config->alarm_id);
    }
    strcpy(pconfig->charset, config->charset);
    pconfig->invite_count = config->invite_count;
    pconfig->voice_count = config->voice_count;
    pconfig->global_auth = config->global_auth;

    pconfig->on_callback_status = config->on_callback_status;
    pconfig->on_callback_invite = config->on_callback_invite;
    pconfig->on_callback_cancel = config->on_callback_cancel;

    eXosip_guess_localip(g_core.context, AF_INET, g_core.localip, DEVICE_DESC_LEN);

    if (null_if_empty(pconfig->server_domain)) {
        sprintf(g_core.proxy, "sip:%s@%s", pconfig->server_id, pconfig->server_domain);
        sprintf(g_core.from, "sip:%s@%s", pconfig->username, pconfig->server_domain);
        sprintf(g_core.to, "sip:%s@%s", pconfig->server_id, pconfig->server_domain);
    } else {
        sprintf(g_core.proxy, "sip:%s@%s:%d", pconfig->server_id, pconfig->server_ip, pconfig->server_port);
        sprintf(g_core.from, "sip:%s@%s:%d", pconfig->username, g_core.localip, pconfig->local_port);
        sprintf(g_core.to, "sip:%s@%s:%d", pconfig->server_id, pconfig->server_ip, pconfig->server_port);
    }
    sprintf(g_core.outboundproxy, "sip:%s@%s:%d", pconfig->server_id, pconfig->server_ip, pconfig->server_port);
    // 这个地方如果加上的话，可能导致_eXosip_register_contact_is_modified RS_DELETIONREQUIRED多次发送注册消息
    //sprintf(g_core.contact, "sip:%s@%s:%d", pconfig->username, g_core.localip, pconfig->local_port);

    if (g_core.localip) {
        LOGI("local address: %s:%d", g_core.localip, pconfig->local_port);
    }
    if (null_if_empty(g_core.firewallip)) {
        LOGI("firewall address: %s:%i", g_core.firewallip, pconfig->local_port);
        eXosip_masquerade_contact(g_core.context, g_core.firewallip, pconfig->local_port);
    }

    LOGI("proxy: %s, outboundproxy: %s", g_core.proxy, g_core.outboundproxy);
    LOGI("from: %s, to: %s, contact: %s, protocol: %s", g_core.from, g_core.to, g_core.contact, pconfig->protocol);

    signal(SIGINT, quit_handler);
    ret = 0;
    if (g_core.running != 1) {
        LOGI("UAC startup...");
        g_core.running = 1;
        ret = pthread_create(&g_core.ua_thread_keepalive, NULL, ua_keepalive_thread, &g_core);
        if (ret) {
            LOGE("start keepalive failed %d", ret);
            goto error;
        }
        ret = pthread_create(&g_core.ua_thread_event, NULL, ua_event_thread, &g_core);
        if (ret) {
            LOGE("start event failed %d", ret);
            goto error;
        }
    }
    g_config_ua = g_core.config_ua;
    g_gm = &g_core.gm;
    return ret;
error:
    ua_uninit();
    return ret;
}

int ua_register()
{
    int ret = -1;
    if (g_core.context == NULL) {
        return ret;
    }

    g_core.cmd = UA_CMD_REGISTER;
    ret = ua_cmd_register(&g_core);
    return ret;
}

int ua_unregister()
{
    int ret;
    if (g_core.context == NULL) {
        return 0;
    }
    g_core.cmd = UA_CMD_UNREGISTER;
    ret = ua_cmd_unregister(&g_core);
    return ret;
}

int ua_isregistered()
{
    return (g_core.registed == REG_OK);
}

void ua_on_config_changed(int changed)
{
    g_core.config_changed = changed;
}

int ua_get_device_cert(char cert[], int size)
{
    FILE *fp = fopen(CERT_PUB_FILE, "r");
    if (NULL == fp) {
        return -1;
    }

    int ret = fread(cert, 1, size - 1, fp);
    if (ret <= 0) {
        LOGE("get device cert failed");
        return -1;
    }
    cert[ret] = '\0';
    return 0;
}

int ua_set_server_cert(const char* server_id, const char* cert, int is_file, char* passin)
{
    X509 *x509_cert;
    EVP_PKEY *pkey;

    x509_cert = gm_get_x509(cert, is_file, passin);
    if (NULL == x509_cert) {
        return -1;
    }
    pkey = X509_get_pubkey(x509_cert);
    if (NULL == pkey) {
        LOGE("get pubkey from cert failed");
        X509_free(x509_cert);
        return -1;
    }

    if (NULL != g_core.context) {
        eXosip_lock(g_core.context);
    }
    if (NULL != g_core.gm.server_cert) {
        X509_free(g_core.gm.server_cert);
    }
    if (NULL != g_core.gm.server_pkey) {
        EVP_PKEY_free(g_core.gm.server_pkey);
    }
    g_core.gm.server_cert = x509_cert;
    g_core.gm.server_ec_pubkey = EVP_PKEY_get0_EC_KEY(pkey);
    g_core.gm.server_pkey = pkey;
    if (NULL != g_core.context) {
        eXosip_unlock(g_core.context);
    }

    LOGI("set server %s cert ok", server_id);
    return 0;
}

int ua_call_stop(int cid)
{
    LOGI("call stop %d", cid);
    if (g_core.context == NULL) {
        return -1;
    }
    eXosip_lock(g_core.context);
    auto it = g_core.tasks->find(cid);
    if (it != g_core.tasks->end()) {
        task_t* task = &it->second;
        ua_cmd_call_stop(&g_core, task);
        if (task->tk_type == task_type::TT_VOICE) {
            if (NULL != task->opaque) {
                broadcast_t* broadcast = (broadcast_t*) task->opaque;
                ua_release_broadcast(&g_core, broadcast);
            }
        }
        g_core.tasks->erase(it);
        LOGI("remove task %d", cid);
    }
    eXosip_unlock(g_core.context);
    return 0;
}

int ua_set_mobilepos(ua_mobilepos_t* mobilepos)
{
    if (NULL == mobilepos) {
        return -1;
    }

    ua_mobilepos_t mp = {0};
    strcpy((char* const) mp.time, mobilepos->time);
    mp.longitude = mobilepos->longitude;
    mp.latitude = mobilepos->latitude;
    mp.speed = mobilepos->speed;
    mp.direction = mobilepos->direction;
    mp.altitude = mobilepos->altitude;
    g_core.mobilepos.pos.store(mp);

    g_config_ua->longitude = mobilepos->longitude;
    g_config_ua->latitude = mobilepos->latitude;

    return 0;
}

int ua_set_alarm(ua_alarm_t* alarm)
{
    if (NULL == alarm) {
        return -1;
    }

    if (g_core.alarm.subscribe.expiry == 0) {
        // 未被订阅添加无效
        return -1;
    }

    std::unique_lock<std::mutex> lock(g_core.alarm.lock);
    if (NULL == g_core.alarm.alarms) {
        return -1;
    }
    g_core.alarm.alarms->push_back(*alarm);
    return 0;
}

int ua_set_rec_path(const char* path)
{
    free(g_core.rec_path);
    g_core.rec_path = strdup(path);
    return 0;
}

void ua_uninit()
{
    LOGI("ua uninit");
    ua_unregister();

    g_core.running = 0;
    if (g_core.context != NULL) {
        pthread_join(g_core.ua_thread_event, NULL);
    }
    if (g_core.registed == REG_OK) {
        pthread_join(g_core.ua_thread_keepalive, NULL);
    }
    quit(&g_core);
}
