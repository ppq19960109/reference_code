
#ifndef _BITMAP_ROTATE_H_
#define _BITMAP_ROTATE_H_

#include "stdint.h"

typedef enum{
    LEFT_ROTATE,
    RIGHT_ROTATE,
}bm_rotate_mode_t;

int32_t bitmap_show(const uint8_t *bitmap, uint32_t w, uint32_t h);

int32_t bitmap_rotate(const uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h, uint8_t mode);

#endif
