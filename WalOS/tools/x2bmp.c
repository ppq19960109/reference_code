/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "lvgui-yuv.h"
#include "lodepng.h"

#include <stdio.h>
#include <stdlib.h>


typedef union
{
    struct
    {
        uint16_t blue : 5;
        uint16_t green : 6;
        uint16_t red : 5;
    } ch;
    uint16_t full;
} bmp_color16_t;

typedef union
{
    struct
    {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
        uint8_t alpha;
    } ch;
    uint32_t full;
} bmp_color32_t;

#define BMP16_COLOR_MAKE(r8, g8, b8) ((bmp_color16_t){{b8 >> 3, g8 >> 2, r8 >> 3}})
#define BMP32_COLOR_MAKE(r8, g8, b8, a8) ((bmp_color32_t){{b8, g8, r8, a8 }})

static void convert_color_depth(uint8_t * img, uint32_t px_cnt, int depth)
{
    bmp_color32_t * img_argb = (bmp_color32_t*)img;
    bmp_color32_t * img_c32 = (bmp_color32_t *) img;
    bmp_color16_t * img_c16 = (bmp_color16_t *)img;
    bmp_color32_t c32;
    bmp_color16_t c16;
    uint32_t i;

    switch(depth){
    case 32:
        for(i = 0; i < px_cnt; i++) {
            c32 = BMP32_COLOR_MAKE(img_argb[i].ch.red, img_argb[i].ch.green, img_argb[i].ch.blue, img_argb[i].ch.alpha);
            img_c32[i].ch.red = c32.ch.blue;
            img_c32[i].ch.blue = c32.ch.red;
        }

        break;
    case 16:
    
        for(i = 0; i < px_cnt; i++) {
            c16 = BMP16_COLOR_MAKE(img_argb[i].ch.blue, img_argb[i].ch.green, img_argb[i].ch.red);
            img_c16[i] = c16;
        }

        break;
        
    default:
        printf("convert_color_depth => unkown depth:%d\n", depth);
        break;
    }
}


#ifdef YUV2BMP
static int nv12_read_file(const char *filepath, uint8_t **buf, int *len){
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
#endif

static const char *srcfile = "./srcfile.nv12";
static const char *dstfile = "./dstfile.bmp";

static unsigned int img_width = 256;
static unsigned int img_height = 256;

int main(){
    uint8_t *raw;
    uint8_t *rgb;

    int chs = 2;

    unsigned int rc;

#ifdef YUV2BMP
    int raw_len;
    int rgb_len;

    rc = nv12_read_file(srcfile, &raw, &raw_len);
    if(rc != 0){
        return rc;
    }

    rgb_len = img_width * img_height * chs;
    rgb = malloc(rgb_len);
    if(rgb == NULL){
        printf("main => alloc memory  for rgb fail\n");
        return -1;
    }

    rc = lvgui_yuv_nv12_to_rgb1555(raw, img_width, img_height, rgb);
    if(rc != 0){
        printf("main => lvgui_yuv_nv12_to_rgb1555 fail\n");
        return -1;
    }
#endif

#define PNG2BMP  1
#ifdef PNG2BMP
    rc = lodepng_decode32_file(&raw, &img_width, &img_height, srcfile);
    if(rc != 0){
        printf("main => load png fail:%s, %d\n", srcfile, rc);
        return -1;
    }

    printf("main => png w:%d, h:%d \n", img_width, img_height);
    
    convert_color_depth(raw, img_width*img_height, 16);
    rgb = raw;
#endif

    rc = lvgui_save_to_bmp(dstfile, rgb, img_width, img_height, 2);
    if(rc != 0){
        printf("main => save_bmp_file fail %d\n", rc);
        return -1;
    }

    free(rgb);

    return 0;
}
