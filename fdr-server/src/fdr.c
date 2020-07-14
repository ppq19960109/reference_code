/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr.h"

#include "fdr-cp.h"
#include "fdr-mp.h"
#include "fdr-gui.h"

#include "fdr-logger.h"
#include "logger.h"
#include "fdr-discover.h"

#include "cJSON.h"
#include "ecc.h"
#include "sha256.h"

#include "fdr-path.h"
#include "fdr-network.h"
#include "fdr-version.h"
#include "fdr-license.h"

#include <event2/thread.h> 

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

static const char *feature_sqlite = FEATURE_SQLITE_PATH"/feature-v%d.%d.%d.sqlite";
static const char *dbs_sqlite = DBS_SQLITE_FILENAME;
static const char *log_sqlite = LOG_SQLITE_FILENAME;
static const char *default_json = DEFAULT_CONFIG_FILENAME;

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

    "heartbeat-duration",       // 心跳时间间隔，单位秒，默认:5s
    "cloud-reconnect-duration", // 重连接云服务时间

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

typedef struct fdr{
    fdr_config_t config;
    uint8_t ecc_pubkey[ECC_BYTES+1];
    uint8_t ecc_prikey[ECC_BYTES];
}fdr_t;

static fdr_t *fdr_get(){
    static fdr_t singleton;
    return &singleton;
}

