/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __BISP_HAL_H__
#define __BISP_HAL_H__

#include <inttypes.h>


// void *handle - set by users
// int status - 1, pressed; 0, unpressed, or other inputable character
typedef int (*bisp_hal_key_dispatch)(int status);

// set key callback
int bisp_hal_set_keydispatch(bisp_hal_key_dispatch dispatch);


// restart os
void bisp_hal_restart();


// get light sensor lumen
int bisp_hal_get_lumen();

// get PIR sessor status
int bisp_hal_get_pir();

// enable = 1, light on;  enable = 0, light off
int bisp_hal_enable_light(int enable);

// send data to serial port
// idx - id of serial port
// data - for writing data
// len - length of data
int bisp_hal_write_serialport(int idx, const void *data, unsigned int len);
// read data from serial port
int bisp_hal_read_serialport(int idx, void *data, unsigned int *len);

// relay control
int bisp_hal_enable_relay(int idx, int enable);

typedef enum{
    BISP_HAL_VOICE_RECOG_SUCCESS = 0,
    BISP_HAL_VOICE_RECOG_FAILURE,

    BISP_HAL_VOICE_CONFIG_SUCCESS,
    BISP_HAL_VOICE_CONFIG_FAILURE,

    BISP_HAL_VOICE_VISITOR_SUCCESS,
    BISP_HAL_VOICE_VISITOR_FAILURE,

    BISP_HAL_VOICE_ACTIVATE_SUCCESS,
    BISP_HAL_VOICE_ACTIVATE_FAILURE,

    BISP_HAL_VOICE_MAX
}bisp_hal_voice_mode_e;

// void play
int bisp_hal_voice_play(unsigned int mode);

// init hwp
int bisp_hal_init();
// fini hwp
int bisp_hal_fini();

#endif // __BISP_HAL_H__
