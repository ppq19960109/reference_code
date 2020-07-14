
#ifndef _HAL_VIO_H_
#define _HAL_VIO_H_

#include "stdint.h"

#if 0
/*30fps*/
#define ISP_BIN_PATH        "/mnt/mtd/usr/usr/isp -v n &"
#else
/*25fps*/
#define ISP_BIN_PATH        "/var/sd/usr/isp &"
#endif

#define LCD_SIZE_43_INCH    0
#define LCD_SIZE_50_INCH    1
#define LCD_SIZE_TYPE       LCD_SIZE_50_INCH

#if defined(LCD_SIZE_TYPE) && (LCD_SIZE_TYPE == LCD_SIZE_43_INCH)
#define LCD_WIDTH           480
#define LCD_HEIGHT          272
#define MIP_VO_INTERFACE    VO_OUTPUT_480x272_60_S
#else
#define LCD_WIDTH           800
#define LCD_HEIGHT          480
#define MIP_VO_INTERFACE    VO_OUTPUT_800x480_60
#endif

#define IMAGE_SIZE_WIDTH    1280
#define IMAGE_SIZE_HEIGHT   720

#define RGB_REC_VI_CHN  2
#define IR_REC_VI_CHN   3

#define RGB_SHOW_VI_CHN 1
#define IR_SHOW_VI_CHN  4

#define RGB_SHOW_VO_CHN 0
#define IR_SHOW_VO_CHN  1

typedef enum {
    COLOR_MODE,
    BLACK_MODE,
}hal_vo_mode_t;

typedef struct {
    uint32_t fps;
    uint32_t shut;
    uint32_t sagain;
    uint32_t sdgain;
    uint32_t idgain;
}camera_attr_t;

int32_t hal_vio_init(camera_attr_t *attr);

#endif

