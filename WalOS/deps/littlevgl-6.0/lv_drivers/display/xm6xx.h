/**
 * @file monitor.h
 *
 */

#ifndef XM6XX_H
#define XM6XX_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lv_drv_conf.h"
#else
#include "../../lv_drv_conf.h"
#endif

#include "lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/

#define XM6XX_SCREEN_WIDTH    LV_HOR_RES_MAX
#define XM6XX_SCREEN_HEIGHT   LV_VER_RES_MAX

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void xm6xx_init(void);
void xm6xx_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* XM6XX_H */
