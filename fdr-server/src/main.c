/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */
#include "fdr.h"
#include "bisp.h"
#include "fdr-gui.h"
#include "fdr-cp.h"
#include "fdr-license.h"

#include "logger.h"

#include <unistd.h>

int main(){
    int rc;

    if(fdr_license_init()) {
        log_warn("license init failed\r\n");
        return -1;
    }
    
    rc = bisp_init_before();
    if(rc != 0){
        log_warn("main => bisp_init_before fail %d\n", rc);
        return -1;
    }

    //fdr_gui_init();

    rc = fdr_init();
    if(rc != 0){
        log_warn("main => fdr_init fail %d\n", rc);
        return -1;
    }

    rc = bisp_init_after();
    if(rc != 0){
        log_warn("main => bisp_init_after fail %d\n", rc);
        return -1;
    }

    log_info("main => looping ...\n");
    
    //fdr_gui_loop();

    while(1){
        // FIXME : watch dog feed 
        sleep(1);
    }

    return 0;
}

