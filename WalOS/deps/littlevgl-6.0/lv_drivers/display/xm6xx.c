/**
 * @file monitor.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "xm6xx.h"
//#ifdef XM6XX

#ifdef XM6XX

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

static char *xm6xx_disp[XM6XX_SCREEN_WIDTH * XM6XX_SCREEN_HEIGHT * sizeof(uint16_t)];


extern int32_t hal_layer_init(void);
extern int32_t hal_bmp_set_layer1(char *bitmap, uint32_t x1, uint32_t y1, uint32_t w, uint32_t h, uint8_t alpha);
extern int32_t hal_bmp_layer_enable(void);

/**
 * Initialize the monitor
 */
void xm6xx_init(void)
{
    // hal_layer_init();
}

static void rgv565_to_rgb1555(char *rgb565, uint32_t w, uint32_t h)
{
    uint16_t *color = (uint16_t *)rgb565;
    uint16_t r;
    uint16_t g;
    uint16_t b;

    uint32_t x;
    uint32_t y;
    uint32_t idx;
    for( y = 0; y < h; y++) {
        for( x = 0; x < w; x++) {
            idx = y * w + x;
            r = color[idx] & 0xF800;
            g = color[idx] & 0x07c0;
            b = color[idx] & 0x001F;
            color[idx] = (r >> 1) | (g >> 1) | b;
        }
    }
}

static void _bitmap_rotate_right(uint16_t *src, uint16_t*dest, uint32_t w, uint32_t h)
{
    uint32_t x;
    uint32_t y;
    uint32_t si;
    uint32_t di;

    for( y = 0; y < h; y++) {
        for( x = 0; x < w; x++) {
            di = (h - 1 - y) + x * h;
            si = x + y * w;
            dest[di] = src[si];
        }
    }
}

static void _bitmap_rotate_left(uint16_t *src, uint16_t*dest, uint32_t w, uint32_t h)
{
    uint32_t x;
    uint32_t y;
    uint32_t si;
    uint32_t di;

    for( y = 0; y < h; y++) {
        for( x = 0; x < w; x++) {
            di = y + (w - 1 - x) * h;
            si = x + y * w;
            dest[di] = src[si];
        }
    }
}


void xm6xx_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    lv_coord_t hres = disp_drv->rotated == 0 ? disp_drv->hor_res : disp_drv->ver_res;
    lv_coord_t vres = disp_drv->rotated == 0 ? disp_drv->ver_res : disp_drv->hor_res;

    printf("x1:%d,y1:%d,x2:%d,y2:%d\n", area->x1, area->y1, area->x2, area->y2);

    /*Return if the area is out the screen*/
    if(area->x2 < 0 || area->y2 < 0 || area->x1 > hres - 1 || area->y1 > vres - 1) {

        lv_disp_flush_ready(disp_drv);
        return;
    }

    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    //uint16_t *rbp = (uint16_t *)malloc(w * h * sizeof(uint16_t));
    rgv565_to_rgb1555((char *)color_p, w, h);
    _bitmap_rotate_left((uint16_t *)color_p, (uint16_t*)xm6xx_disp, w, h);
    hal_bmp_set_layer1((char *)xm6xx_disp, area->x1, area->y1, h, w, 0xff);
    //hal_bmp_set_layer((char *)color_p, area->x1, area->y1, w, h, 0xff);
    hal_bmp_layer_enable();
    //free(rbp);

    printf("x1:%d,y1:%d,w:%d,h:%d\n", area->x1, area->y1, w, h);

    /*IMPORTANT! It must be called to tell the system the flush is ready*/
    lv_disp_flush_ready(disp_drv);
}

#endif
