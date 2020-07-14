/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __FDRCP_UTILS_H__
#define __FDRCP_UTILS_H__

#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <stdint.h>
#include <dirent.h>

#define FDRSDK_ERRCODE_NOMEM            100
#define FDRSDK_ERRCODE_INVALID          101
#define FDRSDK_ERRCODE_DBOPS            102
#define FDRSDK_ERRCODE_NOEXIT           103
#define FDRSDK_ERRCODE_INVALID_FACE     104
#define FDRSDK_ERRCODE_TOKEN            105
#define FDRSDK_ERRCODE_OVERLOAD         106
#define FDRSDK_ERRCODE_CHECKSUM         107

#define FDRCP_DEVID_MAX                 (32 + 1)        // NULL end
#define FDRCP_RS485_DATA_MAX            128

#define FDR_SIGNATURE_MAX               ECC_BYTES       // SHA256 = secp256r1

#define  FDRCP_PATH_MAX                 1024

// version of fdr
#define FDR_VERSION                     "v2.2.1"

#ifdef XM6XX

#if defined(XM650)
#define FDR_HOME_PATH   "/var/sd/app"
#elif defined(XM630)
#define FDR_HOME_PATH   "/var/sd"
#else
#error "Please define the cpu core"
#endif

#define OPLOG_PATH              FDR_HOME_PATH"/log/"
#define CAPTURE_IMAGE_PATH      FDR_HOME_PATH"/data/images/"
#define USER_IMAGE_PATH         FDR_HOME_PATH"/data/users/"
#define UPGRADE_PACKAGE_PATH    FDR_HOME_PATH"/upgrade/"
//#define DEV_PUBKEY_FILENAME     FDR_HOME_PATH"/data/ecc/dev-pubkey.ecc"
//#define DEV_PRIKEY_FILENAME     FDR_HOME_PATH"/data/ecc/dev-prikey.ecc"
//#define SVR_PUBKEY_FILENAME     FDR_HOME_PATH"/data/ecc/svr-pubkey.ecc"
#define FEATURE_SQLITE_PATH     FDR_HOME_PATH"/data/db"
#define DBS_SQLITE_FILENAME     FDR_HOME_PATH"/data/db/dbs.sqlite"
#define LOG_SQLITE_FILENAME     FDR_HOME_PATH"/data/db/log.sqlite"
#define DEFAULT_CONFIG_FILENAME FDR_HOME_PATH"/data/db/default.json"
#define UPGRADE_CONFIG_FILENAME FDR_HOME_PATH"/data/db/upgrade.json"
#define LICENSE_FILENAME        "/mnt/mtd/device.json"

#define FDR_VOICE_PATH          FDR_HOME_PATH"/data/voice"

#if defined(UI_LANG) && (UI_LANG == 1)
#define FDR_UI_PATH             FDR_HOME_PATH"/data/ui-en"
#elif defined(UI_LANG) && (UI_LANG == 2)
#define FDR_UI_PATH             FDR_HOME_PATH"/data/ui-neutral"
#elif defined(UI_LANG) && (UI_LANG == 0)
#define FDR_UI_PATH             FDR_HOME_PATH"/data/ui"
#else
#error "Please define UI_LANG to select ui lang"
#endif

//XM6XX
#else

#define FDR_HOME_PATH           "./data/"
#define OPLOG_PATH              FDR_HOME_PATH"/log/"
#define CAPTURE_IMAGE_PATH      FDR_HOME_PATH"/logr/"
#define USER_IMAGE_PATH         FDR_HOME_PATH"/users/"
#define UPGRADE_PACKAGE_PATH    FDR_HOME_PATH"/upgrade/"
//#define DEV_PUBKEY_FILENAME     FDR_HOME_PATH"/pubkey.ecc"
//#define DEV_PRIKEY_FILENAME     FDR_HOME_PATH"/prikey.ecc"
//#define SVR_PUBKEY_FILENAME     FDR_HOME_PATH"/pubkey.ecc"
#define FEATURE_SQLITE_PATH     FDR_HOME_PATH"/"
#define DBS_SQLITE_FILENAME     FDR_HOME_PATH"/dbs.sqlite"
#define LOG_SQLITE_FILENAME     FDR_HOME_PATH"/log.sqlite"
#define DEFAULT_CONFIG_FILENAME FDR_HOME_PATH"/default.json"
#define FDR_UI_PATH             FDR_HOME_PATH"/ui"
#define LICENSE_FILENAME        FDR_HOME_PATH"/device.json"

