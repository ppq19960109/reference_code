/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdrcp-utils.h"

#include "logger.h"

#include "bisp_aie.h"

//#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <time.h>

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/time.h>


int64_t fdrcp_timestamp(){
    int64_t r;
    struct timeval tv;

    if(gettimeofday(&tv,NULL) != 0){
        return 0;
    }

    r = tv.tv_sec;
    r = r*1000 + tv.tv_usec/1000;

    return r;
}


int fdrcp_utils_logr(int64_t ts, void *frame){
    char path[FDRCP_PATH_MAX];

    time_t tm = (time_t)(ts / 1000);
    struct tm t;

    //setenv("TZ", "GMT-8", 1);
    if(localtime_r(&tm, &t) == NULL){
        log_warn("fdrcp_utils_logr => get gmtime fail\n");
        return -1;
    }

    sprintf(path, "%s/%04d%02d%02d/%02d%02d%02d.%03lu.jpg", CAPTURE_IMAGE_PATH, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                t.tm_hour, t.tm_min, t.tm_sec, (unsigned long)(ts % 1000));

    fdrcp_utils_mkdir(ts);

    bisp_aie_writetojpeg(path, frame);
    bisp_aie_release_frame(frame);

    return 0;
}

int fdrcp_logr_delete(int64_t ts){
    char path[FDRCP_PATH_MAX];

    time_t tm = (time_t)(ts / 1000);
    struct tm t;

    //setenv("TZ", "GMT-8", 1);
    if(localtime_r(&tm, &t) == NULL){
        log_warn("fdrcp_utils_logr => get gmtime fail\n");
        return -1;
    }

    sprintf(path, "%s/%04d%02d%02d/%02d%02d%02d.%03lu.jpg", CAPTURE_IMAGE_PATH, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                t.tm_hour, t.tm_min, t.tm_sec, (unsigned long)(ts % 1000));

    fdrcp_utils_mkdir(ts);

    return remove(path);
}

int fdrcp_utils_mkdir(int64_t ts){
    char path[FDRCP_PATH_MAX];

    time_t tm = (time_t)(ts / 1000);
    struct tm t;

    //setenv("TZ", "GMT-8", 1);
    if(localtime_r(&tm, &t) == NULL){
        log_warn("fdr_util_mkdir => get gmtime fail\n");
        return -1;
    }

    sprintf(path, "%s/%04d%02d%02d", CAPTURE_IMAGE_PATH, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

    if(access(path, F_OK) != 0){
        mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);
    }

    return 0;
}

int fdrcp_utils_foreach_dir(const char *path, fdrcp_utils_iterator iterator, void *handle){
    DIR  *rdir;
    struct dirent *next;
    int rc = -1;

    rdir = opendir(path);
    if(rdir == NULL){
        log_warn("fdrcp_utils_foreach_dir => open dir %s fail!\n", path);
        return rc;
    }

    while((next = readdir(rdir)) != NULL){
        if((strcmp(next->d_name, ".") == 0) || (strcmp(next->d_name, "..") == 0)){
            continue;
        }

        rc = iterator(handle, next, path);
        if(rc != 0){
            break;
        }
    }

    closedir(rdir);

    return rc;
}

int fdrcp_utils_read_user_file(const char *userid, int skip, void *buf, int *len, const char *ext){
    char filepath[FDRCP_PATH_MAX];
    int idx = 0;
    int uidlen = strlen(userid);
    FILE *fj;
    long size;
    size_t rlen;
    int i;

    for(i = 0; i < uidlen; i++){
        idx += userid[i];
    }

    snprintf(filepath, FDRCP_PATH_MAX, "%s/%02d/%s%s", USER_IMAGE_PATH, idx %100, userid, ext);

    fj = fopen(filepath, "r");
    if(fj == NULL){
        log_warn("fdrcp_utils_readui => open %s fail!\n", filepath);
        return -1;
    }

	fseek(fj, 0L, SEEK_END);
	size = ftell(fj);
	fseek(fj, 0L, SEEK_SET);

    if(size > (*len + skip)){
        log_warn("fdrcp_utils_readui => file size overflow : %d!\n", size);
        fclose(fj);
        return -1;
    }

	fseek(fj, skip, SEEK_SET);

    rlen = fread(buf, 1, size, fj);
    fclose(fj);
    if(rlen != (size_t)size){
        log_warn("fdrcp_utils_readui => load fail %lu, %ld!\n", size, rlen);
        return -1;
    }

    *len = rlen;

    return 0;
}

