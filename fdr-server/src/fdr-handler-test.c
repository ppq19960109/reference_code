/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr-handler.h"
#include "fdr.h"
#include "fdr-dbs.h"
#include "fdr-logger.h"

#include "bisp_aie.h"

#include "logger.h"
#include "cJSON.h"

#include <stdlib.h>
#include <strings.h>
#include <stdio.h>

#define TEST_RESULT(retcode,message)            \
    do{                                         \
        if(retcode == 0){                       \
            log_info("PASS => %s\n", message);  \
        }else{                                  \
            log_warn("FAIL => %s\n", message);  \
            return retcode;                     \
        }                                       \
    }while(0)


typedef struct handler_test_files{
    char *input;
    char *output;
    char *tmpl;
}handler_test_files_t;

static const handler_test_files_t config_files[] = {
    {   "./test/cfg_set.json",         "./test/cfg_set_out.json",     NULL    },
    {   "./test/cfg_get.json",         "./test/cfg_get_out.json",     NULL    }
};


static const handler_test_files_t user_files[] = {
    {   "./test/user_add.json",        "./test/user_add_out.json",    NULL    },
    {   "./test/user_update.json",     "./test/user_update_out.json", NULL    },
    {   "./test/user_list.json",       "./test/user_list_out.json",   NULL    },
    {   "./test/user_info.json",       "./test/user_info_out.json",   NULL    },
    {   "./test/user_del.json",        "./test/user_del_out.json",    NULL    }
};


static const handler_test_files_t guest_files[] = {
    {   "./test/guest_insert.json",    "./test/guest_insert_out.json",NULL    },
    {   "./test/guest_delete.json",    "./test/guest_delete_out.json",NULL    }
};

static const handler_test_files_t logger_files[] = {
    {   "./test/logger_max.json",      "./test/logger_max_out.json",NULL    },
    {   "./test/logger_fetch.json",    "./test/logger_fetch_out.json",NULL    }
};

static const char *test_db_sqlite = "./test_db.sqlite";
static const char *test_feature_sqlite = "./test_feature.sqlite";
static const char *test_logger_sqlite = "./test_logger.sqlite";
static const char *default_json = "./install/default.json";

typedef  int  (*handler_pfn)(cJSON *req, cJSON *rsp);

static int loadJson(const char *jsonfile, cJSON **body){
    FILE *fj;
    size_t size;
    char *fbuf;
    cJSON *item;

    fj = fopen(jsonfile, "r");
    if(fj == NULL){
        log_warn("loadJson => load %s fail!\n", jsonfile);
        return -1;
    }

	fseek(fj, 0L, SEEK_END);
	size = ftell(fj);
	fseek(fj, 0L, SEEK_SET);

    fbuf = malloc(size);
    if(fbuf == NULL){
        log_warn("loadJson => no memory!\n");
        fclose(fj);
        return -1;
    }

    if(fread(fbuf, 1, size, fj) != size){
        log_warn("loadJson => read fail!\n");
        free(fbuf);
        fclose(fj);
        return -1;
    }
    fclose(fj);

    item = cJSON_Parse(fbuf);
    free(fbuf);

    if(item == NULL){
        log_warn("loadJson => parse json fail!\n");
        return -1;
    }

    *body = item;
    return 0;
}

static int saveJson(const char *jsonfile, cJSON *body){
    FILE *fj;
    size_t size;
    char *buf;
    int rc = -1;

    buf = cJSON_Print(body);
    if(buf == NULL){
        log_warn("saveJson => print json fail!\n");
        return -1;
    }

    fj = fopen(jsonfile, "w");
    if(fj == NULL){
        log_warn("saveJson => open %s fail!\n", jsonfile);
        return -1;
    }

    size = strlen(buf);

    if(fwrite(buf, 1, size, fj) != size){
        log_warn("saveJson => write fail!\n");
        rc = -1;
    }else{
        rc = 0;
    }

    free(buf);
    fclose(fj);

    return 0;
}


static int test_handler(handler_pfn handler, const handler_test_files_t *files, int fcount){
    const handler_test_files_t *tf;
    cJSON *req;
    cJSON *rsp;
    int rc;
    int i;


    for(i = 0; i < fcount; i++){
        tf = &files[i];

        log_info("test_handler => process %s\n", tf->input);

        rsp = cJSON_CreateObject();
        if(rsp == NULL){
            log_warn("test_handler => create rsp fail!\n");
            return -1;
        }

        rc = loadJson(tf->input, &req);
        if(rc != 0){
            log_warn("test_handler => load json fail!\n");
            cJSON_Delete(rsp);
            return -1;
        }

        rc = handler(req, rsp);
        if(rc != 0){
            log_warn("test_handler => handler fail:%d\n", rc);
            cJSON_Delete(req);
            cJSON_Delete(rsp);
            return -1;
        }

        log_info("test_handler => save to %s\n", tf->output);
        rc = saveJson(tf->output, rsp);
        if(rc != 0){
            log_warn("test_handler => save json fail!\n");
            cJSON_Delete(req);
            cJSON_Delete(rsp);
            return -1;
        }
        
        cJSON_Delete(req);
        cJSON_Delete(rsp);

        req = NULL;
        rsp = NULL;
    }

    return 0;
}


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

fdr_config_t *fdr_config(){
    static fdr_config_t config;
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
    char *val;
    int rc;
    
    // test dbs, so we got clean dbs
    remove(test_db_sqlite);
    remove(test_feature_sqlite);

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

    rc = fdr_logger_init(test_logger_sqlite);
    if(rc != 0){
        log_warn("main => fdr_logger_init fail!\n");
        return -1;
    }


    if((fdr_dbs_config_get("device-id", &val) == 0) && (val != NULL)){
        fdr_free(val);
    }else{
        load_default_settings();
    }

    fdr_loadconfig();

#if 0
    rc = test_handler(handler_config,   config_files, sizeof(config_files)/sizeof(handler_test_files_t));
    TEST_RESULT(rc, "handler_config");
#endif

#if 0
    rc = test_handler(handler_user,     user_files,   sizeof(user_files)/sizeof(handler_test_files_t));
    TEST_RESULT(rc, "handler_user");

    rc = test_handler(handler_guest,    guest_files,  sizeof(guest_files)/sizeof(handler_test_files_t));
    TEST_RESULT(rc, "handler_guest");
#endif

    rc = test_handler(handler_logger,    logger_files,  sizeof(logger_files)/sizeof(handler_test_files_t));
    TEST_RESULT(rc, "handler_logger");

    rc = fdr_logger_fini();
    if(rc != 0){
        log_warn("main => fdr_logger_fini fail!\n");
        return -1;
    }

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
