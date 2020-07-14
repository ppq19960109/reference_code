/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __BISP_AIE_H__
#define __BISP_AIE_H__

#include <stdlib.h>
#include <inttypes.h>

#define BISP_AIE_FDRVECTOR_MAX          512
#define BISP_AIE_IMAGE_WIDTH            512
#define BISP_AIE_IMAGE_HEIGHT           512

#define BISP_AIE_FRFACEMAX              6       //可同时识别的人脸数

// face feature vector
typedef struct bisp_aie_ffv{
    float vector[BISP_AIE_FDRVECTOR_MAX];
}bisp_aie_ffv_t;

// rect
typedef struct bisp_aie_rect{
    uint16_t x0;
    uint16_t y0;
    uint16_t x1;
    uint16_t y1;
}bisp_aie_rect_t;

// face recognization return value
typedef struct bisp_aie_frval{
    int                 id;
    bisp_aie_rect_t     rect;
    bisp_aie_ffv_t      feature;
    unsigned int        paddings[16];
}bisp_aie_frval_t;

// int dnum,  void *dfs - detected face info
// int rnum, bisp_aie_frval_t *rfs - recognized face info
// void *frame - frame data
// return 0, frame is release by  handle module, others release by hwp
typedef int (*bisp_aie_fdr_dispatch)(int dnum, void *dfs, int rnum, bisp_aie_frval_t *rfs, void *frame);

// const char *qrcode - recognized qrcode strings
// int len - length of qrcode
typedef int (*bisp_aie_qrc_dispatch)(const char *qrcode, int len);

void bisp_aie_set_fdrdispatch(bisp_aie_fdr_dispatch dispatch);
void bisp_aie_set_qrcdispatch(bisp_aie_qrc_dispatch dispatch);

// write frame data to jpeg file
int bisp_aie_writetojpeg(const char *path, void *frame);

// get algm face feature
// void *img, int type, - image data, type, NV12 for now
// int width, int heigh, - image size
// float *feature, int *len - face feature buffer, alloc by input, len - input/output size
int bisp_aie_get_fdrfeature(void *img, int type, int width, int heigh, float *feature, size_t *len);


float bisp_aie_matchfeature(float *feature1, float *feature2, int len);

enum {
    BISP_AIE_MODE_FACE = 0,
    BISP_AIE_MODE_QRCODE,
    BISP_AIE_MODE_NONE,
};

// release frame data
void bisp_aie_release_frame(void *frame);

//int mode, - mode of this device, face rec or qrcode rec;
int bisp_aie_set_mode(int mode);

int bisp_aie_version();

int bisp_aie_init();
int bisp_aie_fini();

#endif // __BISP_AIE_H__
