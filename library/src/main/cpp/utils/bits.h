//
// Created by nanuns on 2019/7/15.
//

#ifndef NANGBD_BITS_H
#define NANGBD_BITS_H

#include <malloc.h>
#include <cstdint>

typedef struct bits_buffer_s
{
    int      i_size;
    int      i_data;
    uint8_t  i_mask;
    uint8_t* p_data;
} bits_buffer_t;

static inline int bits_initwrite(bits_buffer_t *p_buffer, int i_size, void *p_data)
{
    p_buffer->i_size = i_size;
    p_buffer->i_data = 0;
    p_buffer->i_mask = 0x80;
    p_buffer->p_data = (uint8_t *) p_data;
    if (!p_buffer->p_data) {
        if (!(p_buffer->p_data = (uint8_t *) malloc(i_size))) {
            return -1;
        }
    }
    p_buffer->p_data[0] = 0;
    return 0;
}

static inline void bits_align(bits_buffer_t* p_buffer)
{
    if (p_buffer->i_mask != 0x80 && p_buffer->i_data < p_buffer->i_size) {
        p_buffer->i_mask = 0x80;
        p_buffer->i_data++;
        p_buffer->p_data[p_buffer->i_data] = 0x00;
    }
}

static inline void bits_write(bits_buffer_t *p_buffer, int i_count, uint64_t i_bits)
{
    while (i_count > 0) {
        i_count--;
        if ((i_bits >> i_count) & 0x01) {
            p_buffer->p_data[p_buffer->i_data] |= p_buffer->i_mask;
        } else {
            p_buffer->p_data[p_buffer->i_data] &= ~p_buffer->i_mask;
        }
        /*操作完一个字节第一位后，操作第二位*/
        p_buffer->i_mask >>= 1;
        /*循环完一个字节的8位后，重新开始下一位*/
        if (p_buffer->i_mask == 0) {
            p_buffer->i_data++;
            p_buffer->i_mask = 0x80;
        }
    }
}

#endif // NANGBD_BITS_H