#ifndef TEST
fdr_config_t *fdr_config(){
    return &fdr_get()->config;
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
    }else if(strcasecmp("heartbeat-duration", key) == 0){
        cc->heartbeat = atoi(val);
    }else if(strcasecmp("cloud-reconnect-duration", key) == 0){
        cc->cloud_reconnect = atoi(val);
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

#endif

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

#ifdef XM650
static int load_keys(){
    fdr_t *f = fdr_get();

    memcpy(f->ecc_pubkey, fdr_get_dev_pubkey(), ECC_BYTES+1);
    memcpy(f->ecc_prikey, fdr_get_dev_prikey(), ECC_BYTES);

    return 0;
}

#else
static int load_key_file(const char *keypath, void *buf, int len){
    FILE *f;
    int sz;

    f = fopen(keypath, "r");
    if(f == NULL){
        return -1;
    }

    sz = fread(buf, 1, len, f);
    fclose(f);
    if(sz == len){
        return 0;
    }

    return -1;
}

static const char *pubkey = "./pubkey.ecc";
static const char *prikey = "./prikey.ecc";
//#define DEV_PUBKEY_FILENAME     FDR_HOME_PATH"/pubkey.ecc"
//#define DEV_PRIKEY_FILENAME     FDR_HOME_PATH"/prikey.ecc"
//#define SVR_PUBKEY_FILENAME     FDR_HOME_PATH"/pubkey.ecc"
static int load_keys(){
    fdr_t *f = fdr_get();
    int len; 
    int rc;

    len = ECC_BYTES + 1;
    rc = load_key_file(pubkey, f->ecc_pubkey, len);
    if(rc != 0){
        log_warn("load_keys => load pubkey fail\n");
        return -1;
    }

    len = ECC_BYTES;
    rc = load_key_file(prikey, f->ecc_prikey, len);
    if(rc != 0){
        log_warn("load_keys => load prikey fail\n");
        return -1;
    }

    log_info("load_keys => load pub & pri key ok\n");

    return 0;
}
#endif

static int init_supports(){
    char path[1024];
    char *val = NULL;
    int version;
    int rc;

    // init debug logger
    logger_init();

    fdr_mem_init();

    // init config/guest/user  db
    rc = fdr_dbs_init(dbs_sqlite);
    if(rc != 0){
        log_warn("init_supports => init db fail!\n");
        return -1;
    }

    if((fdr_dbs_config_get("device-id", &val) == 0) && (val != NULL)){
        if(strcmp(val, (char *)fdr_get_device_id())) {
            fdr_dbs_config_set("device-id", (char *)fdr_get_device_id());
        }
        fdr_free(val);
    }else{
        load_default_settings();
        fdr_dbs_config_set("device-id", (char *)fdr_get_device_id());
    }

    fdr_loadconfig();

    // init feature db
    version = bisp_aie_version();
    snprintf(path, 1023, feature_sqlite, version >> 16, (version >> 8) & 0xFF, version & 0xFF);
    rc = fdr_dbs_feature_init(path);
    if(rc != 0){
        log_warn("init_supports => init feature db fail!\n");
        fdr_dbs_fini();
        return -1;
    }

    // init logger db
    rc = fdr_logger_init(log_sqlite);
    if(rc != 0){
        log_warn("init_supports => init logger fail!\n");
        fdr_dbs_fini();
        fdr_dbs_feature_fini();
        return -1;
    }

    // load ecc key
    rc = load_keys();
    if(rc != 0){
        log_warn("init_supports => load_keys fail!\n");
        fdr_dbs_fini();
        fdr_dbs_feature_fini();
        fdr_logger_fini();
        return -1;
    }

    // make libevent surpport threads
    log_info("init_supports => let event library use pthreads !\n");
    evthread_use_pthreads();

    return rc;
}

int fdr_init(){
    fdr_discover_t *fd;
    int rc;

    // create dbs/logger ...
    rc = init_supports();
    if(rc != 0){
        log_warn("main => init supports fail:%d\n", rc);
        return -1;
    }
    fdr_productid_init();
    fdr_version_init();

    // control plane init
    rc = fdr_cp_init();
    if(rc != 0){
        log_warn("main => create controller plane fail!\n");
        return -1;
    }
    log_info("main => create controller plane ok!\r\n");
    // set callback, update repo when they updated
    // fdr_dbs_user_setcbfn(fdr_ctrller_usrdispatch, cp);

    // management plane  init
    rc = fdr_mp_init();
    if(rc != 0){
        log_warn("main => create management plane fail!\n");
        return -1;
    }
    fdr_discover_init(&fd);
    log_info("main => create management plane ok!\r\n");

    
    // set callback, report recognization result to management plane
    // fdr_cp_set_report(cp, fdr_server_report_logger, mp);
    
    return 0;
}

int fdr_fini(){
    fdr_mem_fini();
    return 0;
}


int fdr_get_publickey(const uint8_t **pubkey, int *klen){
    fdr_t *f = fdr_get();

    *pubkey = f->ecc_pubkey;
    *klen  = ECC_BYTES+1;

    return 0;
}

int fdr_get_privatekey(const uint8_t **prikey, int *klen){
    fdr_t *f = fdr_get();

    *prikey = f->ecc_prikey;
    *klen  = ECC_BYTES;

    return 0;
}

int fdr_sign(const void *bin, size_t len, time_t ts, void *checksum){
    fdr_t *f = fdr_get();
    uint32_t ts32 = (uint32_t)ts;
    uint8_t shacheck[FDR_SIGNATURE_MAX];
    sha256_context ctx;
    int rc;

    ts32 = htonl(ts32);

    sha256_init(&ctx);
    sha256_hash(&ctx, (uint8_t*)bin, len);
    sha256_hash(&ctx, (uint8_t*)&ts32, sizeof(uint32_t));
    sha256_done(&ctx, shacheck);

    rc = ecdsa_sign(f->ecc_prikey, shacheck, checksum);
    if(rc == 1){
        return 0;
    }else{
        log_warn("fdr_sign => sign fail : %d\n", rc);
        return -1;
    }
}

int fdr_verify(const void *bin, size_t len, time_t ts, const void *checksum){
    fdr_t *f = fdr_get();
    uint32_t ts32 = (uint32_t)ts;
    uint8_t shacheck[FDR_SIGNATURE_MAX];
    sha256_context ctx;

    ts32 = htonl(ts32);

    sha256_init(&ctx);
    sha256_hash(&ctx, (uint8_t*)bin, len);
    sha256_hash(&ctx, (uint8_t*)&ts32, sizeof(uint32_t));
    sha256_done(&ctx, shacheck);
    int rc;

    rc = ecdsa_verify(f->ecc_pubkey, shacheck, checksum);
    if(rc == 1){
        return 0;
    }else{
        log_warn("fdr_verify => verify fail : %d\n", rc);
        return -1;
    }
}

#ifdef TEST

typedef struct fdr_mem_test{
    pthread_mutex_t lock;
    int count;
}fdr_mem_test_t;

static fdr_mem_test_t gtest;
static fdr_mem_test_t *get_mem(){
    return &gtest;
}

int fdr_mem_init(){
    
    fdr_mem_test_t *tm = get_mem();

    pthread_mutex_init(&tm->lock, NULL);
    tm->count = 0;

    return 0;
}

int fdr_mem_fini(){
    fdr_mem_test_t *tm = get_mem();

    if(tm->count > 0){
        log_warn("fdr_mem_fini => memory leak %d\n", tm->count);
    }else if(tm->count < 0){
        log_warn("fdr_mem_fini => memory free more than alloc %d\n", tm->count);
    }else{
        log_info("fdr_mem_fini => alloc == free\n");
    }

    return 0;
}

void *fdr_malloc(size_t size){
    void *p;

    fdr_mem_test_t *tm = get_mem();

    pthread_mutex_lock(&tm->lock);

    p = malloc(size);
    if(p != NULL)
        tm->count ++;

    pthread_mutex_unlock(&tm->lock);

    return p;
}

void fdr_free(void *ptr){
    fdr_mem_test_t *tm = get_mem();
    
    pthread_mutex_lock(&tm->lock);
    
    if(ptr != NULL)
        tm->count --;

    pthread_mutex_unlock(&tm->lock);

    return free(ptr);
}

#else

int fdr_mem_init(){
    return 0;
}

int fdr_mem_fini(){
    return 0;
}

void *fdr_malloc(size_t size){
    return malloc(size);
}

void fdr_free(void *ptr){
    return free(ptr);
}

#endif
