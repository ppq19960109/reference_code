/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */


#include "../../lvgl.h"

#include "lv_font_ft.h"

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/freetype.h>

#define LV_FONT_FT_DEFAULT_MARGIN           4
#define LV_FONT_FT_DEFAULT_SIZE             18
#define LV_FONT_FT_DEFAULT_BASELINE         6
#define LV_FONT_FT_BUFFER_SIZE              (128*128)

typedef struct lv_font_freetype_dsc{
    lv_font_t   *font;

    FT_Library  ftlib;
    FT_Face     ftface;

    uint8_t size;
    uint8_t margin;

    uint8_t inited;
    
    uint32_t last_letter;
    uint8_t buffer[LV_FONT_FT_BUFFER_SIZE];
}lv_font_freetype_dsc_t;


static const uint8_t * lv_font_freetype_get_bitmap(const lv_font_t * font, uint32_t unicode_letter);
static bool lv_font_freetype_get_dsc(const lv_font_t * font, lv_font_glyph_dsc_t *gd,
                                    uint32_t letter, uint32_t letter_next);

static lv_font_freetype_dsc_t font_dsc = {};

lv_font_t lv_font_freestyle ={
    .dsc = &font_dsc,
    .get_glyph_bitmap = lv_font_freetype_get_bitmap,
    .get_glyph_dsc = lv_font_freetype_get_dsc,
    .line_height = 18,
    .base_line = 4,
};

int lv_font_freestyle_init(const char *fontfile){
    lv_font_t *font = &lv_font_freestyle;
    lv_font_freetype_dsc_t *dsc = &font_dsc;
	int rc;

	if( fontfile == NULL )
		return -1;

    dsc->font  = font;
    dsc->size = LV_FONT_FT_DEFAULT_SIZE;
    dsc->margin = LV_FONT_FT_DEFAULT_MARGIN;

	rc = FT_Init_FreeType(&dsc->ftlib);
	if(rc != 0){
        printf("FT_Init_FreeType fail:%d\n", rc);
        return -1;
    }

	rc = FT_New_Face(dsc->ftlib, fontfile, 0, &dsc->ftface);
	if(rc != 0){
        printf("FT_New_Face fail:%d\n", rc);
        FT_Done_FreeType(dsc->ftlib);
        return -1;
    }

	FT_Set_Pixel_Sizes(dsc->ftface, LV_FONT_FT_DEFAULT_SIZE, 0);

    dsc->inited = 1;

    printf("lv_font_ft_load => load freetype font ok\n");

    return 0;
}

void lv_font_ft_set(uint8_t size, uint8_t margin){
    lv_font_t *font = &lv_font_freestyle;
    lv_font_freetype_dsc_t *dsc = (lv_font_freetype_dsc_t *)font->dsc;

    font->line_height = size;
    font->base_line = size / 5;
    dsc->size = size;
    dsc->margin = margin;

	FT_Set_Pixel_Sizes(dsc->ftface, size, 0);
}

void lv_font_ft_get(uint8_t *size, uint8_t *margin){
    lv_font_t *font = &lv_font_freestyle;
    lv_font_freetype_dsc_t *dsc = (lv_font_freetype_dsc_t *)font->dsc;

    *size = dsc->size;
    *margin  = dsc->margin;
}

/**
 * Generic bitmap get function used in 'font->get_bitmap' when the font contains all characters in the range
 * @param font pointer to font
 * @param unicode_letter an unicode letter which bitmap should be get
 * @return pointer to the bitmap or NULL if not found
 */
static const uint8_t * lv_font_freetype_get_bitmap(const lv_font_t * font, uint32_t unicode_letter)
{
    lv_font_freetype_dsc_t *dsc = (lv_font_freetype_dsc_t*)font->dsc;
	FT_GlyphSlot  slot;
	int error;

    // printf("lv_font_freetype_get_bitmap => %c\n", unicode_letter);

    slot = dsc->ftface->glyph;
	if(dsc->last_letter != unicode_letter){

	    error = FT_Load_Char(dsc->ftface, unicode_letter, FT_LOAD_RENDER);
	    if (error != 0){
            printf("lv_font_freetype_get_bitmap => get glyph:%d fail\n", unicode_letter);
		    return NULL;
	    }

	    dsc->last_letter = unicode_letter;
    }

    memset(dsc->buffer, 0,  dsc->size * slot->bitmap.width);
    memcpy(dsc->buffer, slot->bitmap.buffer, slot->bitmap.rows*slot->bitmap.width);

    return dsc->buffer;
}


static bool lv_font_freetype_get_dsc(const lv_font_t * font, lv_font_glyph_dsc_t *gd,
                                    uint32_t letter, uint32_t letter_next){
    lv_font_freetype_dsc_t *dsc = (lv_font_freetype_dsc_t*)font->dsc;
	FT_GlyphSlot  slot;
    FT_Glyph_Metrics *gm;
    FT_Bitmap  *bp;
	int error;

    // printf("lv_font_freetype_get_dsc => %c\n", letter);

	if(dsc->last_letter != letter){
	    error = FT_Load_Char(dsc->ftface, letter, FT_LOAD_RENDER);
	    if (error != 0){
		    return false;
	    }
	    dsc->last_letter = letter;
	}

    slot = dsc->ftface->glyph;
    gm  = &slot->metrics;
    bp = &slot->bitmap;

#if 0
    gd->adv_w = (gm->horiAdvance >> 6) + dsc->margin;
    gd->box_w = slot->bitmap.width;
    gd->box_h = gm->vertAdvance >> 6;
    gd->ofs_x = slot->bitmap_left;
    gd->ofs_y = slot->bitmap_top;
#endif
    gd->adv_w = (gm->horiAdvance >> 6) + dsc->margin;
    gd->box_w = slot->bitmap.width;
    gd->box_h = gm->vertAdvance >> 6;
    gd->ofs_x = slot->bitmap_left;
    //gd->ofs_y = (gm->horiBearingY >> 6);
    gd->ofs_y =   (gm->horiBearingY >> 6) - font->line_height;
    
    switch(bp->pixel_mode){
    case FT_PIXEL_MODE_MONO:
        gd->bpp  = 1;
        break;

    case FT_PIXEL_MODE_GRAY:
        gd->bpp = 8;
        break;

    case FT_PIXEL_MODE_GRAY2:
        gd->bpp = 2;
        break;

    case FT_PIXEL_MODE_GRAY4:
        gd->bpp = 4;
        break;

    case FT_PIXEL_MODE_LCD:
        gd->bpp = 8;
        break;

    case FT_PIXEL_MODE_LCD_V:
        gd->bpp = 8;
        break;

    case FT_PIXEL_MODE_BGRA:
        gd->bpp = 32;
        break;

    default:
        printf("lv_font_freetype_get_dsc => bp->pixel_mode : %d\n", bp->pixel_mode);
        return false;
    }
    
    //printf("lv_font_freetype_get_dsc =>  adv_w:%d， box_w:%d, box_h:%d, ofs_x:%d, ofs_y:%d, bpp:%d\n",
    //         gd->adv_w, gd->box_w, gd->box_h, gd->ofs_x, gd->ofs_y, gd->bpp);

    return true;
}

