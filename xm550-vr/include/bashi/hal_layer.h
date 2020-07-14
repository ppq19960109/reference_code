
#ifndef _HAL_LAYER_H_
#define _HAL_LAYER_H_

#include "hal_types.h"

typedef struct{
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
}hal_rect_t;

int32_t hal_clear_layer(uint32_t x, uint32_t y, uint32_t w, uint32_t h);

int32_t hal_layer_set_rect_corner(hal_rect_t *rect, uint32_t thickness, uint32_t size, uint16_t color);

int32_t hal_layer_enable(void);

int32_t hal_layer_disable(void);

int32_t hal_layer_init(void);

int32_t hal_layer_destroy(void);

#endif
