/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __BISP_GUI_H__
#define __BISP_GUI_H__

typedef enum{
    BISP_GUI_FDRMODE_START,                     // startup
    BISP_GUI_FDRMODE_FACEREC,                   // face recognizing
    BISP_GUI_FDRMODE_AC_CONNECTED,              // activate tips page, connected to cloud
    BISP_GUI_FDRMODE_AC_UNCONNECTED,            // activate tips page, unconnected to cloud
    BISP_GUI_FDRMODE_STRANGER,                  // stranger tips page
    BISP_GUI_FDRMODE_VI_SCAN,                   // visitor scan qrcode page
    BISP_GUI_FDRMODE_VI_SUCCESS,                // visitor code ok
    BISP_GUI_FDRMODE_VI_FAILURE,                // visitor code failure
    BISP_GUI_FDRMODE_CF_LOCAL,                  // config tips page, local mode
    BISP_GUI_FDRMODE_CF_CLOUD_CONNECTED,        // config tips page, cloud connected
    BISP_GUI_FDRMODE_CF_CLOUD_UNCONNECTED,      // config tips page, cloud unconnected
    BISP_GUI_FDRMODE_USER_REC,                  //the special one.
    BISP_GUI_FDRMODE_MAX,
}bisp_gui_fdrmode_e;

typedef enum {
    BISP_GUI_FDRWIN_NONE = 0,  // don't display
    BISP_GUI_FDRWIN_PERM,
    BISP_GUI_FDRWIN_NOPERM,
    BISP_GUI_FDRWIN_EXPIRED,
    BISP_GUI_FDRWIN_MAX,
}bisp_gui_fdrwin_e;

// bisp gui init / fini
int bisp_gui_init();
int bisp_gui_fini();

// show fdr mode : active, config, visitor, stranger
int bisp_gui_show_fdrmode(uint32_t mode);

// show fdr recognized window
int bisp_gui_show_fdrwin(void *bitmap, int width, int heigh, bisp_gui_fdrwin_e perm);

#endif // __BISP_GUI_H__
