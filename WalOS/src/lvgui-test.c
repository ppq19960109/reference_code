/*
 * Copyright (c) 2020, Bashi Tech. All rights reserved.
 */

#include "lvgui.h"
#include "logger.h"

#include "fdrcp-utils.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static lvgui_user_t *new_user(int count){
    lvgui_user_t *user;

    user = lvgui_alloc_user();
    if(user == NULL){
        log_warn("new_user => alloc user fail\n");
        return NULL;
    }

    strcpy(user->uid, "LVGUI_TEST_ID0x00");
    sprintf(user->name, "%s.%d","张三", count);
    strcpy(user->desc, "国际贸易与沟通部");

    user->mask = LVGUI_MASK_PASS | LVGUI_MASK_RECOGN;

    user->xpos = 0;
    user->ypos = 0;
    user->width = 200;
    user->heigh = 200;

    return user;
}

static void *lvgui_test_thread_proc(void *args){
    lvgui_user_t *user;
    int count = 0;
    do{
        /*
        user = new_user(count);
        if(user != NULL){
            log_info("lvgui_test_thread_proc => %d\n", count);
            lvgui_dispatch(user);
        }
        count ++;

        sleep(3);

        user = new_user(count);
        if(user != NULL){
            log_info("lvgui_test_thread_proc => %d\n", count);
            lvgui_dispatch(user);
        }
        count ++;

        sleep(3);
        lvgui_setmode(LVGUI_MODE_TIPS, 2);
        sleep(3);

        user = new_user(count);
        if(user != NULL){
            log_info("lvgui_test_thread_proc => %d\n", count);
            lvgui_dispatch(user);
        }
        count ++;
        
        sleep(3);

        lvgui_setmode(LVGUI_MODE_TIPS, 2);
        sleep(2);
        */
        lvgui_setmode(LVGUI_MODE_TEST, 2);
        sleep(10);

    }while(0);

    return NULL;
}

int lvgui_test_init(){
    pthread_t thrd;
    int rc;

    log_info("lvgui_test_init => ...\n");

    rc = pthread_create(&thrd, NULL, lvgui_test_thread_proc, NULL);
    if(rc != 0){
        log_warn("lvgui_test_init => create tick thread fail %d\n", rc);
        return -1;
    }

    return 0;
}
