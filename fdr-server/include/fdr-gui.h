/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __FDR_GUI_H__
#define __FDR_GUI_H__

#include "list.h"
#include "fdr-dbs.h"

#define FDR_GUI_SCREEN_WIDTH    480
#define FDR_GUI_SCREEN_HEIGH    800

#define FDR_GUI_MAIN_WIN_HEIGHT  160

#define FDR_GUI_USERIMAGE_SIZE  128

#define FDR_GUI_USERID_MAX      32
#define FDR_GUI_NAME_MAX        32

#define FDR_GUI_RESULT_MAX      12

struct fdr_gui;
typedef struct fdr_gui  fdr_gui_t;

typedef struct fdr_gui_fid{
    int fid;
    int type;
    int xpos, ypos, width, heigh;
    char uid[FDR_GUI_USERID_MAX];
    char name[FDR_GUI_NAME_MAX];
}fdr_gui_fid_t;

typedef struct fdr_gui_frdresults{
    int fdn;
    int frn;
    fdr_gui_fid_t fids[FDR_GUI_RESULT_MAX];
}fdr_gui_frdresults_t;

typedef struct fdr_gui_user{
    struct list_head node;

    uint32_t fid;   // face id in tracker

    char userid[FDR_DBS_USERID_SIZE];
    char name[FDR_DBS_USERNAME_SIZE];
    char desc[FDR_DBS_USERDESC_SIZE];

    int64_t  timestamp;
    int state;      // perm, expired, unkown
    uint16_t x, y, w, h;    // face position in image

}fdr_gui_user_t;

int fdr_gui_init();
int fdr_gui_fini();

int fdr_gui_decoder_png();
int fdr_gui_decoder_jpg();

void fdr_gui_loop();

int fdr_gui_acquire(fdr_gui_user_t **user);
int fdr_gui_release(uint32_t fid);

int fdr_gui_dispatch(uint32_t fid, fdr_gui_user_t *user);

int fdr_gui_setmode(int mode);

#endif // __FDR_GUI_H__
