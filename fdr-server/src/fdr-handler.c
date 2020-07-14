/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr-handler.h"
#include "fdr.h"
#include "fdr-dbs.h"
#include "fdr-util.h"

#include "bisp_hal.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char *info_keys[] = {
    "manufacturer",
    "device-id",
    "product-id",
    "device-info",
    "device-mode",
    "linux-version",
    "firmware-version",
    "hardware-version",
    "activation",
    "rsync",
};

typedef struct config_update{
    char *key;      // config key
    char update;     // 0, need reboot to update; 1, update now
}config_update_t;

static config_update_t config_keys[] = {

    { "ether-ipv4-address-mode", 0 },
    { "ether-ipv4-address",     0 },
    { "ether-ipv4-mask",        0 },
    { "ether-ipv4-gw",          0 },
    { "ether-dns-server",       0 },

    { "wifi-ipv4-address-mode", 0 },
    { "wifi-ipv4-address",      0 },
    { "wifi-ipv4-mask",         0 },
    { "wifi-ipv4-gw",           0 },
    { "wifi-dns-server",        0 },

    { "ntp-server-address",     0 },

    { "manager-server-address", 1 },
    { "manager-server-port",    1 },
    { "manager-server-path",    1 },

    { "logr-item-max",          1 } ,	// 默认10万条记录
    { "user-max",               0 } ,	// 支持最大用户数 ，默认1000

    { "visitor-mode",           1 },    // 访客模式
    { "voice-enable",           1 },    // 使能音频

    { "recognize-thresh-hold",  1 },    // 人脸比对阈值
    { "controller-type",        1 },
    { "door-open-latency",      1 },    // 开门时延

    { "rs485-open-data",        1 },    // 485打开发送数据
    { "rs485-close-data",       1 },    // 485关闭发送数据
};

int handler_info(cJSON *req, cJSON *rsp){
    fdr_config_t *config = fdr_config();
    char  temp[64];
    char *val;
    int i;
    int rc;

    for(i = 0; i < sizeof(info_keys)/ sizeof(char*); i++){

        rc = fdr_dbs_config_get(info_keys[i], &val);
        if(rc != 0){
            log_warn("handler_info => get %s fail!\n", info_keys[i]);
            goto fail_return;
        }
        cJSON_AddStringToObject(rsp, info_keys[i], val);
        fdr_free(val);
    }

    memset(temp, 0, 64);
    snprintf(temp, 63, "v%d.%d.%d", config->algm_version >> 16, (config->algm_version >> 8) & 0xff, config->algm_version & 0xff);
    cJSON_AddStringToObject(rsp, "algm-version", temp);

    return 0;

fail_return:

    return FDRSDK_ERRCODE_INVALID;
}

static cJSON * new_keyval_item(void *key, void *val){
    cJSON *item;

    item = cJSON_CreateObject();
    if(item == NULL)
        return NULL;

    if(cJSON_AddStringToObject(item, key, val) == NULL){
        cJSON_Delete(item);
        return NULL;
    }

    return item;
}

char handler_insettings(config_update_t *config_keys, int count, char *key){
    int i; 

    for(i = 0; i < count; i++){
        if(strcasecmp(config_keys[i].key, key) == 0){
            return config_keys[i].update;
        }
    }

    return -1;
}

int handler_config(cJSON *req, cJSON *rsp){
    cJSON *item, *retitem, *subitem;
    cJSON *params, *retparams;
    char *key, *val;
    int i,j;
    char update;
    int rc;

    item = cJSON_GetObjectItem(req, "action");
    if(item == NULL){
        return FDRSDK_ERRCODE_INVALID;
    }

    val = cJSON_GetStringValue(item);
    if(val == NULL){
        return FDRSDK_ERRCODE_INVALID;
    }

    params = cJSON_GetObjectItem(req, "params");
    if(params == NULL){
        return FDRSDK_ERRCODE_INVALID;
    }

    retparams = cJSON_CreateArray();
    if(retparams == NULL){
        return FDRSDK_ERRCODE_NOMEM;
    }
    cJSON_AddItemToObject(rsp, "params", retparams);

    if(strcasecmp(val, "get") == 0){

        for(i = 0; i < cJSON_GetArraySize(params); i++){
            item = cJSON_GetArrayItem(params, i);
            if(item == NULL){
                return FDRSDK_ERRCODE_INVALID;
            }
            key = cJSON_GetStringValue(item);
            
            if(handler_insettings(config_keys, sizeof(config_keys)/ sizeof(config_update_t), key) >= 0){
                rc = fdr_dbs_config_get(key, &val);
                if(rc == 0){

                    retitem = new_keyval_item(key, val);
                    free(val);
                    val = NULL;

                    if(retitem != NULL){
                        cJSON_AddItemToArray(retparams, retitem);
                    }else{
                        return FDRSDK_ERRCODE_NOMEM;
                    }
                }else{
                    return FDRSDK_ERRCODE_NOEXIT;
                }
            }else{
                return FDRSDK_ERRCODE_NOEXIT;
            }
        }
    }else if(strcasecmp(val, "set") == 0){

        for(i = 0; i < cJSON_GetArraySize(params); i++){
            item = cJSON_GetArrayItem(params, i);
            if(item == NULL){
                return FDRSDK_ERRCODE_INVALID;
            }

            for(j = 0; j < sizeof(config_keys)/ sizeof(config_update_t); j++){
                key = config_keys[j].key;
                update = config_keys[j].update;
                if(cJSON_HasObjectItem(item, key)){
                    subitem = cJSON_GetObjectItem(item, key);
                    val = cJSON_GetStringValue(subitem);

                    rc = fdr_dbs_config_set(key, val);
                    if(rc == 0){
                        if(update) {
                            fdr_setconfig(key, val);
                        }
                        retitem = cJSON_CreateString(key);
                        if(retitem == NULL){
                            return FDRSDK_ERRCODE_NOMEM;
                        }
                        cJSON_AddItemToArray(retparams, retitem);
                    }
                }
            }
        }

    }else{
        return FDRSDK_ERRCODE_INVALID;
    }

    return 0;
}

