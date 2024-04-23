//
// Created by nanuns on 2018/9/11.
//

#ifndef NANGBD_GB_HEADER_H
#define NANGBD_GB_HEADER_H

#include <memory.h>
#include "utils/bits.h"
#include "utils/util.h"
#include "utils/log.h"

#define DATA_LEN sizeof(uint32_t)
#define TS_LEN sizeof(uint64_t)
#define FRAME_FLAG_LEN sizeof(uint8_t)

#define PS_HDR_LEN  14
#define SYS_HDR_LEN 18
#define PSM_HDR_LEN 24
#define PES_HDR_LEN 19
#define PES_PAYLOAD_MAX_LEN (0xFFFF - 13)

#define RTP_MAX_PACKET_BUFF 1400
#define RTP_TCP_HDR_LEN sizeof(uint16_t)
#define RTP_HDR_LEN 12
#define RTP_VERSION 2
#define RTP_PKT_END 1
#define RTP_PKT_APPENDING 0

#define RTP_V(v)	(((v) >> 30) & 0x03)   /* protocol version */
#define RTP_P(v)	(((v) >> 29) & 0x01)   /* padding flag */
#define RTP_X(v)	(((v) >> 28) & 0x01)   /* header extension flag */
#define RTP_CC(v)	(((v) >> 24) & 0x0F)   /* CSRC count */
#define RTP_M(v)	(((v) >> 23) & 0x01)   /* marker bit */
#define RTP_PT(v)	(((v) >> 16) & 0x7F)   /* payload type */
#define RTP_SEQ(v)	(((v) >> 00) & 0xFFFF) /* sequence number */

/**
 * stream type:
 *      MPEG-4  视频流： 0x10
 *      H.264   视频流： 0x1B
 *      SVACV   视频流： 0x80
 *      G.711   音频流： 0x90
 *      G.722.1 音频流： 0x92
 *      G.723.1 音频流： 0x93
 *      G.729   音频流： 0x99
 *      SVACA   音频流： 0x9B
 */
#define ST_ID_MPEG  0x10
#define ST_ID_H264  0x1b
#define ST_ID_H265  0x24
#define ST_ID_SVACV 0x80

#define ST_ID_G711A 0x90
#define ST_ID_SVACA 0x9b

#define AUDIO_ID 0xC0
#define VIDEO_ID 0xE0

#define PACK_START_CODE             ((unsigned int)0x000001ba)
#define SYS_START_CODE              ((unsigned int)0x000001bb)
#define SEQUENCE_END_CODE           ((unsigned int)0x000001b7)
#define PACKET_START_CODE_MASK      ((unsigned int)0xffffff00)
#define PACKET_START_CODE_PREFIX    ((unsigned int)0x00000100)

/***
 * https://blog.csdn.net/ichenwin/article/details/100086930 PS封装格式：GB28181协议RTP传输
 * https://blog.csdn.net/chen495810242/article/details/39207305
 * https://blog.csdn.net/QuickGBLink/article/details/103434753
 * https://blog.csdn.net/garefield/article/details/45113349 ffmpeg添加mpeg ps流的pcm的编码支持
 * https://blog.csdn.net/qq_24977505/article/details/95193041 最简单的h264/h265/svac和g711封装成ps流, 符合gb28181中标准ps流
 * https://www.cnblogs.com/CoderTian/p/7701597.html MPEG-PS封装格式
 * https://www.cnblogs.com/lidabo/p/6553028.html RTP协议之Header结构解析
 *
 * @remark ps头的封装,里面的具体数据的填写已经占位，可以参考标准
 * @param pData  [in] 填充ps头数据的地址
 * @param s64Src [in] 时间戳
 * @return 0 success, others failed
*/
static int gb_make_ps_header(uint8_t *data, int64_t scr, int mux_rate)
{
    bits_buffer_t pb;
    int stuffing_len = 0;
    int ps_h_len = PS_HDR_LEN + stuffing_len;
    int64_t scr_ext = 0;

    pb.i_size = ps_h_len;
    pb.i_data = 0;
    pb.i_mask = 0x80; // 这里是为了后面对一个字节的每一位进行操作，避免大小端跨字节字序错乱
    pb.p_data = data;
    memset(pb.p_data, 0, ps_h_len);
    bits_write(&pb, 32, PACK_START_CODE);                    /*start codes*/
    bits_write(&pb, 2, 1);                                   /*marker bits '01b'*/
    bits_write(&pb, 3, (uint32_t)(scr >> 30) & 0x07);        /*system_clock_reference_base [32..30]*/
    bits_write(&pb, 1, 1);                                   /*marker bit*/
    bits_write(&pb, 15, (uint32_t)(scr >> 15) & 0x7fff);     /*system_clock_reference_base [29..15]*/
    bits_write(&pb, 1, 1);                                   /*marker bit*/
    bits_write(&pb, 15, (uint32_t)(scr & 0x7fff));           /*system_clock_reference_base [14..0]*/
    bits_write(&pb, 1, 1);                                   /*marker bit*/
    bits_write(&pb, 9, scr_ext & 0x01ff);                    /*system_clock_reference_extension*/
    bits_write(&pb, 1, 1);                                   /*marker bit*/
    bits_write(&pb, 22, mux_rate & 0x3fffff);                /*program_mux_rate(in units of 50 bytes/s.)*/
    bits_write(&pb, 1, 1);                                   /*marker bit*/
    bits_write(&pb, 1, 1);                                   /*marker bit*/
    bits_write(&pb, 5, 0x1f);                                /*reserved*/
    bits_write(&pb, 3, stuffing_len & 0x07);                 /*stuffing length*/
    if (stuffing_len > 0) {
        int i;
        for (i = 0; i < stuffing_len; i++) {
            bits_write(&pb, 8, 0xFF);                        /*stuffing bytes*/
        }
    }
    return ps_h_len;
}

