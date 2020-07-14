/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "lvgui-yuv.h"

#include <stdio.h>
#include <stdlib.h>

static const char *yuvfile = "./test/snap.nv12";
static const char *yuvcropfile = "./test/crop.yuv";
static const char *rgb1555file = "./test/rgb1555.bmp";
static const char *rgb24file = "./test/rgb24.bmp";

static const int yuv_width = 1920;
static const int yuv_heigh = 1080;

static  int save_bmp_file(const char  *filepath, uint8_t *rgb, int width, int heigh, int chs){
    lvgui_bmp_header_t header = {};
    lvgui_bmp_info_t   info = {};
    FILE *fj;
    size_t wlen;

    header.bfType = ('M' << 8) | 'B';
    header.bfSize = width * heigh * chs + sizeof(lvgui_bmp_header_t) + sizeof(lvgui_bmp_info_t);
    header.bfOffBits = sizeof(lvgui_bmp_header_t) + sizeof(lvgui_bmp_info_t);

    info.biSize = sizeof(lvgui_bmp_info_t);
    info.biWidth = width;
    info.biHeight = -heigh;
    info.biPlanes = 1;
    info.biBitCount = 8*chs;
    info.biCompression = 0;
    info.biSizeImage = 0;
    info.biXPelsPerMeter = 2835;
    info.biYPelsPerMeter = 2835;
    info.biClrUsed = 0;
    info.biClrImportant = 0;


    fj = fopen(filepath, "w+");
    if(fj == NULL){
        printf("save_bmp_file => open file  %s fail\n", filepath);
        return -1;
    }

    wlen = fwrite(&header, 1, sizeof(header), fj);
    if(wlen  != sizeof(header)){
        printf("save_bmp_file => write header fail:%zu\n", wlen);
        fclose(fj);
        return -1;
    }

    wlen = fwrite(&info, 1, sizeof(info), fj);
    if(wlen  != sizeof(info)){
        printf("save_bmp_file => write info fail:%zu\n", wlen);
        fclose(fj);
        return -1;
    }
    
    wlen = fwrite(rgb, 1, width*heigh*chs, fj);
    if(wlen  != width*heigh*chs){
        printf("save_bmp_file => write rgb fail:%zu\n", wlen);
        fclose(fj);
        return -1;
    }

    fclose(fj);
    
    printf("save_bmp_file => write rgb ok\n");

    return 0;
}

static int read_file(const char *filepath, uint8_t **buf, int *len){
    FILE *fj;
    long size;
    uint8_t *fbuf;

    fj = fopen(filepath, "r");
    if(fj == NULL){
        printf("read_file => open %s fail!\n", filepath);
        return -1;
    }

	fseek(fj, 0L, SEEK_END);
	size = ftell(fj);
	fseek(fj, 0L, SEEK_SET);

    fbuf = malloc(size);
    if(fbuf == NULL){
        printf("read_file => no memory for load %s\n", filepath);
        fclose(fj);
        return -1;
    }

    if(fread(fbuf, 1, size, fj) != size){
        printf("read_file => load fail!\n");
        free(fbuf);
        fclose(fj);
        return -1;
    }
    fclose(fj);

    *buf = fbuf;
    *len = (int)size;

    return 0;
}

#if 0
int fdr_yuv_nv12_crop(const uint8_t *src, int src_width, int src_heigh, 
                uint8_t *crop, int top, int left, int width, int heigh);

int fdr_yuv_nv12_scale(const uint8_t *src, int src_width, int src_heigh,
                uint8_t *scale, int width, int heigh);

int fdr_yuv_nv12_to_rgb1555(const uint8_t *yuv, int width, int heigh, uint8_t *rgb);

int fdr_yuv_nv12_to_rgb24(const uint8_t *yuv, int width, int heigh, uint8_t *rgb);

#endif

int test_to_rgb24(){
    uint8_t *yuv;
    uint8_t *rgb;
    uint8_t *crop;

    int yuv_len;
    int rgb_len;
    int chs = 2;

    int crop_x = 1256;
    int crop_y = 256;
    int crop_w = 960;
    int crop_h = 540;

    int rc;
    
    rc = read_file(yuvfile, &yuv, &yuv_len);
    if(rc != 0){
        return rc;
    }

    rgb_len = yuv_width * yuv_heigh * chs;
    rgb = malloc(rgb_len);
    if(rgb == NULL){
        printf("test_to_rgb24 => alloc memory  for rgb fail\n");
        return -1;
    }

    crop = malloc(crop_w * crop_h * 3 / 2);
    if(crop == NULL){
        printf("test_to_rgb24 => alloc memory  for rgb fail\n");
        return -1;
    }

    // fdr_yuv_nv12_crop(yuv, yuv_width, yuv_heigh, crop, crop_y, crop_x, crop_w, crop_h);

    //fdr_yuv_nv12_scale(yuv, yuv_width, yuv_heigh, crop, crop_w, crop_h);

    rc = fdr_yuv_nv12_to_rgb1555(yuv, yuv_width, yuv_heigh, rgb);
    // rc = fdr_yuv_nv12_to_rgb24(crop, crop_w, crop_h, rgb);
    if(rc != 0){
        printf("test_to_rgb24 => fdr_yuv_nv12_to_rgb24 fail\n");
        return -1;
    }
    
    rc = save_bmp_file(rgb24file, rgb, yuv_width, yuv_heigh, chs);
    if(rc != 0){
        printf("test_to_rgb24 => save_bmp_file fail %d\n", rc);
        return -1;
    }

    free(rgb);

    return 0;
}

#if 1
static int read_bmpfile(const char *filepath){ 
    lvgui_bmp_header_t header = {};
    lvgui_bmp_info_t   info = {};
    FILE *fj;
    long size;
    uint8_t *fbuf;

    fj = fopen(filepath, "r");
    if(fj == NULL){
        printf("read_bmpfile => open %s fail!\n", filepath);
        return -1;
    }

    if(fread(&header, 1, sizeof(header), fj) != sizeof(header)){
        printf("read_bmpfile => load header fail!\n");
        fclose(fj);
        return -1;
    }

    if(fread(&info, 1, sizeof(info), fj) != sizeof(info)){
        printf("read_bmpfile => load info fail!\n");
        fclose(fj);
        return -1;
    }

    fclose(fj);

    printf("bfSize:%d , Offbits:%d\n", header.bfSize, header.bfOffBits);

    printf("biSize:%d\n", info.biSize);
    printf("biWidth:%d , biHeight:%d\n", info.biWidth, info.biHeight);
    printf("biPlanes:%d , biBitCount:%d\n", info.biPlanes, info.biBitCount);
    printf("biCompression:%d , biSizeImage:%d\n", info.biCompression, info.biSizeImage);
    printf("biXPelsPerMeter:%d , biYPelsPerMeter:%d\n", info.biXPelsPerMeter, info.biYPelsPerMeter);
    printf("biClrUsed:%d , biClrImportant:%d\n", info.biClrUsed, info.biClrImportant);

    return 0;
}
#endif

int main(){
    int rc;

    read_bmpfile("./test/rgb24.bmp");

    rc = test_to_rgb24();

    return rc;
}
