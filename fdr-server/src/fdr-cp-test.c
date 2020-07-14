/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr-cp.h"
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

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

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

static const char *yuv_file = "./test/snap.yuv";
static const char *feature_file = "./test/feature.bin";

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

typedef struct cp_sema{
#ifdef __APPLE__
    dispatch_semaphore_t    sem;
#else
    sem_t                   sem;
#endif
}cp_sema_t;


static inline void
cp_sema_init(cp_sema_t *s, uint32_t value)
{
#ifdef __APPLE__
    dispatch_semaphore_t *sem = &s->sem;

    *sem = dispatch_semaphore_create(value);
#else
    sem_init(&s->sem, 0, value);
#endif
}

static inline void
cp_sema_wait(cp_sema_t *s)
{

#ifdef __APPLE__
    dispatch_semaphore_wait(s->sem, DISPATCH_TIME_FOREVER);
#else
    int r;

    do {
            r = sem_wait(&s->sem);
    } while (r == -1 && errno == EINTR);
#endif
}

static inline void
cp_sema_post(cp_sema_t *s)
{

#ifdef __APPLE__
    dispatch_semaphore_signal(s->sem);
#else
    sem_post(&s->sem);
#endif
}

/*
 *
 */
typedef struct test_cp{
    cp_sema_t  lock;

    int userid;

    void *handle;

    size_t yuv_len;
    void  *yuv;

    size_t feature_len;
    void *feature;
}test_cp_t;

static test_cp_t *get_cp(){
    static test_cp_t cp = {};
    return &cp;
}

static void *load_file(const char *filename, size_t *flen){
    FILE *fj;
    size_t size;
    char *fbuf;

    fj = fopen(filename, "r");
    if(fj == NULL){
        log_warn("load_file => open fail!\n");
        return NULL;
    }

	fseek(fj, 0L, SEEK_END);
	size = ftell(fj);
	fseek(fj, 0L, SEEK_SET);

    fbuf = fdr_malloc(size);
    if(fbuf == NULL){
        log_warn("load_file => no memory\n");
        fclose(fj);
        return NULL;
    }

    if(fread(fbuf, 1, size, fj) != size){
        log_warn("load_file => read fail!\n");
        fdr_free(fbuf);
        fclose(fj);
        return NULL;
    }
    fclose(fj);

    *flen = size;

    return fbuf;
}

static int test_cp_init(){
    test_cp_t *tcp = get_cp();

    tcp->yuv = load_file(yuv_file, &tcp->yuv_len);
    if(tcp->yuv == NULL){
        log_warn("test_cp_init => load yuv fail!\n");
        return -1;
    }

    tcp->feature = load_file(feature_file, &tcp->feature_len);
    if(tcp->feature == NULL){
        log_warn("test_cp_init => load feature fail!\n");
        fdr_free(tcp->yuv);
        return -1;
    }

    cp_sema_init(&tcp->lock, 1);

    return 0;
}

static int test_cp_fini(){
    test_cp_t *tcp = get_cp();

    fdr_free(tcp->yuv);
    fdr_free(tcp->feature);

    return 0;
}



static inline int myrandom(uint32_t *seed) {
    int res;

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
    FILE *fp = fopen("/dev/urandom", "rb");
    if(!fp) {
        return -1;
    }

    res = fread(seed, 1, sizeof(uint32_t), fp);
    fclose(fp);

    if (res != (sizeof(uint32_t))) {
        return -1;
    }

#elif defined(_WIN32)
    HCRYPTPROV hCryptProv;
    res = CryptAcquireContext(
        &hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    if (!res) {
        return -1;
    }

    res = CryptGenRandom(hCryptProv, (DWORD) sizeof(uint32_t), (PBYTE) seed);
    CryptReleaseContext(hCryptProv, 0);
    if (!res) {
        return -1;
    }

#else
    #error "unsupported platform"
#endif

    return 0;
}

#define USERFACE_MAX            6
#define USERREPO_MAX            1000
#define USERFEATURE_LEN         2048

static int test_cp_user(int count){
    test_cp_t *tcp = get_cp();
    uint32_t face_num = 0;
    uint32_t face_idx = 0;
    uint32_t i;
    int rc;
    bisp_aie_frval_t frv[USERFACE_MAX];


    do{
        myrandom(&face_num);
        face_num = face_num % USERFACE_MAX;
        if(face_num == 0){
            face_num  = 1;
        }

        for(i = 0; i < face_num; i++){
            myrandom(&face_idx);

            face_idx = face_idx % (USERREPO_MAX - 1);
            if((face_idx*USERFEATURE_LEN) > tcp->feature_len){
                log_warn("test_cp_user => feature file to small\n");
                return -1;
            }

            //log_warn("test_cp_user => feature index %d\n", face_idx);
            memcpy(frv[i].feature.vector, tcp->feature + face_idx*USERFEATURE_LEN, USERFEATURE_LEN);

            //log_warn("test_cp_user => feature %f %f %f\n", frv[i].feature.vector[0], frv[i].feature.vector[1], frv[i].feature.vector[2]);


            if((tcp->userid % 3)  != 0)
                frv[i].id = tcp->userid;

            tcp->userid ++;

            frv[i].rect.x0 = 120*i;
            frv[i].rect.y0 = 120*i;
            frv[i].rect.x1 = 120*i + 120;
            frv[i].rect.y1 = 120*i + 120;
        }
        
        cp_sema_wait(&tcp->lock);

        rc = fdr_cp_fdrdispatch(0, NULL, face_num, frv, tcp->yuv);
        if(rc != 0){
            log_warn("test_cp_user => fdr_cp_fdrdispatch fail!\n");
            return -1;
        }

        //printf("test_cp_user => fdr_cp_fdrdispatch ok!\n");

    }while((--count) > 0);

    return 0;
}

int main(){
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

    rc = fdr_cp_init();
    if(rc != 0){
        log_warn("main => fdr_cp_create fail!\n");
        return -1;
    }

    test_cp_init();

    rc = test_cp_user(10);

    test_cp_fini();

    rc = fdr_cp_fini();
    if(rc != 0){
        log_warn("main => fdr_cp_destroy fail!\n");
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

void bisp_aie_release_frame(void *frame){
    test_cp_t *tcp = get_cp();
    uint32_t rdm;

    myrandom(&rdm);

    rdm = rdm %500;

    usleep((500 + rdm) * 1000);
    
    printf("bisp_aie_release_frame => post sema \n");

    cp_sema_post(&tcp->lock);
}

void fdr_mp_report_logger(int64_t octime, int64_t seqnum, float score, char *userid){

}
