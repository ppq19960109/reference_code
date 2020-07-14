/*
 * Copyright (c) 2020, Bashi Tech. All rights reserved.
 */

#ifndef __LVGUI_H__
#define __LVGUI_H__

#include "list.h"

#include <inttypes.h>
#include <time.h>

#define LVGUI_UID_MAX           33
#define LVGUI_NAME_MAX          33
#define LVGUI_DESC_MAX          65

#define LVGUI_MASK_DETECT       0x0001
#define LVGUI_MASK_RECOGN       0x0002

#define LVGUI_MASK_PASS         0x0100
#define LVGUI_MASK_DENY         0x0200
#define LVGUI_MASK_EXPIRED      0x0400
#define LVGUI_MASK_UNKOWN       0x0800

#define LVGUI_SCREEN_PATH       "./gui/800x480"

enum{
    LVGUI_MODE_NONE = 0,
    LVGUI_MODE_INIT,
    LVGUI_MODE_RECO,
    LVGUI_MODE_INFO,
    LVGUI_MODE_VISI,
    LVGUI_MODE_TIPS,
    LVGUI_MODE_ADVT,
    LVGUI_MODE_TEST,
    LVGUI_MODE_MAX
};

typedef struct {
    struct list_head node;

    int fid;
    int mask;

    int xpos, ypos, width, heigh;

    char uid [LVGUI_UID_MAX];
    char name[LVGUI_NAME_MAX];
    char desc[LVGUI_DESC_MAX];

}lvgui_user_t;

int lvgui_init();
int lvgui_fini();
void lvgui_loop();

int  lvgui_decoder_init();

lvgui_user_t *lvgui_alloc_user();
int lvgui_free_user(lvgui_user_t *user);

int lvgui_dispatch(lvgui_user_t *user);

int lvgui_setmode(int mode, int seconds);

int lvgui_therm_update(float therm);

#endif // __LVGUI_H__