#define FDR_VOICE_PATH          FDR_HOME_PATH"/voice"

#endif //XM6XX

enum {
    FDRCP_RS485_MODE_NONE = 0,
    FDRCP_RS485_MODE_DOOR,
    FDRCP_RS485_MODE_VOICE,
    FDRCP_RS485_MODE_TEMP,
    FDRCP_RS485_MODE_USER
};

enum {
    FDRCP_VOICE_MODE_CLOSED = 0,
    FDRCP_VOICE_MODE_DEFAULT,
    FDRCP_VOICE_MODE_USER
};

enum {
    FDRCP_DEVMODE_INACT = 0,
    FDRCP_DEVMODE_CLOUD,
    FDRCP_DEVMODE_LOCAL,
    FDRCP_DEVMODE_MIXED
};

typedef struct {
    // 设备相关配置
    char devid[FDRCP_DEVID_MAX];    // 设备ID号
    
    // 新版本，控制平面不用
    int  devmode;       // 设备激活状态, 0 - 未激活；1 - 激活,云模式；2 - 激活，本地模式； 3 - 激活，云+本地

    // 用户相关配置
    int user_max;       // 最多用户底库数

    int vcode_max;      // 最多访客数

    // 新版本，控制平面不用
    int user_logr_max;  // 最多用户识别记录数

    // 持续时间相关配置
    int visi_duration;      // 访客扫码UI持续时间, 10s
    int tips_duration;      // 提示信息UI持续时间, 02s
    int reco_duration;      // 识别用户UI持续时间, 02s
    int advt_duration;      // 广告显示UI持续时间, 10s
    int door_duration;      // 开门持续时间,      03s
    int info_duration;      // 长按使能信息页时间, 10s

    // 外设控制模式
    int visi_mode;          // 0 - 无访客；1 - 二维码； 2 - 键盘
    int rs485_mode;         // 0 - 未使用能；1 - 开门控制；2 - 语音控制；3 - 测温配件；4 - 自定义
    int voice_mode;         // 0 - 关闭语音；1 - 默认语音；2 - 自定义语音；
    int relay_mode;         // 0 - 无开关； 1 - 有开关
    int therm_mode;         // 0 - 无测温； 1 - 有测温；
    int hcode_mode;         // 0 - 无健康码； 2 - 有健康码；
    
    float therm_fix;         // 温度修正值
    float therm_threshold;   // 温度阈值：36.5

    // 算法参数
    float algm_threshold;   // 人脸比对阈值：0.6
    int   algm_version;     // 非配置项，从算法SDK获取；

    // RS485自定义数据
    char rs485_start_data[FDRCP_RS485_DATA_MAX]; // 识别成功发送数据
    char rs485_close_data[FDRCP_RS485_DATA_MAX]; // 超时关闭发送数据
}fdrcp_conf_t;

fdrcp_conf_t *fdrcp_conf_get();
int fdrcp_conf_set(const char *key, const char *val);

int64_t fdrcp_timestamp();
int fdrcp_utils_mkdir(int64_t ts);
int fdrcp_utils_logr(int64_t ts, void *frame);
int fdrcp_logr_delete(int64_t ts);

int fdrcp_utils_read_file(const char *filepath, int skip, void *buf, int *len);
int fdrcp_utils_read_user_file(const char *userid, int skip, void *buf, int *len, const char *ext);

typedef int (*fdrcp_utils_iterator)(void *handle, struct dirent *next, const char *dir);

int fdrcp_mem_init();
int fdrcp_mem_fini();
void *fdrcp_malloc(size_t size);
void  fdrcp_free(void *ptr);

#endif // __FDRCP_UTILS_H__
