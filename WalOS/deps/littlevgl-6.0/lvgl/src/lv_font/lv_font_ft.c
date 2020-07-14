/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */


#include "../../lvgl.h"

#include "lv_font_ft.h"

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/freetype.h>

#include <pthread.h>

/* On OSX spinlock = mutex */
#if defined(__APPLE__)
    #define pthread_spinlock_t      pthread_mutex_t
    #define pthread_spin_lock       pthread_mutex_lock
    #define pthread_spin_unlock     pthread_mutex_unlock

    #define pthread_spin_init(__lock,__share)       pthread_mutex_init(__lock, NULL)
    #define pthread_spin_destroy    pthread_mutex_destroy
#endif

#define LV_FONT_FT_DEFAULT_MARGIN           4
#define LV_FONT_FT_DEFAULT_SIZE             18
#define LV_FONT_FT_DEFAULT_BASELINE         4

#define LV_FONT_FT_24_MARGIN                0
#define LV_FONT_FT_24_SIZE                  24
#define LV_FONT_FT_24_BASELINE              5

#define LV_FONT_FT_32_MARGIN                0
#define LV_FONT_FT_32_SIZE                  32
#define LV_FONT_FT_32_BASELINE              6

#define LV_FONT_FT_BUFFER_SIZE              (128*128)

typedef struct lv_font_freetype_dsc{
    lv_font_t   *font;

    uint8_t size;
    uint8_t margin;

    uint8_t inited;
    
    uint32_t last_letter;
    uint8_t buffer[LV_FONT_FT_BUFFER_SIZE];
}lv_font_freetype_dsc_t;


static const uint8_t * lv_font_freetype_get_bitmap(const lv_font_t * font, uint32_t unicode_letter);
static bool lv_font_freetype_get_dsc(const lv_font_t * font, lv_font_glyph_dsc_t *gd,
                                    uint32_t letter, uint32_t letter_next);

static lv_font_freetype_dsc_t font_dsc = {
    .size = LV_FONT_FT_DEFAULT_SIZE,
    .margin = LV_FONT_FT_DEFAULT_MARGIN,
    .inited = 1,
};

static lv_font_freetype_dsc_t font_dsc24 = {
    .size = LV_FONT_FT_24_SIZE,
    .margin = LV_FONT_FT_24_MARGIN,
    .inited = 1,
};

static lv_font_freetype_dsc_t font_dsc32 = {
    .size = LV_FONT_FT_32_SIZE,
    .margin = LV_FONT_FT_32_MARGIN,
    .inited = 1,
};

lv_font_t lv_font_freestyle ={
    .dsc = &font_dsc,
    .get_glyph_bitmap = lv_font_freetype_get_bitmap,
    .get_glyph_dsc = lv_font_freetype_get_dsc,
    .line_height = LV_FONT_FT_DEFAULT_SIZE,
    .base_line = LV_FONT_FT_DEFAULT_SIZE/5,
};

lv_font_t lv_font_ft24 ={
    .dsc = &font_dsc24,
    .get_glyph_bitmap = lv_font_freetype_get_bitmap,
    .get_glyph_dsc = lv_font_freetype_get_dsc,
    .line_height = LV_FONT_FT_24_SIZE,
    .base_line = LV_FONT_FT_24_SIZE/5,
};

lv_font_t lv_font_ft32 ={
    .dsc = &font_dsc32,
    .get_glyph_bitmap = lv_font_freetype_get_bitmap,
    .get_glyph_dsc = lv_font_freetype_get_dsc,
    .line_height = LV_FONT_FT_32_SIZE,
    .base_line = LV_FONT_FT_32_SIZE/5,
};

static FT_Library  ftlib;
static FT_Face     ftface;
static pthread_spinlock_t ftlock;

int lv_font_freestyle_init(const char *fontfile){
    // lv_font_t *font = &lv_font_freestyle;
    lv_font_freetype_dsc_t *dsc = &font_dsc;
    lv_font_freetype_dsc_t *dsc24 = &font_dsc;
    lv_font_freetype_dsc_t *dsc32 = &font_dsc;
	int rc;

	if( fontfile == NULL )
		return -1;

    dsc->font   = &lv_font_freestyle;
    dsc24->font = &lv_font_ft24;
    dsc32->font = &lv_font_ft32;

	rc = FT_Init_FreeType(&ftlib);
	if(rc != 0){
        printf("FT_Init_FreeType fail:%d\n", rc);
        return -1;
    }

	rc = FT_New_Face(ftlib, fontfile, 0, &ftface);
	if(rc != 0){
        printf("FT_New_Face fail:%d\n", rc);
        FT_Done_FreeType(ftlib);
        return -1;
    }

    pthread_spin_init(&ftlock, 0);

    printf("lv_font_ft_load => load freetype font %s ok\n", fontfile);

    return 0;
}

void lv_font_ft_set(uint8_t size, uint8_t margin){
    lv_font_t *font = &lv_font_freestyle;
    lv_font_freetype_dsc_t *dsc = (lv_font_freetype_dsc_t *)font->dsc;

    font->line_height = size;
    font->base_line = size / 5;
    dsc->size = size;
    dsc->margin = margin;

	FT_Set_Pixel_Sizes(ftface, size, 0);
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

    pthread_spin_lock(&ftlock);

	FT_Set_Pixel_Sizes(ftface, dsc->size, 0);

    slot = ftface->glyph;
	if(dsc->last_letter != unicode_letter){

	    error = FT_Load_Char(ftface, unicode_letter, FT_LOAD_RENDER);
	    if (error != 0){
            printf("lv_font_freetype_get_bitmap => get glyph:%d fail\n", unicode_letter);
            pthread_spin_unlock(&ftlock);
		    return NULL;
	    }

	    dsc->last_letter = unicode_letter;
    }

    memset(dsc->buffer, 0,  dsc->size * slot->bitmap.width);
    memcpy(dsc->buffer, slot->bitmap.buffer, slot->bitmap.rows*slot->bitmap.width);

    pthread_spin_unlock(&ftlock);

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

    pthread_spin_lock(&ftlock);
	FT_Set_Pixel_Sizes(ftface, dsc->size, 0);

	if(dsc->last_letter != letter){
	    error = FT_Load_Char(ftface, letter, FT_LOAD_RENDER);
	    if (error != 0){
            pthread_spin_unlock(&ftlock);
		    return false;
	    }
	    dsc->last_letter = letter;
	}

    slot = ftface->glyph;
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

    default:
        printf("lv_font_freetype_get_dsc => bp->pixel_mode : %d\n", bp->pixel_mode);
        pthread_spin_unlock(&ftlock);
        return false;
    }
    
    //printf("lv_font_freetype_get_dsc =>  adv_w:%d， box_w:%d, box_h:%d, ofs_x:%d, ofs_y:%d, bpp:%d\n",
    //         gd->adv_w, gd->box_w, gd->box_h, gd->ofs_x, gd->ofs_y, gd->bpp);

    pthread_spin_unlock(&ftlock);
    return true;
}

