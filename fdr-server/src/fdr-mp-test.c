/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr-mp.h"
#include "fdr.h"

#include "fdr-logger.h"

#include "logger.h"
#include "cJSON.h"

#include <event2/thread.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event_struct.h>

#include <time.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <math.h>
#include <sys/stat.h>
#include <unistd.h>

// configurations
static char* config_keys[] = {
    "activation",               // 激活状态，0 - 未激活， 1 - 激活；
    "device-mode",              // 设备模式， 0 - 本地模式， 1 - 云端模式
    "device-id",                // 设备ID号，全局唯一
    //"cloud-connected",          // 非配置项，动态设备
    //"algm-version",             // 非配置项，从算法SDK获取
    
    "cloud-server-address",     // 云端服务器地址
    "cloud-server-port",        // 云端服务器端口号
    "manager-server-address",   // 本地服务器地址，用于本地模式时上报数据
    "manager-server-port",      // 本地服务器端口，用于本地模式时上报数据
    "manager-server-path",      // 本地服务器路径，用于本地模式时上报数据

    // GUI显示相关
    "show-tips-latency",        // 各种操作成功/失败的提示时延：2s
    "config-active-duration",   // 长按调出配置页面时间：10s

    // 状态持续时间
    "config-duration",          // 配置状态超时时间：15s
    "visitor-duration",         // 访客状态超时时间：15s
    "idle-run-interval",        // 清理活动时间间隔：3600s

    // 算法参数
    "recognize-thresh-hold",    // 人脸比对阈值：0.6

    // 内部控制
    "voice-enable",             // 0 - 关闭提示音， 1 - 打开提示音
    "visitor-mode",             // 0 - qrcode, 1 - keyboard

    // 外设控制
    "controller-type",          // 0 - 继电器， 1 - 485串口， 2 - 两者同时
    "door-open-latency",        // 开门时延: 3s
    "rs485-open-data",          // 485打开发送数据
    "rs485-close-data",         // 485关闭发送数据

    // 存储配置
    "logr-item-max",	        // 默认4万条记录，超出即删除
    "user-max",                 // 最大用户数，XM650-1,000; DV300-10,000; X2600-50,000;
};


static const char *test_db_sqlite = "./test_db.sqlite";
static const char *test_feature_sqlite = "./test_feature.sqlite";
static const char *default_json = "./install/default.json";
static const char *log_sqlite = "./test_logger.sqlite";

fdr_config_t *fdr_config(){
    static fdr_config_t config = {};
    return &config;
}

int fdr_setconfig(const char *key, const char *val){
    fdr_config_t *cc = fdr_config();

    if(strcasecmp("activation", key) == 0){
        cc->activation = atoi(val);
    }else if(strcasecmp("device-mode", key) == 0){
        cc->device_mode = atoi(val);
    }else if(strcasecmp("device-id", key) == 0){
        cc->device_id = strdup(val);
    }else if(strcasecmp("cloud-server-address", key) == 0){
        cc->cloud_addr = strdup(val);
    }else if(strcasecmp("cloud-server-port", key) == 0){
        cc->cloud_port = (unsigned short)atoi(val);
    }else if(strcasecmp("manager-server-address", key) == 0){
        cc->mgmt_addr = strdup(val);
    }else if(strcasecmp("manager-server-port", key) == 0){
        cc->mgmt_port = (unsigned short)atoi(val);
    }else if(strcasecmp("manager-server-path", key) == 0){
        cc->mgmt_path = strdup(val);
    }else if(strcasecmp("show-tips-latency", key) == 0){
        cc->show_tips_latency = atoi(val);
    }else if(strcasecmp("config-active-duration", key) == 0){
        cc->config_active_duration = atoi(val);
    }else if(strcasecmp("config-duration", key) == 0){
        cc->config_duration = atoi(val);
    }else if(strcasecmp("visitor-duration", key) == 0){
        cc->visitor_duration = atoi(val);
    }else if(strcasecmp("idle-run-interval", key) == 0){
        cc->idle_run_interval = atoi(val);
    }else if(strcasecmp("recognize-thresh-hold", key) == 0){
        cc->recognize_thresh_hold = (float)atoi(val);
        cc->recognize_thresh_hold = cc->recognize_thresh_hold / 100;
    }else if(strcasecmp("controller-type", key) == 0){
        cc->controller_type = atoi(val);
    }else if(strcasecmp("voice-enable", key) == 0){
        cc->voice_enable = atoi(val);
    }else if(strcasecmp("visitor-mode", key) == 0){
        cc->visitor_mode = atoi(val);
    }else if(strcasecmp("door-open-latency", key) == 0){
        cc->door_open_latency = atoi(val);
    }else if(strcasecmp("rs485-open-data", key) == 0){
        strncpy(cc->rs485_open_data, val, FDR_CONFIG_RS485_BUFFER_MAX);
    }else if(strcasecmp("rs485-close-data", key) == 0){
        strncpy(cc->rs485_close_data, val, FDR_CONFIG_RS485_BUFFER_MAX);
    }else if(strcasecmp("logr-item-max", key) == 0){
        cc->logr_item_max = atoi(val);
    }else if(strcasecmp("user-max", key) == 0){
        cc->user_max = atoi(val);
    }else {
        log_warn("config key %s, val %s not used by ctrller\n", key, val);
        return -1;
    }

    return 0;
}