/***
 *@remark sys(PS system header)头的封装
 *@param pData [in] 填充ps头数据的地址
 *@return 0 success, others failed
*/
static int gb_make_sys_header(uint8_t *data, int audio_bound, int video_bound, int mux_rate)
{
#define audio_max_buf_size (4 * 1024)
#define video_max_buf_size (1024 * 8191)

    bits_buffer_t pb;
    int sys_h_len = SYS_HDR_LEN;
    if (audio_bound <= 0) {
        sys_h_len -= 3;
    }
    if (video_bound <= 0) {
        sys_h_len -= 3;
    }

    pb.i_size = sys_h_len;
    pb.i_data = 0;
    pb.i_mask = 0x80;
    pb.p_data = data;
    memset(pb.p_data, 0, sys_h_len);
    /*system header*/
    bits_write(&pb, 32, SYS_START_CODE);    /*start code*/
    bits_write(&pb, 16, sys_h_len - 6);     /*header_length 表示此字节后面的长度,后面的相关头也是此意思*/
    bits_write(&pb, 1, 1);                  /*marker_bit*/
    bits_write(&pb, 22, mux_rate);          /*rate_bound*/
    bits_write(&pb, 1, 1);                  /*marker_bit*/
    bits_write(&pb, 6, audio_bound);        /*audio_bound*/
    if (false) { // is_vcd
        // /* see VCD standard p IV-7 */
        bits_write(&pb, 1, 0);              /*fixed_flag: variable bitrate*/
        bits_write(&pb, 1, 1);              /*CSPS_flag: nonconstrained bitstream*/
    } else {
        bits_write(&pb, 1, 0);              /*fixed_flag: variable bitrate*/
        bits_write(&pb, 1, 0);              /*CSPS_flag: nonconstrained bitstream*/
    }
    if (0) { // is_vcd || is_dvd
        /* see VCD standard p IV-7 */
        bits_write(&pb, 1, 1);              /*system_audio_lock_flag*/
        bits_write(&pb, 1, 1);              /*system_video_lock_flag*/
    } else {
        bits_write(&pb, 1, 0);              /*system_audio_lock_flag*/
        bits_write(&pb, 1, 0);              /*system_video_lock_flag*/
    }
    bits_write(&pb, 1, 1);                  /*marker_bit*/
    bits_write(&pb, 5, video_bound);        /*video_bound*/

    if (0) { // is_dvd
        bits_write(&pb, 1, 0);              /*packet_rate_restriction_flag: dif from mpeg1*/
        bits_write(&pb, 7, 0x7f);           /*reserved*/
    } else {
        bits_write(&pb, 8, 0xff);           /*reserved*/
    }

    if (video_bound > 0) {
        /*video stream bound*/
        bits_write(&pb, 8, VIDEO_ID);           /*stream_id 0xb9*/
        bits_write(&pb, 2, 3);                  /*marker_bit */
        bits_write(&pb, 1, 1);                  /*P-STD_buffer_bound_scale*/
        bits_write(&pb, 13, video_max_buf_size / 1024); /*PSTD_buffer_size_bound*/
    }
    if (audio_bound > 0) {
        /*audio stream bound*/
        bits_write(&pb, 8, AUDIO_ID);           /*stream_id 0xb8*/
        bits_write(&pb, 2, 3);                  /*marker_bit */
        bits_write(&pb, 1, 0);                  /*P-STD_buffer_bound_scale*/
        bits_write(&pb, 13, audio_max_buf_size / 128); /*PSTD_buffer_size_bound*/
    }
    return sys_h_len;
}

