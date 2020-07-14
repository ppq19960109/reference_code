
#ifndef _HAL_AUDIO_H_
#define _HAL_AUDIO_H_

#include "stdint.h"

#define AUDIO_ENABLE_PIN  72

int32_t hal_audio_play(char *filename, uint8_t volume);

int32_t hal_audio_record_start(char *filename);

int32_t hal_audio_record_stop(void);

int32_t hal_audio_init(void);

int32_t hal_audio_destroy(void);

#endif
