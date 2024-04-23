//
// Created by nanuns on 2021/9/10.
//

#ifndef NANGBD_GM_CODEC_H
#define NANGBD_GM_CODEC_H

#include <stdint.h>

typedef struct extention_nalu_header {
    uint8_t forbidden_zero_bit;                 // u(1), 1
    uint8_t nal_ref_idc;                        // u(1)
    uint8_t nal_unit_type;                      // u(4)
    uint8_t encryption_idc;                     // u(1)
    uint8_t authentication_idc;                 // u(1)
} extention_nalu_header_t;

typedef struct sec_parameter_set {
    uint8_t encryption_flag;                    // u(1)
    uint8_t authentication_flag;                // u(1)

    uint8_t encryption_type;                    // u(4), 0:SM1,1:SM4
    uint8_t vek_flag;                           // u(1)
    uint8_t iv_flag;                            // u(1)

    uint8_t vek_encryption_type;                // u(4), 0:SM1,1:SM4
    uint8_t evek_length_minus1;                 // u(8)
    uint8_t* evek;                              // f(n), evek_length_minus1 + 1
    uint8_t vkek_version_length_minus1;         // u(8)
    uint8_t* vkek_version;                      // f(n), vkek_version_length_minus1 + 1

    uint8_t iv_length_minus1;                   // u(8)
    uint8_t* iv;                                // f(n), iv_length_minus1 + 1

    uint8_t hash_type;                          // u(2), 0:SM3-32 bytes
    uint8_t hash_discard_p_pictures;            // u(1)
    uint8_t signature_type;                     // u(2), 0:SM2
    uint8_t successive_hash_pictures_minus1;    // u(8)
    uint8_t camera_idc[19];                     // f(152)

    uint8_t camera_id[20];                      // f(160)
} sec_parameter_set_t;

typedef struct authentication_data {
    uint8_t frame_num;                          // u(8)
    uint8_t spatial_el_flag;                    // u(8)
    uint8_t auth_data_length_minus1;            // u(8)
    uint8_t* auth_data;                         // auth_data_length_minus1 + 1, base64
} authentication_data_t;

// 封装安全参数集
int sec_parameter_set_rbsp(const extention_nalu_header_t& header, const sec_parameter_set_t& seps, uint8_t* data, int len);

// 封装认证数据
int authentication_data_rbsp(const extention_nalu_header_t& header, const authentication_data_t& auth_data, uint8_t* data, int len);

// 封装sei nalu
// https://www.jianshu.com/p/7c6861f0d7fd
int gen_sei_nalu(const uint8_t* content, uint32_t content_len, bool is_annexb, uint8_t* packet, uint32_t* packet_len);

int get_sei_content(uint8_t* packet, uint32_t size, uint8_t* buffer, uint32_t *count);

#endif // NANGBD_GM_CODEC_H
