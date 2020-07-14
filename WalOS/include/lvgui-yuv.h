/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __LVGUI_YUV_H__
#define __LVGUI_YUV_H__

#include <inttypes.h>

typedef struct lvgui_bmp_header{
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
}__attribute__((packed))lvgui_bmp_header_t;

typedef struct lvgui_bmp_info{
    uint32_t  biSize;
    uint32_t  biWidth;
    uint32_t  biHeight;
    uint16_t  biPlanes;
    uint16_t  biBitCount;
    uint32_t  biCompression;
    uint32_t  biSizeImage;
    uint32_t  biXPelsPerMeter;
    uint32_t  biYPelsPerMeter;
    uint32_t  biClrUsed;
    uint32_t  biClrImportant;
}__attribute__((packed))lvgui_bmp_info_t;

// 剪裁出来的图像必须2像素对齐；
int lvgui_yuv_nv12_crop(uint8_t *src, int src_width, int src_heigh, 
                uint8_t *crop, int top, int left, int width, int heigh);

// 缩放的图像必须2像素对齐；
int lvgui_yuv_nv12_scale(uint8_t *src, int src_width, int src_heigh,
                uint8_t *scale, int width, int heigh);

int lvgui_yuv_nv12_to_rgb1555(const uint8_t *yuv, int width, int heigh, uint8_t *rgb);
int lvgui_yuv_nv12_to_rgb24(const uint8_t *yuv, int width, int heigh, uint8_t *rgb);

int lvgui_save_to_bmp(const char  *filepath, uint8_t *rgb, int width, int heigh, int chs);

#endif // __LVGUI_YUV_H__