int fdrcp_utils_read_file(const char *filepath, int skip, void *buf, int *len){
    FILE *fj;
    size_t rlen;
 
    fj = fopen(filepath, "r");
    if(fj == NULL){
        log_warn("fdrcp_utils_read_file => open %s fail!\n", filepath);
        return -1;
    }

	fseek(fj, skip, SEEK_SET);

    rlen = fread(buf, 1, *len, fj);
    fclose(fj);
    if(rlen < 0){
        log_warn("fdrcp_utils_read_file => load fail %ld!\n", rlen);
        return -1;
    }

    *len = (int)rlen;

    return 0;
}

fdrcp_conf_t *fdrcp_conf_get(){
    static fdrcp_conf_t cfg = {
        .devid = "DEVID_DEFAULT_0X001", // 设备ID号
        .devmode = FDRCP_DEVMODE_MIXED, // 设备激活状态, 0 - 未激活；1 - 激活,云模式；2 - 激活，本地模式； 3 - 激活，云+本地

        .user_max = 1000,       // 最多用户底库数
        .user_logr_max = 40000, // 最多用户识别记录数
        .vcode_max = 100,       // 最多访客数

        .visi_duration = 10,    // 访客扫码UI持续时间, 10s
        .tips_duration = 2,     // 提示信息UI持续时间, 02s
        .reco_duration = 2,     // 识别用户UI持续时间, 02s
        .advt_duration = 2,     // 广告显示UI持续时间, 10s
        .door_duration = 3,     // 开门持续时间,      03s
        .info_duration = 10,    // 长按使能信息页时间, 10s

        .visi_mode  = 1,
        .rs485_mode = FDRCP_RS485_MODE_NONE,
        .voice_mode = FDRCP_VOICE_MODE_DEFAULT, // 0 - 关闭语音；1 - 默认语音；2 - 自定义语音；
        .relay_mode = 1,         // 0 - 无开关； 1 - 有关系
        .therm_mode = 0,         // 0 - 无测温； 1 - 有测温；
        .hcode_mode = 0,         // 0 - 无健康码； 2 - 有健康码；
        .therm_fix  = 0,            // 温度修正值
        .therm_threshold = 36.5,    // 温度阈值：36.5
    
        .algm_threshold = 0.80,     // 人脸比对阈值：0.6
        .algm_version = 3,          // 非配置项，从算法SDK获取；
    };

    return &cfg;
}

