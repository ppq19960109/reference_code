
#ifndef _HAL_JPEG_PLAYER_H_
#define _HAL_JPEG_PLAYER_H_

#include "stdint.h"

void jpeg_player_image(uint32_t idx, uint8_t *image, uint32_t len);

int jpeg_player_init(void);

int jpeg_player_fini(void);

#endif

