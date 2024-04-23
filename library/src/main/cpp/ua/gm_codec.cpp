//
// Created by nanuns on 2021/9/10.
//

#include "../utils/util.h"
#include "../gb_header.h"
#include "gm_codec.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define UUID_SIZE 16
static uint8_t NALU_ANNEXB_CODE_3_BYTE[] = { 0x00, 0x00, 0x01 };
static uint8_t NALU_ANNEXB_CODE_4_BYTE[] = { 0x00, 0x00, 0x00, 0x01 };

// random ID number generated according to ISO-11578
//static const uint8_t uuid[16] = {
//        0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7,
//        0x96, 0x2c, 0xd8, 0x20, 0xd9, 0x23, 0xee, 0xef
//};
static unsigned char uuid[] = {
        0x54, 0x80, 0x83, 0x97, 0xf0, 0x23, 0x47, 0x4b,
        0xb7, 0xf7, 0x4f, 0x32, 0xb5, 0x4e, 0x06, 0xac
};


int sec_parameter_set_rbsp(const extention_nalu_header_t& header, const sec_parameter_set_t& seps, uint8_t* data, int len)
{
    int i;

    bits_buffer_t pb;
    pb.i_size = len;
    pb.i_data = 0;
    pb.i_mask = 0x80;
    pb.p_data = data;
    memset(pb.p_data, 0, len);

    bits_write(&pb, 1, header.forbidden_zero_bit);
    bits_write(&pb, 1, header.nal_ref_idc);
    bits_write(&pb, 4, header.nal_unit_type);
    bits_write(&pb, 1, header.encryption_idc);
    bits_write(&pb, 1, header.authentication_idc);

    bits_write(&pb, 1, seps.encryption_flag);
    bits_write(&pb, 1, seps.authentication_flag);

    if (seps.encryption_flag) {
        bits_write(&pb, 4, seps.encryption_type);
        bits_write(&pb, 1, seps.vek_flag);
        bits_write(&pb, 1, seps.iv_flag);

        if (seps.vek_flag) {
            int evek_len = seps.evek_length_minus1 + 1;
            int vkek_version_len = seps.vkek_version_length_minus1 + 1;

            bits_write(&pb, 4, seps.vek_encryption_type);
            bits_write(&pb, 8, seps.evek_length_minus1);
            for (i = 0; i < evek_len; i++) {
                bits_write(&pb, 8, seps.evek[i]);
            }
            bits_write(&pb, 8, seps.vkek_version_length_minus1);
            for (i = 0; i < vkek_version_len; i++) {
                bits_write(&pb, 8, seps.vkek_version[i]);
            }
        }
        if (seps.iv_flag) {
            int iv_len = seps.iv_length_minus1 + 1;
            bits_write(&pb, 8, seps.iv_length_minus1);
            for (i = 0; i < iv_len; i++) {
                bits_write(&pb, 8, seps.iv[i]);
            }
        }
    }
    if (seps.authentication_flag) {
        bits_write(&pb, 2, seps.hash_type);
        bits_write(&pb, 1, seps.hash_discard_p_pictures);
        bits_write(&pb, 2, seps.signature_type);
        bits_write(&pb, 8, seps.successive_hash_pictures_minus1);

        for (i = 0; i < sizeof(seps.camera_idc); i++) {
            bits_write(&pb, 8, seps.camera_idc[i]);
        }
    }
    if (seps.vek_flag || seps.authentication_flag) {
        for (i = 0; i < sizeof(seps.camera_id); i++) {
            bits_write(&pb, 8, seps.camera_id[i]);
        }
    }
    // rbsp

    return pb.i_data;
}

