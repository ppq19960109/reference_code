
#ifndef _HAL_VIDEO_H_
#define _HAL_VIDEO_H_

#include "stdint.h"



#define VO_COLOR_MODE 0
#define VO_BLACK_MODE 1
#define VO_NONE_MODE  2

#define LCD_SIZE_43_INCH 0
#define LCD_SIZE_50_INCH 1
#define LCD_SIZE_70_INCH 2

#ifndef VO_MODE
#error "Please select vo mode"
#endif


#if !defined(LCD_SIZE_TYPE)
#error "Please select lcd type"
#elif (LCD_SIZE_TYPE == LCD_SIZE_43_INCH)
#define LCD_WIDTH           480
#define LCD_HEIGHT          272
#define MIP_VO_INTERFACE    VO_OUTPUT_480x272_60_S
#elif (LCD_SIZE_TYPE == LCD_SIZE_50_INCH)
#define LCD_WIDTH           800
#define LCD_HEIGHT          480
#define MIP_VO_INTERFACE    VO_OUTPUT_800x480_60
#elif (LCD_SIZE_TYPE == LCD_SIZE_70_INCH)
#define LCD_WIDTH           1024
#define LCD_HEIGHT          600
#define MIP_VO_INTERFACE    VO_OUTPUT_1024x600_60
#else
#error "unkown lcd type"
#endif


#define FDR_IMAGE_SIZE_WIDTH    1920
#define FDR_IMAGE_SIZE_HEIGHT   1080

#define FDR_RGB_REC_VI_CHN  2
#define FDR_IR_REC_VI_CHN   3

#define FDR_RGB_SHOW_VI_CHN 1
#define FDR_IR_SHOW_VI_CHN  4

#define FDR_RGB_SHOW_VO_CHN 0
#define FDR_IR_SHOW_VO_CHN  1





int32_t hal_vio_disable(void);

int32_t hal_vio_enable(void);

int32_t hal_vio_init(void);

#endif

