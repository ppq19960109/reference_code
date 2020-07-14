
#ifndef _HAL_LAYER_H_
#define _HAL_LAYER_H_

#include "stdint.h"



typedef enum {
    OSD_TIME_LAYER_E,
    OSD_ONLINE_LAYER_E,
    OSD_IPADDR_LAYER_E,
    MAX_OSD_LAYER_E,
}hal_osd_layer_t;

typedef struct{
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
}hal_rect_corner_t;

int32_t hal_osd_show_time(uint8_t enable);

int32_t hal_bmp_show(char *path, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2,  uint8_t flag, uint8_t trans);

int32_t hal_bmp_set_rect_corner(hal_rect_corner_t *rect, uint32_t thickness, uint32_t size, uint16_t color);

int32_t hal_bmp_set_layer(char *bitmap, uint32_t x1, uint32_t y1, uint32_t w, uint32_t h, uint8_t alpha);

int32_t hal_bmp_set_layer1(char *bitmap, uint32_t x1, uint32_t y1, uint32_t w, uint32_t h, uint8_t alpha);

int32_t hal_bmp_clear_layer(uint32_t x, uint32_t y, uint32_t w, uint32_t h);

int32_t hal_bmp_layer_enable(void);

int32_t hal_bmp_layer_disable(void);

int32_t hal_layer_init(void);

int32_t hal_layer_destory(void);

#endif