int authentication_data_rbsp(const extention_nalu_header_t& header, const authentication_data_t& auth_data, uint8_t* data, int len)
{
    int i;
    uint8_t spatial_svc_flag = 0;
    int auth_data_len = auth_data.auth_data_length_minus1 + 1;

    bits_buffer_t pb;
    pb.i_size = len;
    pb.i_data = 0;
    pb.i_mask = 0x80;
    pb.p_data = data;
    memset(pb.p_data, 0, len);

    bits_write(&pb, 1, header.forbidden_zero_bit);
    bits_write(&pb, 1, header.nal_ref_idc);
    bits_write(&pb, 4, header.nal_unit_type);
    bits_write(&pb, 1, header.encryption_idc);
    bits_write(&pb, 1, header.authentication_idc);

    bits_write(&pb, 8, auth_data.frame_num);
    if (spatial_svc_flag) {
        bits_write(&pb, 8, auth_data.spatial_el_flag);
    }
    bits_write(&pb, 8, auth_data.auth_data_length_minus1);
    for (i = 0; i < auth_data_len; i++) {
        bits_write(&pb, 8, auth_data.auth_data[i]);
    }
    return pb.i_data;
}

uint32_t get_sei_nalu_len(uint32_t content_len)
{
    // SEI payload size
    uint32_t sei_payload_len = content_len + UUID_SIZE;
    // NALU + payload类型 + 数据长度 + 数据
    uint32_t sei_len = 1 + 1 + (sei_payload_len / 0xFF + (sei_payload_len % 0xFF != 0 ? 1 : 0)) + sei_payload_len;
    // 截止码
    uint32_t tail_len = 2;
    if (sei_len % 2 == 1) {
        tail_len -= 1;
    }
    sei_len += tail_len;

    return sei_len;
}

int get_sei_buffer(uint8_t* data, uint32_t size, uint8_t * buffer, uint32_t *count)
{
    uint8_t* sei = data;
    int sei_type = 0;
    uint32_t sei_size = 0;

    // payload type
    do {
        sei_type += *sei;
    } while (*sei++ == 0xFF);
    // payload len
    do {
        sei_size += *sei;
    } while (*sei++ == 0xFF);

    // 检查uuid
    if (sei_size >= UUID_SIZE && sei_size <= (data + size - sei) &&
        sei_type == 5 && memcmp(sei, uuid, UUID_SIZE) == 0) {
        sei += UUID_SIZE;
        sei_size -= UUID_SIZE;

        if (buffer != NULL && count != NULL) {
            if (*count > (uint32_t)sei_size) {
                memcpy(buffer, sei, sei_size);
            }
        }
        if (count != NULL) {
            *count = sei_size;
        }
        return sei_size;
    }
    return -1;
}

int gen_sei(const uint8_t* content, int len, uint8_t* sei_nalu, int* sei_nalu_len)
{
    int index = 0;
    sei_nalu[index++] = 0x00;
    sei_nalu[index++] = 0x00;
    sei_nalu[index++] = 0x00;
    sei_nalu[index++] = 0x01; // nalu start code
    sei_nalu[index++] = 0x06; // nalu type
    sei_nalu[index++] = 0x05; // sei payload type

    sei_nalu[index++] = len + 16 + 1; // sei payload size
    memcpy(&sei_nalu[index], uuid, 16); index += 16; // sei payload uuid
    memcpy(&sei_nalu[index], content, len); index += len; // sei payload content
    sei_nalu[index++] = 0x80; // rbsp

    *sei_nalu_len = index;
    return 0;
}

/**
 * https://blog.csdn.net/passionkk/article/details/102804023  FFmpeg H264增加SEI
 * SEI NALU数据格式
 *      NALU 类型 1 字节: 0x06
 *      SEI 负载类型 1 字节: 0x05 (用户自定义数据)
 *      负载大小(uuid+自定义数据), 如果 len 大于 255, 前 int(len / 255) 个字节都是 FF, 最后一个字节是剩余部分
 *      负载的唯一标志 uuid 16 字节
 *      自定义数据
 *   替换 00 00 为 00 00 03
 *   添加 SEI 使用 av_grow_packet 后在开头插入 SEI NALU 数据
 * @param content
 * @param content_len
 * @param is_annexb
 * @param packet
 * @param packet_len
 * @return
 */
