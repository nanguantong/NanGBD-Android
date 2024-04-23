//
// Created by nanuns on 2021/7/28.
//

#include <openssl/sm2.h>
#include <openssl/sms4.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include "../utils/util.h"
#include "../utils/log.h"
#include "gm_ssl.h"

using namespace std;

int gm_init()
{
    ERR_load_ERR_strings();
    ERR_load_crypto_strings();
    return 0;
}

X509* gm_get_x509(const char* buf, int is_file, char* passin)
{
    BIO *in = NULL;
    X509 *cert;

    if (NULL == buf || buf[0] == '\0') {
        return NULL;
    }
    if (is_file) {
        in = BIO_new_file(buf, "r");
    } else {
        in = BIO_new_mem_buf(buf, strlen(buf));
    }
    if (NULL == in) {
        return NULL;
    }

    //req = PEM_read_bio_X509_REQ(in, NULL, NULL, NULL);
    cert = PEM_read_bio_X509(in, NULL, NULL, passin);
    if (NULL == cert) {
        LOGE("get x509 cert failed");
    } else {
#if 0
        STACK_OF(X509) *certs = NULL;
        certs = sk_X509_new_null();
        if (!sk_X509_push(certs, cert)) {
            LOGE("push x509 cert failed");
            sk_X509_pop_free(certs, X509_free);
            return NULL;
        }
#endif
    }

    BIO_free(in);
    return cert;
}

EC_KEY* gm_get_ecpubkey(const char* buf, int is_file, char* passin)
{
    BIO *in = NULL;
    EC_KEY *eckey;

    if (NULL == buf || buf[0] == '\0') {
        return NULL;
    }
    if (is_file) {
        in = BIO_new_file(buf, "r");
    } else {
        in = BIO_new_mem_buf(buf, strlen(buf));
    }
    if (NULL == in) {
        return NULL;
    }

    eckey = PEM_read_bio_EC_PUBKEY(in, NULL, NULL, passin);
    if (NULL == eckey) {
        LOGE("get ec pubkey failed");
    }

    BIO_free(in);
    return eckey;
}

EC_KEY* gm_get_ecprikey(const char* buf, int is_file, char* passin)
{
    BIO *in = NULL;
    EC_KEY *eckey;

    if (NULL == buf || buf[0] == '\0') {
        return NULL;
    }
    if (is_file) {
        in = BIO_new_file(buf, "r");
    } else {
        in = BIO_new_mem_buf(buf, strlen(buf));
    }
    if (NULL == in) {
        return NULL;
    }

    eckey = PEM_read_bio_ECPrivateKey(in, NULL, NULL, passin);
    if (NULL == eckey) {
        LOGE("get ec prikey failed");
    }

    BIO_free(in);
    return eckey;
}

EC_KEY* gm_get_sm2_eckey(const char* path)
{
    int ret;
    string pri_path = path;
    pri_path.append("/private");

    string pub_path = path;
    pub_path.append("/public");

    char *hex_pri = NULL, *hex_pub = NULL;
    BN_CTX *bn_ctx = NULL;
    BIGNUM *bn_prikey = NULL;
    EC_POINT *pubkey_point = NULL;
    EC_KEY *ec_key = NULL;

    hex_pri = (char *) malloc(1024);
    memset(hex_pri, 0, 1024);
    ret = read_file((char *) pri_path.c_str(), hex_pri, 1024);
    if (ret != 0) {
        goto clear;
    }

    hex_pub = (char *) malloc(1024);
    memset(hex_pub, 0, 1024);
    ret = read_file((char *) pub_path.c_str(), hex_pub, 1024);
    if (ret != 0) {
        goto clear;
    }

    ec_key = EC_KEY_new_by_curve_name(NID_sm2p256v1);
    bn_ctx = BN_CTX_new();
    pubkey_point = EC_POINT_hex2point(EC_KEY_get0_group(ec_key), hex_pub, NULL, bn_ctx);
    if (NULL == pubkey_point) {
        LOGE("hex2point failed: %s", ERR_reason_error_string(ERR_get_error()));
        goto clear;
    }
    ret = EC_KEY_set_public_key(ec_key, pubkey_point);
    if (!ret) {
        LOGE("set public key failed");
        ret = -1;
        goto clear;
    }

    bn_prikey = BN_new();
    ret = BN_hex2bn(&bn_prikey, hex_pri);
    if (NULL == bn_prikey) {
        LOGE("hex2bn failed: %s", ERR_reason_error_string(ERR_get_error()));
        ret = -1;
        goto clear;
    }
    ret = EC_KEY_set_private_key(ec_key, bn_prikey);
    if (ret != 0) {
        LOGE("set private key failed: %s", ERR_reason_error_string(ERR_get_error()));
        ret = -1;
        goto clear;
    }

clear:
    free(hex_pri);
    free(hex_pub);
    BN_free(bn_prikey);
    EC_POINT_free(pubkey_point);
    BN_CTX_free(bn_ctx);
    if (ret != 0) {
        EC_KEY_free(ec_key);
        ec_key = NULL;
    }
    return ec_key;
}

