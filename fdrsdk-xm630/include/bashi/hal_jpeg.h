
#ifndef _HAL_JPEG_H_
#define _HAL_JPEG_H_

#include "stdint.h"

typedef void (*hal_jpeg_render_t)(uint32_t idx, uint8_t *jpeg, uint32_t len);

typedef enum {
    COLOR_INDEX = 0,
    BLACK_INDEX = 1,
    INVALID_INDEX,
}jpeg_play_index_t;

int32_t hal_frame_to_jpeg(const char *filename, void *frame);

int32_t hal_jpeg_play(uint32_t idx, void *frame);

void hal_jpeg_regist_render(hal_jpeg_render_t render);

int32_t hal_jpeg_init(void);

#endif
