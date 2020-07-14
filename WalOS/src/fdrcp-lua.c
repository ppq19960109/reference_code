/*
 * Copyright (c) 2020, Bashi Tech. All rights reserved.
 */
#include "fdrcp.h"
#include "fdrcp-utils.h"
#include "lvgui-yuv.h"

#include "bisp_aie.h"

#include "list.h"
#include "logger.h"

#include <inttypes.h>
#include <time.h>
#include <string.h>

#if 1
#define LOGINFO log_info
#else
#define LOGINFO(...)
#endif

int config_set(const char *key, const char *val){
    LOGINFO("config_set : %s, %s\n", key, val);
    return fdrcp_conf_set(key, val);
}

int user_append(const char *uid, const char *uname, const char *udesc, int rule, long expired, const float *feature){
    fdrcp_user_t *user = fdrcp_acquire_user();
    if(user == NULL){
        log_warn("user_append => fdrcp_acquire_user fail\n");
        return -1;
    }

    LOGINFO("user_append : %s, %s\n", uid, uname);

    strncpy(user->usrid, uid,   FDRCP_USRID_MAX);
    strncpy(user->uname, uname, FDRCP_UNAME_MAX);
    strncpy(user->udesc, udesc, FDRCP_UDESC_MAX);

    user->rule = rule;
    user->expired = expired;

    memcpy(user->feature, feature, FDRCP_FEATURE_MAX);

    if(fdrcp_usrdispatch(FDRCP_EVMSG_USER_APPEND, user) != 0){
        log_warn("user_append => fdrcp_usrdispatch fail\n");
        fdrcp_release_user(user);
        return -1;
    }

    return 0;
}

int user_update(const char *uid, const char *uname, const char *udesc, int rule, long expired, const float *feature){
    fdrcp_user_t *user = fdrcp_acquire_user();
    if(user == NULL){
        log_warn("user_update => fdrcp_acquire_user fail\n");
        return -1;
    }

    LOGINFO("user_update : %s, %s\n", uid, uname);

    strncpy(user->usrid, uid,   FDRCP_USRID_MAX);
    strncpy(user->uname, uname, FDRCP_UNAME_MAX);
    strncpy(user->udesc, udesc, FDRCP_UDESC_MAX);

    user->rule = rule;
    user->expired = expired;

    memcpy(user->feature, feature, FDRCP_FEATURE_MAX);

    if(fdrcp_usrdispatch(FDRCP_EVMSG_USER_UPDATE, user) != 0){
        fdrcp_release_user(user);
        log_warn("user_update => fdrcp_usrdispatch fail\n");
        return -1;
    }

    return 0;
}

int user_delete(const char *uid){
    fdrcp_user_t *user = fdrcp_acquire_user();
    if(user == NULL){
        log_warn("user_delete => fdrcp_acquire_user fail\n");
        return -1;
    }

    LOGINFO("user_delete : %s\n", uid);

    strncpy(user->usrid, uid,   FDRCP_USRID_MAX);

    if(fdrcp_usrdispatch(FDRCP_EVMSG_USER_DELETE, user) != 0){
        fdrcp_release_user(user);
        log_warn("user_delete => fdrcp_usrdispatch fail\n");
        return -1;
    }

    return 0;
}

int user_genfeature(char *nvdata, int nvsize, void *feature, int fsize){
    size_t len = fsize;

    LOGINFO("user_genfeature \n");

    return bisp_aie_get_fdrfeature(nvdata, 0, BISP_AIE_IMAGE_WIDTH, BISP_AIE_IMAGE_HEIGHT, (float *)feature, &len);
}

int user_genui(const char *fname, const void *nvdata, int nvsize){
    uint8_t *scale, *rgb;
    int ret;

    LOGINFO("user_genui \n");

    scale = fdrcp_malloc(nvsize);
    if(scale == NULL){
        log_warn("user_genui => alloc mem for scale fail\n");
        return -1;
    }

    ret = lvgui_yuv_nv12_scale((uint8_t *)nvdata, BISP_AIE_IMAGE_WIDTH, BISP_AIE_IMAGE_HEIGHT,
                scale, FDRCP_UIWIDTH, FDRCP_UIHEIGHT);
    if(ret != 0){
        log_warn("user_genui => scale fail\n");
        fdrcp_free(scale);
        return -1;
    }

    // alloc for rgb1555
    rgb = fdrcp_malloc(FDRCP_UIWIDTH*FDRCP_UIHEIGHT*2);
    if(rgb == NULL){
        log_warn("user_genui => alloc mem for rgb fail\n");
        fdrcp_free(scale);
        return -1;
    }

    ret = lvgui_yuv_nv12_to_rgb1555(scale, FDRCP_UIWIDTH, FDRCP_UIHEIGHT, rgb);
    fdrcp_free(scale);
    if(ret != 0){
        log_warn("user_genui => convert to rgb1555 fail\n");
        fdrcp_free(rgb);
        return -1;
    }
    
    ret = lvgui_save_to_bmp(fname, rgb, FDRCP_UIWIDTH, FDRCP_UIHEIGHT, 2);
    fdrcp_free(rgb);
    if(ret != 0){
        log_warn("user_genui => save to bmp fail\n");
        return -1;
    }

    return 0;
}

int visitor_insert(int vcid, const char *vcode, long start, long expired){
    fdrcp_vcode_t *v;

    v = fdrcp_acquire_vcode();
    if(v == NULL){
        log_warn("visitor_insert => fdrcp_acquire_vcode fail\n");
        return -1;
    }

    LOGINFO("visitor_insert %d \n", vcid);

    v->vcid = vcid;
    v->start = start;
    v->expire = expired;
    strncpy(v->vcode, vcode, FDRCP_VCODE_MAX - 1);

    fdrcp_vtcdispatch(FDRCP_EVMSG_VCODE_INSERT, v);

    return 0;
}

int visitor_delete(int vcid){
    fdrcp_vcode_t *v;

    v = fdrcp_acquire_vcode();
    if(v == NULL){
        log_warn("visitor_delete => fdrcp_acquire_vcode fail\n");
        return -1;
    }

    LOGINFO("visitor_delete %d \n", vcid);

    v->vcid = vcid;

    fdrcp_vtcdispatch(FDRCP_EVMSG_VCODE_DELETE, v);

    return 0;
}

int cpa_snapshot(const char *path, double timestamp){
    // int64_t ts = (int64_t)timestamp;
    LOGINFO("cpa_snapshot %d \n");

    // return fdrcp_snapshot(ts);
    return 0;
}

int cpa_start(){
    LOGINFO("cpa_start %d \n");
    return fdrcp_start();
}