/***
 * @remark psm(PS system Map)头的封装
 * @param pData [in] 填充ps头数据的地址
 * @param v_stream_type
 * @param a_stream_type
 * @return 0 success, others failed
*/
static int gb_make_psm_header(uint8_t *data, int v_stream_type, int a_stream_type)
{
    bits_buffer_t pb;
    int psm_h_len = PSM_HDR_LEN;
    int stream_info_len = 0;
    if (v_stream_type > 0) {
        stream_info_len += 4;
    } else {
        psm_h_len -= 4;
    }
    if (a_stream_type > 0) {
        stream_info_len += 4;
    } else {
        psm_h_len -= 4;
    }

    pb.i_size = psm_h_len;
    pb.i_data = 0;
    pb.i_mask = 0x80;
    pb.p_data = data;
    memset(pb.p_data, 0, psm_h_len);
    bits_write(&pb, 24, 0x000001);              /*start code*/
    bits_write(&pb, 8, 0xBC);                   /*map stream id*/
    bits_write(&pb, 16, 10 + stream_info_len);  /*program stream map length*/
    bits_write(&pb, 1, 1);                      /*current next indicator */
    bits_write(&pb, 2, 3);                      /*reserved*/
    bits_write(&pb, 5, 1);                      /*program stream map version, or 0*/
    bits_write(&pb, 7, 0x7F);                   /*reserved */
    bits_write(&pb, 1, 1);                      /*marker bit */
    bits_write(&pb, 16, 0);                     /*program stream info length*/
    bits_write(&pb, 16, stream_info_len);       /*elementary stream map length*/
    /*video*/
    if (v_stream_type > 0) {
        bits_write(&pb, 8, v_stream_type);      /*stream_type*/
        bits_write(&pb, 8, 0xE0);               /*elementary_stream_id*/
        bits_write(&pb, 16, 0);                 /*elementary_stream_info_length */
    }
    /*audio*/
    if (a_stream_type > 0) {
        bits_write(&pb, 8, a_stream_type);      /*stream_type*/
        bits_write(&pb, 8, 0xC0);               /*elementary_stream_id*/
        bits_write(&pb, 16, 0);                 /*elementary_stream_info_length is*/
    }
    /*crc (2e b9 0f 3d)*/
    bits_write(&pb, 8, 0x45);                   /*crc (24~31) bits*/
    bits_write(&pb, 8, 0xBD);                   /*crc (16~23) bits*/
    bits_write(&pb, 8, 0xDC);                   /*crc (8~15) bits*/
    bits_write(&pb, 8, 0xF4);                   /*crc (0~7) bits*/
    return psm_h_len;
}

/***
 * @remark pes头的封装
 * @param pData      [in] 填充ps头数据的地址
 *        stream_id  [in] 码流类型
 *        paylaod_len[in] 负载长度
 *        pts        [in] 时间戳
 *        dts        [in]
 * @return  0 success, others failed
*/
static int gb_make_pes_header(uint8_t *data, int stream_id, int payload_len, int64_t pts, int64_t dts)
{
    int pes_h_len = PES_HDR_LEN;
    bits_buffer_t pb;
    pb.i_size = pes_h_len;
    pb.i_data = 0;
    pb.i_mask = 0x80;
    pb.p_data = data;
    memset(pb.p_data, 0, pes_h_len);
    bits_write(&pb, 24, 0x000001);                  /*start code*/
    bits_write(&pb, 8, stream_id);                  /*streamID*/
    bits_write(&pb, 16, payload_len + 3 + 10);      /*packet_len 指出pes分组中数据长度和该字节后的长度和*/
    bits_write(&pb, 2, 2);                          /*'10'*/
    bits_write(&pb, 2, 0);                          /*scrambling_control 加扰控制，00表示没有加密，剩下的01,10,11由用户自定义*/
    bits_write(&pb, 1, 0);                          /*priority*/
    bits_write(&pb, 1, 0);                          /*data_alignment_indicator*/
    bits_write(&pb, 1, 0);                          /*copyright*/
    bits_write(&pb, 1, 0);                          /*original_or_copy*/
    bits_write(&pb, 1, 1);                          /*PTS_flag*/
    bits_write(&pb, 1, 1);                          /*DTS_flag*/
    bits_write(&pb, 1, 0);                          /*ESCR_flag*/
    bits_write(&pb, 1, 0);                          /*ES_rate_flag*/
    bits_write(&pb, 1, 0);                          /*DSM_trick_mode_flag*/
    bits_write(&pb, 1, 0);                          /*additional_copy_info_flag*/
    bits_write(&pb, 1, 0);                          /*PES_CRC_flag*/
    bits_write(&pb, 1, 0);                          /*PES_extension_flag*/
    bits_write(&pb, 8, 10);                         /*PES_header_data_length*/
    // 指出包含在 PES 分组标题中的可选字段和任何填充字节所占用的总字节数
    // 该字段之前的字节指出了有无可选字段

    /*PTS,DTS  PTS DTS均为1的情况*/
    bits_write(&pb, 4, 3);                          /*'0011'*/
    bits_write(&pb, 3, ((pts) >> 30) & 0x07);       /*PTS[32..30]*/
    bits_write(&pb, 1, 1);
    bits_write(&pb, 15, ((pts) >> 15) & 0x7FFF);    /*PTS[29..15]*/
    bits_write(&pb, 1, 1);
    bits_write(&pb, 15, (pts) & 0x7FFF);            /*PTS[14..0]*/
    bits_write(&pb, 1, 1);

    bits_write(&pb, 4, 1);                          /*'0001'*/
    bits_write(&pb, 3, ((dts) >> 30) & 0x0);        /*DTS[32..30]*/
    bits_write(&pb, 1, 1);
    bits_write(&pb, 15, ((dts) >> 15) & 0x7FFF);    /*DTS[29..15]*/
    bits_write(&pb, 1, 1);
    bits_write(&pb, 15, (dts) & 0x7FFF);            /*DTS[14..0]*/
    bits_write(&pb, 1, 1);
    return pes_h_len;
}

