
#ifndef _HAL_JPEG_H_
#define _HAL_JPEG_H_

#include "stdint.h"

int32_t hal_frame_to_jpeg(const char *filename, void *frame);

int32_t hal_jpeg_init(void);

#endif