EC_KEY* gm_gen_sm2_eckey(const char* path, char* pub, char* pri)
{
    int ret;
    string pri_path = path;
    pri_path.append("/private");
    LOGI("private key path: %s", pri_path.c_str());

    string pub_path = path;
    pub_path.append("/public");
    LOGI("public key path: %s", pub_path.c_str());

    char *hex_pri = NULL, *hex_pub = NULL;
    BN_CTX *bn_ctx = NULL;

    EC_KEY *ec_key = EC_KEY_new_by_curve_name(NID_sm2p256v1);
    ret = EC_KEY_generate_key(ec_key);
    if (!ret) {
        LOGE("unable to generate key: %s", ERR_reason_error_string(ERR_get_error()));
        ret = -1;
        goto clear;
    }
    ret = -1;
    if (!EC_KEY_check_key(ec_key)) {
        LOGE("SM2 Key invalid");
        goto clear;
    }
    bn_ctx = BN_CTX_new();
    hex_pub = EC_POINT_point2hex(EC_KEY_get0_group(ec_key), EC_KEY_get0_public_key(ec_key),
                                 POINT_CONVERSION_UNCOMPRESSED, bn_ctx);
    if (NULL == hex_pub) {
        LOGE("point2hex failed: %s", ERR_reason_error_string(ERR_get_error()));
        goto clear;
    }
    hex_pri = BN_bn2hex(EC_KEY_get0_private_key(ec_key));
    if (NULL == hex_pri) {
        LOGE("bn2hex failed: %s", ERR_reason_error_string(ERR_get_error()));
        goto clear;
    }

    ret = write_file(pub_path.c_str(), hex_pub, strlen(hex_pub));
    LOGI("public key create result: %d", ret);
    ret = write_file(pri_path.c_str(), hex_pri, strlen(hex_pri));
    LOGI("private key create result: %d", ret);

    if (NULL != pub) {
        strcpy(pub, hex_pub);
    }
    if (NULL != pri) {
        strcpy(pri, hex_pri);
    }

clear:
    OPENSSL_free(hex_pub);
    OPENSSL_free(hex_pri);
    BN_CTX_free(bn_ctx);
    if (ret != 0) {
        EC_KEY_free(ec_key);
        ec_key = NULL;
    }
    return ec_key;
}

int gm_sm2_sign(EC_KEY *ec_key, const unsigned char* buf, int buf_len, unsigned char* out, int& out_len)
{
    int ret;
    unsigned char dgst[EVP_MAX_MD_SIZE];
    size_t dgst_len = sizeof(dgst);
    out_len = 0;

    ret = SM2_compute_message_digest(EVP_sm3(), EVP_sm3(), buf, buf_len,
                                     SM2_DEFAULT_ID_GMT09, SM2_DEFAULT_ID_LENGTH,
                                     dgst, (size_t *) &dgst_len, ec_key);
    if (!ret) {
        LOGE("SM2 digest failed: %s", ERR_reason_error_string(ERR_get_error()));
        return -1;
    }

    ret = SM2_sign(/*NID_sm3*/NID_undef, dgst, dgst_len, out, (unsigned int *) &out_len, ec_key);
    if (ret != 1) {
        LOGE("SM2 sign failed: %s", ERR_reason_error_string(ERR_get_error()));
        return -1;
    }
    return 0;
}

