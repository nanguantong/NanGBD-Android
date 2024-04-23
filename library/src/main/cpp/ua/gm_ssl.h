//
// Created by nanuns on 2021/7/28.
//

#ifndef NANGBD_GM_SSL_H
#define NANGBD_GM_SSL_H

extern "C"
{
#include <libavutil/base64.h>
}
#include <openssl/sm2.h>

#ifndef SM3_DIGEST_LENGTH
#define SM3_DIGEST_LENGTH 32
#endif
#define GM_ALGORITHM "A:SM2;H:SM3;S:SM1/OFB/PKCS5,SM1/CBC/PKCS5,SM4/OFB/PKCS5,SM4/CBC/PKCS5;SI:SM3-SM2"

typedef struct gm {
    bool unidirection;
    char algorithm[256]; // sip server algorithm
    char keyversion[32]; // 2020-05-15T11:45:36
    char random1[64]; // 只能一次+时效性
    char random2[64]; // 只能一次+时效性
    char sign1[1024]; // device sm2 private-key sign
    char sign2[1024]; // server sm2 private-key sign
    // VKEK由视频监控安全管理平台产生并分发给设备
    // VKEK在设备注册时更新, 前端设备的VKEK保存周期应满足历史视频保存时间的要求
    // VKEK更新周期不大于1天, VEK更新周期不大于1h
    char vkek[128];
    char cryptkey[512];
    char *device_key_pass;
    EC_KEY *device_ec_prikey;
    EC_KEY *server_ec_pubkey;
    EVP_PKEY *server_pkey;
    X509 *server_cert;

    bool signatured, encrypted;
} gm_t;

int gm_init();
X509* gm_get_x509(const char* buf, int is_file, char* passin);
EC_KEY* gm_get_ecpubkey(const char* buf, int is_file, char* passin);
EC_KEY* gm_get_ecprikey(const char* buf, int is_file, char* passin);
EC_KEY* gm_get_sm2_eckey(const char* path);
EC_KEY* gm_gen_sm2_eckey(const char* path, char* pub, char* pri);
int gm_sm2_sign(EC_KEY *ec_key, const unsigned char* buf, int buf_len, unsigned char *sig, int& sign_len);
int gm_sm2_verify(EC_KEY *ec_key, const unsigned char* buf, int buf_len, const unsigned char *sig, int sign_len);
int gm_sm2_encrypt(EC_KEY *ec_key, const unsigned char* buf, int buf_len, unsigned char* out, int& out_len);
int gm_sm2_decrypt(EC_KEY *ec_key, const unsigned char* buf, int buf_len, unsigned char* out, int& out_len);
int gm_sm3_hash(const unsigned char* buf, int buf_len, unsigned char* dgst, int& dgst_len);
int gm_sm4_encrypt(const unsigned char* key, const unsigned char* buf, int buf_len, unsigned char* out, int& out_len);
int gm_sm4_decrypt(const unsigned char* key, const unsigned char* buf, int buf_len, unsigned char* out, int& out_len);

#endif // NANGBD_GM_SSL_H
