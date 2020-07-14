/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */
#include "bisp_aie.h"
#include "bisp_hal.h"
#include "bisp_gui.h"

#include "logger.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

static bisp_aie_fdr_dispatch    recognize_dispatch = NULL;

static bisp_hal_key_dispatch    keytouch_dispatch = NULL;

static bisp_aie_qrc_dispatch    qrcode_dispatch =  NULL;

#define IMAGE_WIDTH             512
#define IMAGE_HEIGHT            512
#define LOG_IMAGE_DATA_SIZE     (IMAGE_WIDTH * IMAGE_HEIGHT *3/2)
#define FEATURE_VECTOR_MAX      512

void bisp_aie_set_fdrdispatch(bisp_aie_fdr_dispatch dispatch){
    recognize_dispatch= dispatch;
}

void bisp_aie_set_qrcdispatch(bisp_aie_qrc_dispatch dispatch){
    qrcode_dispatch = dispatch;
}

// write frame data to jpeg file
int bisp_aie_writetojpeg(const char *path, void *frame){
    FILE *f;
    bisp_aie_frame_t *baf = (bisp_aie_frame_t *)frame;

    log_info("bisp_aie_writetojpeg => %s\n", path);

    f = fopen(path, "w");
    if(f != NULL){
        fwrite(baf->data, 1, baf->len, f);
        fclose(f);

        return 0;
    }
    
    return -1;
}

#ifndef TEST
// release frame data
void bisp_aie_release_frame(void *frame){
    // free(frame);
}

#endif

// get algm face feature
// void *img, int type, - image data, type, NV12 for now
// int width, int heigh, - image size
// float *feature, int *len - face feature buffer, alloc by input, len - input/output size
int bisp_aie_get_fdrfeature(void *img, int type, int width, int heigh, float *feature, size_t *len){
    usleep(500000);
    return 0;
}

int bisp_aie_set_mode(int mode){
    log_info("call bisp_aie_set_mode:%d\n", mode);
    return 0;
}

int bisp_aie_init(){
    return 0;
}

int bisp_aie_fini(){
    return 0;
}

// set key callback
int bisp_hal_set_keydispatch(bisp_hal_key_dispatch dispatch){
    keytouch_dispatch = dispatch;
    return 0;
}


// restart os
void bisp_hal_restart(){
}


// get light sensor lumen
int bisp_hal_get_lumen(){
    return 0;
}

// get PIR sessor status
int bisp_hal_get_pir(){
    return 0;
}

// enable = 1, light on;  enable = 0, light off
int bisp_hal_enable_light(int enable){
    return 0;
}

// send data to serial port
// idx - id of serial port
// data - for writing data
// len - length of data
int bisp_hal_write_serialport(int idx, const void *data, unsigned int len){
    return 0;
}

// read data from serial port
int bisp_hal_read_serialport(int idx, void *data, unsigned int *len){
    return 0;
}

// relay control
int bisp_hal_enable_relay(int idx, int enable){
    return 0;
}

// void play
int bisp_hal_voice_play(unsigned int mode){
    return 0;
}

// init hwp
int bisp_hal_init(){
    return 0;
}

// fini hwp
int bisp_hal_fini(){
    return 0;
}


// bisp gui init / fini
int bisp_gui_init(){
    return 0;
}

int bisp_gui_fini(){
    return 0;
}

// show fdr mode : active, config, visitor, stranger
int bisp_gui_show_fdrmode(bisp_gui_fdrmode_e mode){
    log_info("call bisp_gui_show_fdrmode: %d\n", mode);
    return 0;
}

// show fdr recognized window
int bisp_gui_show_fdrwin(void *bitmap, int width, int heigh, bisp_gui_fdrwin_e perm){
    log_info("call bisp_gui_show_fdrwin: %d\n", perm);
    return 0;
}

int bisp_aie_version(){
    return (0x02 << 16) + (0x01 << 8)  + 2;
}

int bisp_init_before(){
    return 0;
}

int bisp_init_after(){
    return 0;
}