int handler_restart(cJSON *req, cJSON *rsp){
    cJSON_AddNumberToObject(rsp, "retcode", 0);
    bisp_hal_restart();
    return 0;
}


int handler_guest(cJSON *req, cJSON *rsp){
    cJSON *item, *retitem, *subitem;
    cJSON *params, *retparams;
    cJSON *temp;
    char *val;
    int i;
    int rc;
    int scodeid;
    const char *userid;
    const char *gcode;
    time_t expired;

    val = cJSON_GetObjectString(req, "action");
    if(val == NULL){
        return -1;
    }

    params = cJSON_GetObjectItem(req, "params");
    if(params == NULL){
        return FDRSDK_ERRCODE_INVALID;
    }

    retparams = cJSON_CreateArray();
    if(retparams == NULL){
        return FDRSDK_ERRCODE_NOMEM;
    }
    cJSON_AddItemToObject(rsp, "params", retparams);

    if(strcasecmp(val, "add") == 0){

        for(i = 0; i < cJSON_GetArraySize(params); i++){
            item = cJSON_GetArrayItem(params, i);
            if(item == NULL){
                return FDRSDK_ERRCODE_INVALID;
            }

            temp = cJSON_GetObjectItem(item, "scodeid");
            if(temp == NULL){
                return FDRSDK_ERRCODE_INVALID;
            }
            scodeid = temp->valueint;

            userid = cJSON_GetObjectString(item, "userid");
            if(userid == NULL){
                return FDRSDK_ERRCODE_INVALID;
            }
#ifndef TEST
            rc = fdr_dbs_user_exist(userid);
            if(!rc){
                return FDRSDK_ERRCODE_NOEXIT;
            }
#endif
            gcode = cJSON_GetObjectString(item, "gcode");
            if(gcode == NULL){
                return FDRSDK_ERRCODE_INVALID;
            }

            val = cJSON_GetObjectString(item, "expired");
            if(val == NULL){
                expired = time(NULL) + 24*3600;    // defualt one day
            }else{
                expired = fdr_util_strptime(val);
                if(expired == -1){
                    expired += time(NULL) + 24*3600;    // defualt one day
                }
            }

            rc = fdr_dbs_guest_insert(scodeid, userid, gcode, expired);
            if(rc == 0){
                retitem = cJSON_CreateObject();
                if(retitem == NULL){
                    return FDRSDK_ERRCODE_NOMEM;
                }
                subitem = cJSON_AddNumberToObject(retitem, "scodeid", scodeid);
                if(subitem == NULL){
                    return FDRSDK_ERRCODE_NOMEM;
                }
                cJSON_AddItemToArray(retparams, retitem);
            }else{
                return FDRSDK_ERRCODE_DBOPS;
            }
        }
        cJSON_AddNumberToObject(rsp, "retcode", 0);
    }else if(strcasecmp(val, "del") == 0){

        for(i = 0; i < cJSON_GetArraySize(params); i++){
            item = cJSON_GetArrayItem(params, i);
            if(item == NULL){
                return FDRSDK_ERRCODE_INVALID;
            }

            temp = cJSON_GetObjectItem(item, "scodeid");
            if(temp == NULL){
                return FDRSDK_ERRCODE_INVALID;
            }
            scodeid = temp->valueint;

            rc = fdr_dbs_guest_delete(scodeid);
            if(rc == 0){
                retitem = cJSON_CreateObject();
                if(retitem == NULL){
                    return FDRSDK_ERRCODE_NOMEM;
                }
                subitem = cJSON_AddNumberToObject(retitem, "scodeid", scodeid);
                if(subitem == NULL){
                    return FDRSDK_ERRCODE_NOMEM;
                }
                cJSON_AddItemToArray(retparams, retitem);
            }else{
                return FDRSDK_ERRCODE_DBOPS;
            }
        }
        cJSON_AddNumberToObject(rsp, "retcode", 0);
    }else{
        return FDRSDK_ERRCODE_INVALID;
        cJSON_AddNumberToObject(rsp, "retcode", FDRSDK_ERRCODE_INVALID);
    }
    
    return 0;
}

