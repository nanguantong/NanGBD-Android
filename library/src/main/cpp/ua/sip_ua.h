//
// Created by nanuns on 2020/6/7.
//

#ifndef NANGBD_SIP_UA_H
#define NANGBD_SIP_UA_H

#include "ua_config.h"

//class SipUa {
//public:
int   ua_init(ua_config_t* config);
int   ua_register();
int   ua_unregister();
int   ua_isregistered();
void  ua_on_config_changed(int changed);
int   ua_get_device_cert(char cert[], int size);
int   ua_set_server_cert(const char* server_id, const char* cert, int is_file, char* passin);
int   ua_call_stop(int cid);
int   ua_set_mobilepos(ua_mobilepos_t* mobilepos);
int   ua_set_alarm(ua_alarm_t* alarm);
int   ua_set_rec_path(const char* path);
void  ua_uninit();
//};

#endif // NANGBD_SIP_UA_H
