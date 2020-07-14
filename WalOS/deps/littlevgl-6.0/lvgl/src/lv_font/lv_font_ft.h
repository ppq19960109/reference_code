/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __LV_FONT_FREETYPE_H__
#define __LV_FONT_FREETYPE_H__

#include "../../lvgl.h"
#include "lv_font.h"

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/freetype.h>

extern int  lv_font_freestyle_init(const char *fontfile);
extern void lv_font_ft_set(uint8_t size, uint8_t margin);
extern void lv_font_ft_get(uint8_t *size, uint8_t *margin);

#endif // __LV_FONT_FREETYPE_H__