static void fdr_loadconfig(){
    fdr_config_t *config = fdr_config();
    char *key, *val;
    int i;
    int rc;

    memset(config, 0, sizeof(fdr_config_t));

    for(i = 0; i < sizeof(config_keys) / sizeof(config_keys[0]); i++){
        rc = fdr_dbs_config_get(config_keys[i], &val);
        key = config_keys[i];
        if(rc == 0){
            log_info("fdr_loadconfig => load key:%s val:%s\n", config_keys[i], val);
            fdr_setconfig(key, val);
            fdr_free(val);
        }else{
            log_info("fdr_loadconfig => load key:%s fail\n", config_keys[i]);
        }
    }

    config->algm_version = bisp_aie_version();
}


static int load_default_settings(){
    FILE *fj;
    long size;
    char *fbuf;
    cJSON *settings;
    cJSON *item;

    fj = fopen(default_json, "r");
    if(fj == NULL){
        log_warn("load default settings fail!\n");
        return -1;
    }

	fseek(fj, 0L, SEEK_END);
	size = ftell(fj);
	fseek(fj, 0L, SEEK_SET);

    fbuf = fdr_malloc(size);
    if(fbuf == NULL){
        log_warn("no memory for load default settings!\n");
        fclose(fj);
        return -1;
    }

    if(fread(fbuf, 1, size, fj) != size){
        log_warn("read load default settings fail!\n");
        fdr_free(fbuf);
        fclose(fj);
        return -1;
    }
    fclose(fj);

    settings = cJSON_Parse(fbuf);
    fdr_free(fbuf);

    if(settings == NULL){
        log_warn("parse default settings fail!\n");
        return -1;
    }

    cJSON_ArrayForEach(item, settings){
        fdr_dbs_config_set(item->string, item->valuestring);
        log_info("set default config => %s : %s\n", item->string, item->valuestring);
    }

    cJSON_free(settings);

    return 0;
}

int main(){
    int count = 100;
    char *val;
    int rc;
    
    // test dbs, so we got clean dbs
    //remove(test_db_sqlite);
    //remove(test_feature_sqlite);
    remove(log_sqlite);

    logger_init();
    fdr_mem_init();

    rc = fdr_dbs_init(test_db_sqlite);
    if(rc != 0){
        log_warn("main =>fdr_dbs_init fail!\n");
        return -1;
    }

    rc = fdr_dbs_feature_init(test_feature_sqlite);
    if(rc != 0){
        log_warn("main => fdr_dbs_feature_init fail!\n");
        return -1;
    }

    if((fdr_dbs_config_get("device-id", &val) == 0) && (val != NULL)){
        fdr_free(val);
    }else{
        load_default_settings();
    }

    fdr_loadconfig();

    // init logger db
    rc = fdr_logger_init(log_sqlite);
    if(rc != 0){
        log_warn("main => fdr_logger_init fail!\n");
        return -1;
    }

    // make libevent surpport threads
    log_info("init_supports => let event library use pthreads !\n");
    evthread_use_pthreads();

    rc = fdr_mp_init();
    if(rc != 0){
        log_warn("main => fdr_mp_create fail!\n");
        return -1;
    }

    while(count--) sleep(1);


    rc = fdr_mp_fini();
    if(rc != 0){
        log_warn("main => fdr_mp_destroy fail!\n");
        return -1;
    }

    // wait cp exit, for fdr_free happy
    sleep(1);
    
    rc =fdr_dbs_fini();
    if(rc != 0){
        log_warn("main => fdr_dbs_fini fail!\n");
        return -1;
    }
    
    rc = fdr_dbs_feature_fini();
    if(rc != 0){
        log_warn("main => fdr_dbs_feature_fini fail!\n");
        return -1;
    }

    fdr_mem_fini();
    
    return 0;
}


int fdr_cp_usrdispatch(int type, const char *userid){
    return 0;
}


int fdr_cp_init(){
    return 0;
}
