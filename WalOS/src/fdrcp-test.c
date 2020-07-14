/*
 * Copyright (c) 2020, Bashi Tech. All rights reserved.
 */

#include "fdrcp.h"
#include "fdrcp-utils.h"

#include "list.h"
#include "logger.h"

#include "bisp_aie.h"

#include <inttypes.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>


#define FDRCP_TEST_FRAME_DATA           "./gui/800x480/init.png"

#define FDRCP_TEST_UNAME_MAX            8
#define FDRCP_TEST_UDESC_MAX            8
#define FDRCP_TEST_FEATURE_MAX          8
#define FDRCP_TEST_FEATURE_VECMAX       512

#define FDRCP_TEST_USER_MAX             256

#define FDRCP_TEST_RECO_RATIO           4

static char uname[FDRCP_TEST_UNAME_MAX][16] = {
    "张三.诺夫",
    "李四.斯基",
    "王五.博格",
    "陆六.松下",
    "刘七.田边",
    "赵八.小子",
    "马九九",
    "胡十赌",
};

static char udesc[FDRCP_TEST_UDESC_MAX][33] = {
    "市场与营销部",
    "中央研究院开发部",
    "未来技术研究部",
    "NASA R&D",
    "国际贸易部",
    "财务部",
    "人力资源与战略规划部",
    "供应链体系采购部",
};

typedef struct { float vector[FDRCP_TEST_FEATURE_VECMAX]; int used; } feature_t;
static feature_t features[FDRCP_TEST_FEATURE_MAX];
static bisp_aie_frame_t frame_data;

#ifdef XM6XX
#define arc4random  (int)random
#endif

int myrandom(){
    int r = arc4random();
    if(r < 0){
        return -r;
    }else{
        return r;
    }
}

static void feature_init_array(float *vec){
    float *f;
    int j;

    srand((unsigned int)time(NULL));

    for(j = 0; j < FDRCP_TEST_FEATURE_VECMAX; j++){
        f = &vec[j];
        *f = (float)arc4random()/(float)(RAND_MAX/2);
    }
}

static void feature_init(){
    feature_t *ft;
    int i;

    for(i = 0; i < FDRCP_TEST_FEATURE_MAX; i++){
        ft = &features[i];

        ft->used = 0;
        feature_init_array(ft->vector);
    }
}

static void fdrcp_test_init_repo(){
    fdrcp_user_t *user;
    int un, ud, uf = 0;
    int runout = 0;
    int i;

    srand((unsigned int)time(NULL));

    for(i = 0; i < FDRCP_TEST_USER_MAX; i++){
        user = fdrcp_acquire_user();
        if(user == NULL){
            log_warn("fdrcp_test_init_repo -> fdrcp_acquire_user fail %d\n", i);
            return ;
        }
        memset(user, 0, sizeof(*user));

        un = myrandom() % FDRCP_TEST_UNAME_MAX;
        ud = myrandom() % FDRCP_TEST_UDESC_MAX;

        snprintf(user->usrid, 32, "TUID-%d%d", i, myrandom()%10000);
        strncpy(user->uname, uname[un], 16);
        strncpy(user->udesc, udesc[ud], 32);
        user->rule = 1;
        user->expired = time(NULL) + 24*3600*365;

        if((i%(FDRCP_TEST_RECO_RATIO * 4)) == 7){
            memcpy(user->feature, features[uf].vector, sizeof(float) * FDRCP_TEST_FEATURE_VECMAX);
            uf ++;

            if(uf == FDRCP_TEST_FEATURE_MAX)
                runout = 1;
        }else{
            feature_init_array(user->feature);
        }

        //log_info("fdrcp_test_init_repo => %s, %s, %f\n", user->usrid, user->uname, user->feature[2]);

        if(fdrcp_usrdispatch(FDRCP_EVMSG_USER_APPEND, user) != 0){
            log_warn("fdrcp_test_init_repo -> fdrcp_usrdispatch fail %d\n", i);
            break;
        }

        if(runout){
            //log_info("fdrcp_test_init_repo -> runout %d\n", i);
            break;
        }

        usleep(10000);
    }
}

static void fdrcp_test_fd_random_init(fdrcp_fd_t *fd, int tfid){
    memset(fd, 0, sizeof(*fd));

    fd->tfid = tfid;
}

static void fdrcp_test_fr_random_init(fdrcp_fr_t *fr, int tfid){
    fr->tfid = tfid;

    if((myrandom()%FDRCP_TEST_RECO_RATIO) == 1){

        int uf = myrandom() % FDRCP_TEST_FEATURE_MAX;
        memcpy(fr->feature, features[uf].vector, sizeof(float) * FDRCP_TEST_FEATURE_VECMAX);
        log_info("fdrcp_test_fr_random_init -> uf %d\n", uf);

    }else{
        log_info("fdrcp_test_fr_random_init -> rand feature \n");
        feature_init_array(fr->feature);
    }
}

static fdrcp_data_t *new_data(int num, int tfid){
    fdrcp_data_t *d;
    int i;

    d = fdrcp_acquire();
    if(d == NULL){
        log_warn("new_data -> fdrcp_acquire fail\n");
        return NULL;
    }
    memset(d, 0, sizeof(*d));

    d->timestamp = fdrcp_timestamp();
    d->fdnum = num;
    d->frnum = num;
    for(i = 0; i < num; i++){
        fdrcp_fd_t *fd = &d->fds[i];
        fdrcp_test_fd_random_init(fd, tfid + i);

        fdrcp_fr_t *fr = &d->frs[i];
        fdrcp_test_fr_random_init(fr, tfid + i);
    }

    d->frame = &frame_data;

    return d;
}

static void *fdrcp_test_thread_proc(void *args){
    fdrcp_data_t *data;
    int tfid = 100;
    int us;
    
    do{
        data = new_data(1, tfid);

        log_info("fdrcp_test_thread_proc -> dispatch user to fdrcp \n");
        fdrcp_fdrdispatch(data);

        us = myrandom();
        if(us < 0)
            us = -us;
        us = (us % 700) + 500;
        
        us *= 1000;
        usleep(us);

        if(myrandom() % 2){
            tfid ++;
        }

    }while(1);

    return NULL;
}

int fdrcp_test_init(){
    pthread_t thrd;
    unsigned char *buf;
    int len;
    int rc;

    sleep(1);

    feature_init();

    len = 1024*1024;
    buf = malloc(len);
    if(buf == NULL){
        log_warn("fdrcp_test_init => alloc buffer fail\n");
        return -1;
    }

    if(fdrcp_utils_read_file(FDRCP_TEST_FRAME_DATA, 0, buf, &len) != 0){
        log_warn("fdrcp_test_init => read frame data fail\n");
        return -1;
    }

    frame_data.data = buf;
    frame_data.len = len;
    
    fdrcp_test_init_repo();
    
    rc = pthread_create(&thrd, NULL, fdrcp_test_thread_proc, NULL);
    if(rc != 0){
        log_warn("fdrcp_test_init => create thread fail %d\n", rc);
        return -1;
    }

    return 0;
}