/**
 * RTP头封装
 * @param pData buffer地址
 * @param payload_type
 * @param seq 序号
 * @param timestamp 时间戳
 * @param ssrc
 * @param marker 一帧是否结束
 * @return
 */
static int gb_make_rtp_header(uint8_t *data, int payload_type, uint16_t seq, int64_t timestamp, int ssrc, int8_t marker)
{
    int rtp_h_len = RTP_HDR_LEN;
    bits_buffer_t pb;
    pb.i_size = rtp_h_len;
    pb.i_data = 0;
    pb.i_mask = 0x80;
    pb.p_data = data;
    memset(pb.p_data, 0, rtp_h_len);
    bits_write(&pb, 2, RTP_VERSION);    /*rtp version*/
    bits_write(&pb, 1, 0);              /*rtp padding*/
    bits_write(&pb, 1, 0);              /*rtp extension*/
    bits_write(&pb, 4, 0);              /*rtp CSRC count*/
    bits_write(&pb, 1, marker);         /*rtp marker*/
    bits_write(&pb, 7, payload_type);   /*rtp payload type*/
    bits_write(&pb, 16, seq);           /*rtp sequence*/
    bits_write(&pb, 32, timestamp);     /*rtp timestamp */
    bits_write(&pb, 32, ssrc);          /*rtp SSRC*/
    return rtp_h_len;
}

static int gb_parse_rtp_packet(uint8_t *data, size_t size)
{
    if (NULL == data || size < RTP_HDR_LEN) {
        LOGE("too short to be a valid rtp header");
        return -1;
    }

    uint8_t b = data[0]; // mask 1100 0000
    uint8_t version = (b & 0xc0) >> 6;
    if (version != 2) {
        LOGE("the rtp version is not 2");
        return -1;
    }

    bool padding = (b & 0x20) > 0; // mask 0010 0000
    if (padding) { // Padding present.
        int padding_len = data[size - 1];
        if (padding_len + RTP_HDR_LEN > size) {
            LOGE("the rtp padding too long");
            return -1;
        }
    }

    int csrc_count = b & 0x0f; // mask 0000 1111
    int payload_offset = RTP_HDR_LEN + 4 * csrc_count;
    if (size < payload_offset) {
        LOGE("not enough data to fit the header and all the CSRC entries");
        return -1;
    }

    int rtp_h_len = RTP_HDR_LEN;

    bool extension = (b & 0x10) > 0; // mask 0001 0000
    if (extension) { // Header extension present.
        if (size < payload_offset + 4) {
            LOGE("not enough data to fit the header, all CSRC entries and the first 4 bytes of the extension header");
            return -1;
        }

        const uint8_t *extension_data = &data[payload_offset];
        int extension_len = 4 * (extension_data[2] << 8 | extension_data[3]);
        if (size < payload_offset + 4 + extension_len) {
            LOGE("the rtp extension too long");
            return -1;
        }
        payload_offset += (4 + extension_len);
        rtp_h_len += (4 + extension_len);
    }

    b = data[1];
    bool marker = (b & 0x80) > 0; // mask 0000 0001
    int payload_type = (b & 0x7f); // mask 0111 1111
    short seq = bytes2ushort(&data[2]);
    int timestamp = bytes2uint(&data[4]);
    int ssrc = bytes2uint(&data[8]);
    if (csrc_count > 0) {
        rtp_h_len += csrc_count * 4;
    }

    return rtp_h_len;
}

#endif // NANGBD_GB_HEADER_H
