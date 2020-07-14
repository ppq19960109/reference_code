
#ifndef _HAL_IMAGE_H_
#define _HAL_IMAGE_H_

#include "stdint.h"

typedef void (*hal_image_dispatch_func_t)(void *handle, uint32_t idx, uint8_t *image, uint32_t w, uint32_t h);

typedef struct{
    hal_image_dispatch_func_t dispatch;
    void *handle;
}hal_image_dispatch_t;

int32_t hal_image_capture(uint32_t idx, const char *filename);

int32_t hal_image_release(uint32_t idx);

int32_t hal_image_regist_dispatch(hal_image_dispatch_t *dispatch);

int32_t hal_image_init(void);

#endif

