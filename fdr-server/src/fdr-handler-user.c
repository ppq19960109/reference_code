/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr-handler.h"
#include "fdr-cp.h"
#include "fdr.h"

#include "bisp_aie.h"

#include "fdr-mp.h"
#include "fdr-dbs.h"
#include "fdr-util.h"

#include "logger.h"
#include "base58.h"

#include "cJSON.h"

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

typedef cJSON_bool (*cJSON_ItemTypeCheck)(const cJSON * const item);
typedef struct user_param {
    char *key;
    cJSON_ItemTypeCheck check;
}user_param_t;

static user_param_t add_keys[] = {
    {   "userid",   cJSON_IsString  },
    {   "name",     cJSON_IsString  },
    {   "desc",     cJSON_IsString  },
//    {   "others",   cJSON_IsString  },
    {   "perm",     cJSON_IsNumber  },
    {   "expire",   cJSON_IsString  }
};

static user_param_t del_keys[] = {
    {   "userid",   cJSON_IsString  }
};

int user_check_param(cJSON *param, int num, user_param_t *keys){
    cJSON *item;
    int i;
     
    for(i = 0; i < num; i++){
        item = cJSON_GetObjectItem(param, keys[i].key);
        if(item == NULL)
            return FDRSDK_ERRCODE_INVALID;

        if(! keys[i].check(item))
            return FDRSDK_ERRCODE_INVALID;
    }

    return 0;
}

static int add_string_to_array(cJSON *ary, const char *str){
    cJSON *cStr = cJSON_CreateString(str);

    if(cStr == NULL){
        log_warn("add_string_to_array  fail\n");
        return -1;
    }

    cJSON_AddItemToArray(ary, cStr);

    return 0;
}


static int handler_user_add(cJSON *params, cJSON *rsp){
    fdr_dbs_user_t  user;
    cJSON *uj, *item, *rsp_params;
    char *b58str;
    void *feature;
    size_t flen;
    int rc;
    int i;

    rsp_params = cJSON_CreateArray();
    if(rsp_params != NULL){
        cJSON_AddItemToObject(rsp, "params", rsp_params);
    }else{
        return FDRSDK_ERRCODE_NOMEM;
    }

    for(i = 0; i < cJSON_GetArraySize(params); i++){
        uj = cJSON_GetArrayItem(params, i);
        if(uj == NULL){
            log_warn("user add get param fail:%d\n", i);
            return FDRSDK_ERRCODE_INVALID;
        }

        if(user_check_param(uj, sizeof(add_keys)/sizeof(user_param_t), add_keys) != 0){
            log_warn("user add check param fail:%d\n", i);
            return FDRSDK_ERRCODE_INVALID;
        }

        memset(&user, 0, sizeof(user));

        user.userid = cJSON_GetObjectString(uj, "userid");
        if((user.userid == NULL) || (strlen(user.userid) > FDR_DBS_USERID_SIZE)){
            log_warn("user add userid too long : %d\n", i);
            return FDRSDK_ERRCODE_INVALID;
        }

        user.name   = cJSON_GetObjectString(uj, "name");
        if((user.name == NULL) || (strlen(user.name) > FDR_DBS_USERNAME_SIZE)){
            log_warn("user add name too long : %d, %d, %s\n", i, strlen(user.name), user.name);
            return FDRSDK_ERRCODE_INVALID;
        }

        user.desc   = cJSON_GetObjectString(uj, "desc");
        if((user.desc == NULL) || (strlen(user.desc) > FDR_DBS_USERDESC_SIZE)){
            log_warn("user add desc too long : %d\n", i);
            return FDRSDK_ERRCODE_INVALID;
        }

        user.others  = cJSON_GetObjectString(uj,  "others");
        if((user.others != NULL) && (strlen(user.others) > FDR_DBS_USEROTHERS_SIZE)){
            log_warn("user add others too long : %d\n", i);
            return FDRSDK_ERRCODE_INVALID;
        }

        item  = cJSON_GetObjectItem(uj, "perm");
        if(item == NULL){
            user.perm = 1;
        }else{
            user.perm   = cJSON_GetObjectItem(uj, "perm")->valueint;
        }
        
        user.status = 0;

        b58str = cJSON_GetObjectString(uj, "feature");
        if(b58str != NULL){

            flen = BISP_AIE_FDRVECTOR_MAX * sizeof(float);
            feature = (void*)fdr_malloc(flen);

            rc = base58_decode(b58str, strlen(b58str), feature, flen);
            if(rc){
                user.feature_len = flen;
                user.feature = feature;
                user.status = 1;
            }else{
                user.feature_len = 0;
                user.feature = NULL;
                log_warn("handler_user_add => feature b58 to bin fail %d\n", rc);
            }
        }else{
            user.feature_len = 0;
            user.feature = NULL;
        }

        item = cJSON_GetObjectItem(uj, "expire");
        if(item == NULL){
            // default 1 year expired
            user.exptime = time(NULL) + 365*24*3600;
        }else{
            user.exptime = fdr_util_strptime(cJSON_GetStringValue(item));
            if(user.exptime == -1){
                log_warn("user add expire time convert fail:%s\n", cJSON_GetStringValue(item));
                return FDRSDK_ERRCODE_INVALID;
            }
        }

        //log_info("call adduser %s type %d\n", user.userid, user.type);
        rc = fdr_dbs_user_insert(&user);
        if(rc != 0){
            log_warn("user add fail:%d\n", rc);
            if(user.feature  != NULL){
                fdr_free(user.feature);
                user.feature = NULL;
                user.feature_len = 0;
            }
            return (rc == -1) ? FDRSDK_ERRCODE_DBOPS : FDRSDK_ERRCODE_OVERLOAD;
        }

        if(user.feature != NULL){
            rc = fdr_dbs_feature_insert(user.userid, user.feature, user.feature_len);
            fdr_free(user.feature);
            user.feature = NULL;
            user.feature_len = 0;

            if(rc != 0){
                log_warn("user add feature fail:%d\n", rc);
                return FDRSDK_ERRCODE_DBOPS;
            }
        }

        fdr_cp_usrdispatch(FDR_CP_EVT_USER_UPDATE, user.userid);

        add_string_to_array(rsp_params, user.userid);
    }

    return 0;
}