int fdrcp_conf_set(const char *key, const char *val){
    fdrcp_conf_t *cc = fdrcp_conf_get();

    if(strcasecmp("device-mode", key) == 0){
        cc->devmode = atoi(val);
    }else if(strcasecmp("device-id", key) == 0){
        strncpy(cc->devid, val, FDRCP_DEVID_MAX - 1);
        cc->devid[FDRCP_DEVID_MAX - 1] = 0;
    }else if(strcasecmp("user-max", key) == 0){
        cc->user_max = atoi(val);
    }else if(strcasecmp("vcode-max", key) == 0){
        cc->vcode_max = atoi(val);
    }else if(strcasecmp("user-logr-max", key) == 0){
        cc->user_logr_max = atoi(val);
    }else if(strcasecmp("visitor-duration", key) == 0){
        cc->visi_duration = atoi(val);
    }else if(strcasecmp("tips-duration", key) == 0){
        cc->tips_duration = atoi(val);
    }else if(strcasecmp("recognized-duration", key) == 0){
        cc->reco_duration = atoi(val);
    }else if(strcasecmp("advertising-duration", key) == 0){
        cc->advt_duration = atoi(val);
    }else if(strcasecmp("door-duration", key) == 0){
        cc->door_duration = atoi(val);
    }else if(strcasecmp("info-duration", key) == 0){
        cc->info_duration = atoi(val);
    }else if(strcasecmp("visitor-max", key) == 0){
        cc->vcode_max = atoi(val);
    }else if(strcasecmp("visitor-mode", key) == 0){
        cc->visi_mode = atoi(val);
    }else if(strcasecmp("rs485-mode", key) == 0){
        cc->rs485_mode = atoi(val);
    }else if(strcasecmp("voice-mode", key) == 0){
        cc->voice_mode = atoi(val);
    }else if(strcasecmp("relay-mode", key) == 0){
        cc->relay_mode = atoi(val);
    }else if(strcasecmp("therm-mode", key) == 0){
        cc->therm_mode = atoi(val);
    }else if(strcasecmp("therm-fix", key) == 0){
        cc->therm_fix = atof(val);
    }else if(strcasecmp("therm-threshold", key) == 0){
        cc->therm_threshold = atof(val);
    }else if(strcasecmp("healthcode-mode", key) == 0){
        cc->hcode_mode = atoi(val);
    }else if(strcasecmp("algm-threshold", key) == 0){
        cc->algm_threshold = atof(val);
    }else if(strcasecmp("algm-version", key) == 0){
        cc->algm_version = atoi(val);
    }else if(strcasecmp("rs485-start-data", key) == 0){
        strncpy(cc->rs485_start_data, val, FDRCP_RS485_DATA_MAX);
    }else if(strcasecmp("rs485-close-data", key) == 0){
        strncpy(cc->rs485_close_data, val, FDRCP_RS485_DATA_MAX);
    }else {
        log_warn("config key %s, val %s not used by ctrller\n", key, val);
        return -1;
    }

    return 0;
}


#ifdef TEST

typedef struct {
    pthread_mutex_t lock;
    int count;
}fdrcp_mem_t;

static fdrcp_mem_t *get_mem(){
    static fdrcp_mem_t gtest;
    return &gtest;
}

int fdrcp_mem_init(){
    
    fdrcp_mem_t *tm = get_mem();

    pthread_mutex_init(&tm->lock, NULL);
    tm->count = 0;

    return 0;
}

int fdrcp_mem_fini(){
    fdrcp_mem_t *tm = get_mem();

    if(tm->count > 0){
        log_warn("fdr_mem_fini => memory leak %d\n", tm->count);
    }else if(tm->count < 0){
        log_warn("fdr_mem_fini => memory free more than alloc %d\n", tm->count);
    }else{
        log_info("fdr_mem_fini => alloc == free\n");
    }

    return 0;
}

void *fdrcp_malloc(size_t size){
    void *p;

    fdrcp_mem_t *tm = get_mem();

    pthread_mutex_lock(&tm->lock);

    p = malloc(size);
    if(p != NULL)
        tm->count ++;

    pthread_mutex_unlock(&tm->lock);

    return p;
}

void fdrcp_free(void *ptr){
    fdrcp_mem_t *tm = get_mem();
    
    pthread_mutex_lock(&tm->lock);
    
    if(ptr != NULL)
        tm->count --;

    pthread_mutex_unlock(&tm->lock);

    return free(ptr);
}

#else

int fdrcp_mem_init(){
    return 0;
}

int fdrcp_mem_fini(){
    return 0;
}

void *fdrcp_malloc(size_t size){
    return malloc(size);
}

void fdrcp_free(void *ptr){
    return free(ptr);
}

#endif
