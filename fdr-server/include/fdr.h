/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __FDR_H__
#define __FDR_H__

#include "ecc.h"

#include <stdlib.h>
#include <inttypes.h>
#include <time.h>

#define FDRSDK_ERRCODE_NOMEM            100
#define FDRSDK_ERRCODE_INVALID          101
#define FDRSDK_ERRCODE_DBOPS            102
#define FDRSDK_ERRCODE_NOEXIT           103
#define FDRSDK_ERRCODE_INVALID_FACE     104
#define FDRSDK_ERRCODE_TOKEN            105
#define FDRSDK_ERRCODE_OVERLOAD         106
#define FDRSDK_ERRCODE_CHECKSUM         107

#define FDR_CONFIG_RS485_BUFFER_MAX     128
#define FDR_SIGNATURE_MAX               ECC_BYTES       // SHA256 = secp256r1

#define  FDR_PATH_MAX                   1024

// version of fdr
#define FDR_VERSION                     "v2.2.1"

typedef struct {
    char *device_id;                // 设备ID号

    // GUI显示相关
    int show_tips_latency;          // 各种操作成功/失败的提示时延：2s
    int config_active_duration;     // 长按调出配置页面时间：10s

    // 状态持续时间
    int config_duration;            // 配置状态超时时间：15s
    int visitor_duration;           // 访客状态超时时间：15s
    int idle_run_interval;          // 清理数据时间间隔：3600s

    // 算法参数
    float recognize_thresh_hold;      // 人脸比对阈值：0.6
    int  algm_version;              // 非配置项，从算法SDK获取；

    int voice_enable;               // 语音控制
    int visitor_mode;               // 访客模式，0 二维码， 1  键盘
    int device_mode;                // 设备运行模式
    int activation;                 // 设备激活状态
    
    // 云端地址
    char *cloud_hostname;
    char *cloud_addr;
    unsigned short cloud_port;

    // 云端心跳时间
    int heartbeat;

    int cloud_reconnect;            // 断线重连接时间间隔
    int cloud_connected;            // 不是配置项，根据连接情况设置
    
    // used by local mode
    char *mgmt_addr;
    unsigned short mgmt_port;
    char *mgmt_path;

    // 存储配置
    int logr_item_max; 	            // 默认4万条记录，超出即删除
    int user_max;                   // 设备最大用户数，XM650为1000，DV300为10000

    // 外设控制
    int controller_type;            // 0 - 继电器， 1 - 485串口， 2 - 两者同时
    int door_open_latency;          // 开门时延: 3s
    char rs485_open_data[FDR_CONFIG_RS485_BUFFER_MAX];     // 485打开发送数据
    char rs485_close_data[FDR_CONFIG_RS485_BUFFER_MAX];    // 485关闭发送数据
}fdr_config_t;

fdr_config_t *fdr_config();
int fdr_setconfig(const char *key, const char *val);

int fdr_init();
int fdr_fini();

int fdr_get_publickey(const uint8_t **pubkey, int *klen);
int fdr_get_privatekey(const uint8_t **prikey, int *klen);

int fdr_sign(const void *bin, size_t len, time_t ts, void *checksum);
int fdr_verify(const void *bin, size_t len, time_t ts, const void *checksum);

int fdr_mem_init();
int fdr_mem_fini();

void *fdr_malloc(size_t size);
void fdr_free(void *ptr);


#endif // __FDR_H__