int gm_sm2_verify(EC_KEY *ec_key, const unsigned char* buf, int buf_len, const unsigned char *sig, int sign_len)
{
    int ret;
    unsigned char dgst[EVP_MAX_MD_SIZE];
    size_t dgst_len = sizeof(dgst);

    ret = SM2_compute_message_digest(EVP_sm3(), EVP_sm3(), buf, buf_len,
                                     SM2_DEFAULT_ID_GMT09, SM2_DEFAULT_ID_LENGTH,
                                     dgst, (size_t *) &dgst_len, ec_key);
    if (!ret) {
        LOGE("SM2 digest failed: %s", ERR_reason_error_string(ERR_get_error()));
        return -1;
    }

    ret = SM2_verify(/*NID_sm3*/NID_undef, dgst, dgst_len, sig, sign_len, ec_key);
    if (ret != 1) {
        LOGE("SM2 verify failed: %s", ERR_reason_error_string(ERR_get_error()));
        return -1;
    }
    return 0;
}

int gm_sm2_encrypt(EC_KEY *ec_key, const unsigned char* buf, int buf_len, unsigned char* out, int& out_len)
{
    int ret;
    out_len = 0;

    ret = SM2_encrypt(NID_sm3, buf, buf_len, out, (size_t *) &out_len, ec_key);
    if (ret != 1) {
        LOGE("SM2 encrypt failed: %s", ERR_reason_error_string(ERR_get_error()));
        return -1;
    }
    return 0;
}

int gm_sm2_decrypt(EC_KEY *ec_key, const unsigned char* buf, int buf_len, unsigned char* out, int& out_len)
{
    int ret;
    out_len = 0;

    ret = SM2_decrypt(NID_sm3, (const unsigned char *) buf, buf_len, out, (size_t *) &out_len, ec_key);
    if (ret != 1) {
        LOGE("SM2 decrypt failed: %s", ERR_reason_error_string(ERR_get_error()));
        return -1;
    }
    if (NULL != out) {
        out[out_len] = '\0';
    }
    return 0;
}

int gm_sm3_hash(const unsigned char* buf, int buf_len, unsigned char* dgst, int& dgst_len)
{
    sm3(buf, buf_len, dgst);
    dgst_len = SM3_DIGEST_LENGTH;
    return 0;
}

int gm_sm4_encrypt(const unsigned char* key, const unsigned char* buf, int buf_len, unsigned char* out, int& out_len)
{
    int padding_len = SMS4_KEY_LENGTH - buf_len % SMS4_KEY_LENGTH;
    int block = buf_len / SMS4_KEY_LENGTH;
    int end_len = SMS4_KEY_LENGTH - padding_len;

    unsigned char *p = (unsigned char *) malloc(SMS4_KEY_LENGTH + 1);
    memset(p, 0, SMS4_KEY_LENGTH + 1);
    memset(p + end_len, padding_len, (size_t) padding_len);
    memcpy(p, buf + block * SMS4_KEY_LENGTH, (size_t) end_len);

    sms4_key_t en_key;
    sms4_set_encrypt_key(&en_key, key);

    for (int i = 0; i < block; i++) {
        sms4_encrypt((const unsigned char *) (buf + (i * 16)), out + i * 16, &en_key);
    }
    sms4_encrypt(p, out + block * 16, &en_key);

    out_len = buf_len + padding_len;

    free(p);
    return 0;
}

int gm_sm4_decrypt(const unsigned char* key, const unsigned char* buf, int buf_len, unsigned char* out, int& out_len)
{
    sms4_key_t dec_key;
    sms4_set_decrypt_key(&dec_key, key);

    for (int i = 0; i < buf_len / 16; i++) {
        sms4_decrypt((const unsigned char *) (buf + (i * 16)), out + i * 16, &dec_key);
    }
    int padding_len = out[buf_len - 1]; // 补位
    memset(out + buf_len - padding_len, 0, padding_len);
    out_len = buf_len - padding_len;

    return 0;
}
