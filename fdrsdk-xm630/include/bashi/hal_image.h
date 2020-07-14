
#ifndef _HAL_IMAGE_H_
#define _HAL_IMAGE_H_

#include "stdint.h"

typedef enum {
    IMG_FACE_MODE,
    IMG_QRCODE_MODE,
    IMG_INVALID_MODE,
}hal_image_mode_t;

typedef enum {
    IMG_COLOR_TYPE,
    IMG_BLACK_TYPE,
}hal_image_type_t;

typedef int (*hal_qrcode_cbfn)(void *handle, const char *qrcode, int len);


int32_t hal_release_all_frame(void);

int32_t hal_image_init(void);

/*
@brief: translate the image to jpeg.
@parm[in] filename: the jpeg filename.
@parm[in] type: reference to fdr_image_type_t; 
*/
int32_t hal_image_capture(const char *filename, uint8_t type);

int32_t hal_image_set_mode(uint8_t mode);

uint8_t hal_image_get_mode(uint8_t mode);

int32_t hal_set_qrcode_cb(hal_qrcode_cbfn cb, void *handle);

#endif