static int handler_user_del(cJSON *params, cJSON *rsp){
    cJSON *uj, *rsp_params;
    char *userid;
    int i;

    rsp_params = cJSON_CreateArray();
    if(rsp_params != NULL){
        cJSON_AddItemToObject(rsp, "params", rsp_params);
    }else{
        return FDRSDK_ERRCODE_NOMEM;
    }

    for(i = 0; i < cJSON_GetArraySize(params); i++){
        uj = cJSON_GetArrayItem(params, i);
        if(uj == NULL){
            log_warn("user del get param fail:%d\n", i);
            return FDRSDK_ERRCODE_INVALID;
        }

        if(user_check_param(uj, sizeof(del_keys)/sizeof(user_param_t), del_keys) != 0){
            log_warn("user del check param fail:%d\n", i);
            return FDRSDK_ERRCODE_INVALID;
        }

        userid = cJSON_GetObjectString(uj, "userid");
        
        // log_info("user del call db : %s\n", userid);
        if((userid != NULL) && (fdr_dbs_user_delete(userid) == 0)){

            fdr_dbs_feature_delete(userid);

            add_string_to_array(rsp_params, userid);
        }
        
        fdr_cp_usrdispatch(FDR_CP_EVT_USER_DELETE, userid);
    }
    return 0;
}

