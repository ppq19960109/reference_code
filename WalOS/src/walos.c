/*
 * Copyright (c) 2020, Bashi Tech. All rights reserved.
 */

#include "fdrcp.h"
#include "fdrcp-utils.h"

#include "lvgui.h"

#include "bisp.h"
#include "logger.h"

#include <event2/thread.h>

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

extern int fdrmp_init();

static void init_supports(){

    // init debug logger
    logger_init();

    // make libevent surpport threads
    log_info("init_supports => enable event library with pthreads\n");
    evthread_use_pthreads();

    // init fdrcp memory allocator
    fdrcp_mem_init();
}

extern int lvgui_test_init();
extern int fdrcp_test_init();

static void hal_init(){
        hal_vio_init();

    hal_layer_init();
    hal_window_disable();
    hal_osd_window_disable();

    hal_jpeg_init();

    jpeg_player_init();
    // // hal_jpeg_regist_render(jpeg_player_image);

    // hal_fdr_init(0);

    // //hal_image_set_mode(IMG_QRCODE_MODE);
    // //hal_set_qrcode_cb(qrcode_handler, NULL);
    // hal_image_init();


}

int main(int argc, char *argv[]){
    int rc;

    // init support libraries
    init_supports();

    hal_init();

    // init bisp - first half
    rc = bisp_init_before();
    if(rc != 0){
        log_warn("main => bisp_init_before fail %d\n", rc);
        return -1;
    }

    // init lvgui
    lvgui_init();

    // init control plane
    rc = fdrcp_init();
    if(rc != 0){
        log_warn("main => fdr_init fail %d\n", rc);
        return -1;
    }

    // init bisp - second half
    rc = bisp_init_after();
    if(rc != 0){
        log_warn("main => bisp_init_after fail %d\n", rc);
        return -1;
    }

    // start management plane
    // fdrmp_init();

    // init test, if need
    //lvgui_test_init();
    fdrcp_test_init();

    // lvgui loop
    lvgui_loop();

    //while(1){
        // FIXME : watch dog feed
    //    sleep(1);
    //}

    return 0;
}

