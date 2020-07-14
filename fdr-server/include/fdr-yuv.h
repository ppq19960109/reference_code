/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __FDR_YUV_H__
#define __FDR_YUV_H__

#include <inttypes.h>

// 剪裁出来的图像必须2像素对齐；
int fdr_yuv_nv12_crop(uint8_t *src, int src_width, int src_heigh, 
                uint8_t *crop, int top, int left, int width, int heigh);

// 缩放的图像必须2像素对齐；
int fdr_yuv_nv12_scale(uint8_t *src, int src_width, int src_heigh,
                uint8_t *scale, int width, int heigh);

int fdr_yuv_nv12_to_rgb1555(const uint8_t *yuv, int width, int heigh, uint8_t *rgb);
int fdr_yuv_nv12_to_rgb24(const uint8_t *yuv, int width, int heigh, uint8_t *rgb);

int fdr_yuv_save_to_bmp(const char  *filepath, uint8_t *rgb, int width, int heigh, int chs);

#endif // __FDR_YUV_H__