static int handler_user_update(cJSON *params, cJSON *rsp){
    fdr_dbs_user_t  user;
    cJSON *uj;
    cJSON *item;
    cJSON *rsp_params;
    char *b58str;
    void *feature;
    size_t flen;
    int rc;
    int i;

    rsp_params = cJSON_CreateArray();
    if(rsp_params != NULL){
        cJSON_AddItemToObject(rsp, "params", rsp_params);
    }else{
        return FDRSDK_ERRCODE_NOMEM;
    }

    for(i = 0; i < cJSON_GetArraySize(params); i++){
        uj = cJSON_GetArrayItem(params, i);
        if(uj == NULL){
            log_warn("user update get param fail:%d\n", i);
            return FDRSDK_ERRCODE_INVALID;
        }

        memset(&user, 0, sizeof(user));

        user.userid = cJSON_GetObjectString(uj, "userid");
        if(user.userid == NULL){
            log_warn("user update  param should have userid\n");
            return FDRSDK_ERRCODE_INVALID;
        }

        user.name    = cJSON_GetObjectString(uj, "name");
        user.desc    = cJSON_GetObjectString(uj, "desc");
        user.others  = cJSON_GetObjectString(uj, "phone");

        item = cJSON_GetObjectItem(uj, "expire");
        if(item != NULL){
            user.exptime = fdr_util_strptime(cJSON_GetStringValue(item));
            if(user.exptime == -1){
                log_warn("user add expire time convert fail: %s\n", cJSON_GetStringValue(item));
                return FDRSDK_ERRCODE_INVALID;
            }
        }
        
        item = cJSON_GetObjectItem(uj, "perm");
        if((item != NULL) && cJSON_IsNumber(item)){
            user.perm = (int)item->valueint;
        }else{
            user.perm = -1;
        }

        b58str = cJSON_GetObjectString(uj, "feature");
        if(b58str != NULL){

            flen = BISP_AIE_FDRVECTOR_MAX * sizeof(float);
            feature = (void*)fdr_malloc(flen);

            rc = base58_decode(b58str, strlen(b58str), feature, flen);
        
            if(rc > 0){
                user.feature_len = flen;
                user.feature = feature;
            }else{
                user.feature_len = 0;
                user.feature = NULL;
                log_warn("handler_user_add => feature b58 to bin fail\n");
            }
        }else{
            user.feature_len = 0;
            user.feature = NULL;
        }

        rc = fdr_dbs_user_update(user.userid, &user);
        if(rc != 0){
            log_warn("user update fail:%d\n", rc);
            if(user.feature != NULL){
                fdr_free(user.feature);
                user.feature = NULL;
                user.feature_len = 0;
            }

            return FDRSDK_ERRCODE_DBOPS;
        }
        
        if(user.feature != NULL){
            rc = fdr_dbs_feature_update(user.userid, user.feature, user.feature_len);
            fdr_free(user.feature);
            user.feature = NULL;
            user.feature_len = 0;

            if(rc != 0){
                log_warn("user add feature fail:%d\n", rc);
                return FDRSDK_ERRCODE_DBOPS;
            }
        }

        fdr_cp_usrdispatch(FDR_CP_EVT_USER_UPDATE, user.userid);
        add_string_to_array(rsp_params, user.userid);
    }
    return 0;
}

static int handler_user_info(cJSON *params, cJSON *rsp){
    fdr_dbs_user_t  *user;
    char * userid;
    cJSON *uj;
    cJSON *ary;
    cJSON *info;
    cJSON *item;
    struct tm *t;
    char buf[32];
    int rc;
    int i;

    ary = cJSON_CreateArray();
    if(ary == NULL){
        log_warn("user info create rsp array fail\n");
        return FDRSDK_ERRCODE_NOMEM;
    }
    cJSON_AddItemToObject(rsp, "params", ary);

    for(i = 0; i < cJSON_GetArraySize(params); i++){
        uj = cJSON_GetArrayItem(params, i);
        if(uj == NULL){
            log_warn("user info get param fail:%d\n", i);
            return FDRSDK_ERRCODE_INVALID;
        }

        userid = cJSON_GetObjectString(uj, "userid");
        if(userid == NULL){
            log_warn("user info get param fail, no userid\n", i);
            return FDRSDK_ERRCODE_INVALID;
        }

        // log_info("update user info : %s \n", userid);

        rc = fdr_dbs_user_lookup(userid, &user);
        if(rc != 0){
            log_warn("can't find user : %s - %d\n", userid, rc);
            return FDRSDK_ERRCODE_NOEXIT;
        }
        
        info = cJSON_CreateObject();
        if(info == NULL){
            log_warn("user info create user info fail\n");
            return FDRSDK_ERRCODE_NOMEM;
        }
        cJSON_AddItemToArray(ary, info);

        item = cJSON_CreateString(user->userid);
        if(item == NULL){
            rc = FDRSDK_ERRCODE_NOMEM;
            goto fail_exit;
        }
        cJSON_AddItemToObject(info, "userid", item);
        
        item = cJSON_CreateString(user->name);
        if(item == NULL){
            rc = FDRSDK_ERRCODE_NOMEM;
            goto fail_exit;
        }
        cJSON_AddItemToObject(info, "name", item);

        item = cJSON_CreateString(user->desc);
        if(item == NULL){
            rc = FDRSDK_ERRCODE_NOMEM;
            goto fail_exit;
        }
        cJSON_AddItemToObject(info, "desc", item);

        if(user->others != NULL){
            item = cJSON_CreateString(user->others);
        }else{
            item = cJSON_CreateNull();
        }
        if(item == NULL){
            rc = FDRSDK_ERRCODE_NOMEM;
            goto fail_exit;
        }
        cJSON_AddItemToObject(info, "others", item);
        

        item = cJSON_CreateNumber(user->status);
        if(item == NULL){
            rc = FDRSDK_ERRCODE_NOMEM;
            goto fail_exit;
        }
        cJSON_AddItemToObject(info, "status", item);

        item = cJSON_CreateNumber(user->perm);
        if(item == NULL){
            rc = FDRSDK_ERRCODE_NOMEM;
            goto fail_exit;
        }
        cJSON_AddItemToObject(info, "perm", item);

        item = cJSON_CreateString(user->create);
        if(item == NULL){
            rc = FDRSDK_ERRCODE_NOMEM;
            goto fail_exit;
        }
        cJSON_AddItemToObject(info, "create", item);


        t = localtime(&user->exptime);
        strftime(buf, 32, "%Y-%m-%d %H:%M:%S", t);
        item = cJSON_CreateString(buf);
        if(item == NULL){
            rc = FDRSDK_ERRCODE_NOMEM;
            goto fail_exit;
        }
        cJSON_AddItemToObject(info, "expire", item);

        item = cJSON_CreateString(user->modify);
        if(item == NULL){
            rc = FDRSDK_ERRCODE_NOMEM;
            goto fail_exit;
        }
        cJSON_AddItemToObject(info, "modify", item);

        fdr_dbs_user_free(user);
    }

    return 0;

fail_exit:
    fdr_dbs_user_free(user);
    return rc;
}

