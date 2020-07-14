/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr-handler.h"
#include "fdr.h"
#include "fdr-dbs.h"
#include "fdr-logger.h"

#include "logger.h"
#include "cJSON.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int handler_logger(cJSON *req, cJSON *rsp){
    cJSON *action;
    cJSON *temp;
    cJSON *ary;
    char *act_str;
    int64_t seqnum;
    int number;
    int i;
    fdr_logger_record_t lr;
    int rc = 0;

    action = cJSON_GetObjectItem(req, "action");
    if(action == NULL){
        return FDRSDK_ERRCODE_INVALID;
    }

    act_str = cJSON_GetStringValue(action);
    if(act_str == NULL){
        return FDRSDK_ERRCODE_INVALID;
    }
    
    if(strcasecmp(act_str, "max") == 0){
        
        rc = fdr_logger_max(&seqnum);
        if(rc == 0){
            cJSON_AddNumberToObject(rsp, "retcode", 0);
            cJSON_AddNumberToObject(rsp, "logger-max", seqnum);
        }else{
            cJSON_AddNumberToObject(rsp, "retcode", FDRSDK_ERRCODE_DBOPS);
        }

        return 0;
    }

    if(strcasecmp(act_str, "fetch") != 0){
        cJSON_AddNumberToObject(rsp, "retcode", FDRSDK_ERRCODE_INVALID);
        return FDRSDK_ERRCODE_INVALID;
    }

    temp = cJSON_GetObjectItem(req, "seqnum");
    if(temp != NULL){
        if(! cJSON_IsNumber(temp)){
            return FDRSDK_ERRCODE_INVALID;
        }
        seqnum = (int64_t)temp->valuedouble;
    }else{
        return FDRSDK_ERRCODE_INVALID;
    }

    temp = cJSON_GetObjectItem(req, "number");
    if(temp != NULL){
        if(! cJSON_IsNumber(temp)){
            return FDRSDK_ERRCODE_INVALID;
        }
        number = temp->valueint;
    }else{
        number = 10;
    }

    ary = cJSON_CreateArray();
    if(ary == NULL){
        log_warn("user list create rsp array fail\n");
        return FDRSDK_ERRCODE_NOMEM;
    }
    cJSON_AddItemToObject(rsp, "logger", ary);

    for(i = 0; i < number; i++){
        rc = fdr_logger_get(seqnum++, &lr);
        if(rc != 0){
            break;
        }

        temp = cJSON_CreateObject();
        if(temp == NULL){
            rc = FDRSDK_ERRCODE_NOMEM;
            break;
        }
        cJSON_AddItemToArray(ary, temp);

        cJSON_AddNumberToObject(temp, "seqnum", (double)lr.seqnum);
        cJSON_AddNumberToObject(temp, "occtime", (double)lr.occtime);
        cJSON_AddNumberToObject(temp, "face_x", (double)lr.face_x);
        cJSON_AddNumberToObject(temp, "face_y", (double)lr.face_y);
        cJSON_AddNumberToObject(temp, "face_w", (double)lr.face_w);
        cJSON_AddNumberToObject(temp, "face_h", (double)lr.face_h);
        cJSON_AddNumberToObject(temp, "sharp", (double)lr.sharp);
        cJSON_AddNumberToObject(temp, "score", (double)lr.score);

        cJSON_AddStringToObject(temp, "userid", lr.userid);
    }

    if(i > 0)
        return 0;
    else
        cJSON_AddNumberToObject(rsp, "retcode", rc);

    return rc;
}