int gen_sei_nalu(const uint8_t* content, uint32_t content_len, bool is_annexb, uint8_t* packet, uint32_t *packet_len)
{
    uint8_t* data = (uint8_t*) packet;
    uint32_t nalu_len = get_sei_nalu_len(content_len);
    uint32_t sei_len = nalu_len;

    // NALU 开始码
    if (is_annexb) {
        memcpy(data, NALU_ANNEXB_CODE_4_BYTE, sizeof(uint32_t));
    }
    else {
        // 大端转小端
        nalu_len = reverse_bytes(nalu_len);
        memcpy(data, &nalu_len, sizeof(uint32_t));
    }
    data += sizeof(uint32_t);

    uint8_t* sei = data;
    // NALU header
    *data++ = 6; // SEI nalu type
    // sei payload type
    *data++ = 5; // sei payload type: unregister user data
    size_t sei_payload_len = content_len + UUID_SIZE;
    //数据长度
    while (true) {
        *data++ = (sei_payload_len >= 0xFF ? 0xFF : (char)sei_payload_len);
        if (sei_payload_len < 0xFF) break;
        sei_payload_len -= 0xFF;
    }

    // uuid
    memcpy(data, uuid, UUID_SIZE);
    data += UUID_SIZE;
    // payload
    memcpy(data, content, content_len);
    data += content_len;

    // 截止对齐码
    uint32_t tail_len = sei + sei_len - data;
    if (tail_len == 1) {
        *data = 0x80;
    }
    else if (tail_len == 2) {
        *data++ = 0x00;
        *data++ = 0x80;
    }
    if (packet_len) {
        *packet_len = sei_len + tail_len;
    }

    return 0;
}

int get_sei_content(uint8_t* packet, uint32_t size, uint8_t* buffer, uint32_t *count)
{
    uint8_t *data = packet;
    bool is_annexb = false;

    if ((size > 3 && memcmp(data, NALU_ANNEXB_CODE_3_BYTE, 3) == 0) ||
        (size > 4 && memcmp(data, NALU_ANNEXB_CODE_4_BYTE, 4) == 0)) {
        is_annexb = true;
    }
    //暂时只处理MP4封装,annexb暂为处理
    if (is_annexb) {
        while (data < packet + size) {
            if ((packet + size - data) > 4 && data[0] == 0x00 && data[1] == 0x00) {
                int start_code_size = 2;
                if (data[2] == 0x01) {
                    start_code_size = 3;
                }
                else if (data[2] == 0x00 && data[3] == 0x01) {
                    start_code_size = 4;
                }
                if (start_code_size == 3 || start_code_size == 4) {
                    if ((packet + size - data) > (start_code_size + 1) &&
                        (data[start_code_size] & 0x1F) == 6) {
                        // SEI
                        uint8_t * sei = data + start_code_size + 1;

                        int ret = get_sei_buffer(sei, (packet + size - sei), buffer, count);
                        if (ret != -1) {
                            return ret;
                        }
                    }
                    data += start_code_size + 1;
                }
                else {
                    data += start_code_size + 1;
                }
            }
            else {
                data++;
            }
        }
    }
    else {
        // 当前NALU
        while (data < packet + size) {
            // MP4格式起始码/长度
            uint32_t *length = (uint32_t *)data;
            uint32_t nalu_size = reverse_bytes(*length);
            // NALU header
            if ((*(data + 4) & 0x1F) == 6) {
                // SEI
                uint8_t * sei = data + 4 + 1;

                int ret = get_sei_buffer(sei, min(nalu_size, (packet + size - sei)), buffer, count);
                if (ret != -1) {
                    return ret;
                }
            }
            data += 4 + nalu_size;
        }
    }
    return -1;
}