static int handler_user_list(cJSON *params, cJSON *rsp){
    char  **users;
    int offset, limit;
    int num;
    cJSON *ary, *item;
    int rc;
    int i;

    item = cJSON_GetObjectItem(params, "offset");
    if(item == 0){
        offset = 0;
    }else{
        offset = item->valueint;
    }

    item = cJSON_GetObjectItem(params, "limit");
    if(item == 0){
        limit = 10;
    }else{
        limit = item->valueint;
    }

    // FIXME:
    num = fdr_dbs_user_list(offset, limit, &users);
    if(num < 0){
        return FDRSDK_ERRCODE_NOEXIT;
    }

    ary = cJSON_CreateArray();
    if(ary == NULL){
        log_warn("user list create rsp array fail\n");
        rc = FDRSDK_ERRCODE_NOMEM;
        goto fail_exit;
    }
    cJSON_AddItemToObject(rsp, "params", ary);

    for(i = 0; i < num; i ++){
        rc = add_string_to_array(ary, users[i]);
        if(rc != 0){
            log_warn("user list create userid fail\n");
            rc = FDRSDK_ERRCODE_NOMEM;
            goto fail_exit;
        }
    }

    rc = 0;

fail_exit:
    for(i = 0; i < num; i ++){
        fdr_free(users[i]);
    }
    fdr_free(users);

    return rc;
}

int handler_user(cJSON *req, cJSON *rsp){
    cJSON *action;
    cJSON *params;
    char *act_str;
    int rc;

    action = cJSON_GetObjectItem(req, "action");
    if(action == NULL){
        return FDRSDK_ERRCODE_INVALID;
    }

    act_str = cJSON_GetStringValue(action);
    if(act_str == NULL){
        return FDRSDK_ERRCODE_INVALID;
    }

    params = cJSON_GetObjectItem(req, "params");
    if(params != NULL){
        if(! cJSON_IsArray(params)){
            return FDRSDK_ERRCODE_INVALID;
        }
    }else if (strcasecmp("list", act_str) != 0){
            return FDRSDK_ERRCODE_INVALID;
    }

    if(strcasecmp("add", act_str) == 0){
        rc = handler_user_add(params, rsp);
    }else if (strcasecmp("del", act_str) == 0){
        rc =  handler_user_del(params, rsp);
    }else if (strcasecmp("update", act_str) == 0){
        rc =  handler_user_update(params, rsp);
    }else if (strcasecmp("info", act_str) == 0){
        rc =  handler_user_info(params, rsp);
    }else if (strcasecmp("list", act_str) == 0){
        rc =  handler_user_list(req, rsp);
    }else{
        log_warn("unkown request action type : %s\n", act_str);
        rc = FDRSDK_ERRCODE_INVALID;
    }

    cJSON_AddNumberToObject(rsp, "retcode", rc);

    return rc;
}

