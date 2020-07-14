/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr-mp.h"
#include "fdr-cp.h"
#include "fdr.h"

#include "fdr-handler.h"
#include "fdr-dbs.h"
#include "logger.h"
#include "fdr-path.h"
#include "fdr-qrcode.h"
#include "fdr-util.h"
#include "fdr-license.h"

#include "bisp_aie.h"
#include "bisp_hal.h"

#include "cJSON.h"
#include "sha256.h"
#include "base58.h"
#include "ecc.h"

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event_struct.h>
#include <event2/thread.h>
#include <event2/util.h>
#include <event2/dns.h>
#include <event2/dns_struct.h>

#include <pthread.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <assert.h>
#include <arpa/inet.h> 
#include <limits.h>

#include "list.h"

typedef enum {
    CLOUD_STATE_START    = 0,        // start idle
    CLOUD_STATE_CONNECTING,          // connecting to cloud
    CLOUD_STATE_CONNECTED,           // connected
    CLOUD_STATE_AUTHED               // authed
}fdr_mp_cloud_state_e;

typedef struct cloud_datatrans{
    uint16_t  type;
    uint16_t  fileid;

    FILE      *fd;
    size_t    fsize;
    // size_t    opsize;
    time_t    timestamp;

    char      path[PATH_MAX];

    struct list_head node;
}cloud_datatrans_t;

struct fdr_mp_cloud{
    fdr_mp_t    *mp;    // point to management plane server

    int recv_seqence;
    int send_seqence;

    uint16_t recv_bufflen;
    uint16_t send_bufflen;

    int recv_pendding;

    bcp_pm_header_t send_header;
    char send_buffer[BCP_PM_PAYLOAD_MAX];

    bcp_pm_header_t recv_header;
    char recv_buffer[BCP_PM_PAYLOAD_MAX];
    
    // client to cloud handles
    struct bufferevent* conn;
    evutil_socket_t     fd;
    struct event ev_conn;
    int state;

    // heartbeat timer
    struct event ev_heartbeat;
    time_t  ts_heartbeat;

    // file transfer handles
    uint16_t fileid;
    struct list_head fileids;

    // dns resolvler
    struct evdns_base *dns_base;
};

typedef struct cloud_path{
    char *type;
    char *path;
    char *fmt;
    char *fix;
    int  ops;   // 0 - do nothing; 1 - get logimage, 2 - get oplogfile, 3 - add suffix
}cloud_path_t;

typedef enum {
    PUSHDATA_TYPE_UPGRADE  = 0,
    PULLDATA_TYPE_LOGIMAGE,
    PULLDATA_TYPE_LOGFILE,
    PUSHDATA_TYPE_USERID,
    PUSHDATA_TYPE_USERUI
}pushdata_type_e;

static cloud_path_t paths[] = {
    {   "log-image",    CAPTURE_IMAGE_PATH,     "%s/%s",        NULL,       PULLDATA_TYPE_LOGIMAGE  },
    {   "log-file",     OPLOG_PATH,             "%s/%s",        NULL,       PULLDATA_TYPE_LOGFILE   },
    {   "userface",     USER_IMAGE_PATH,        "%s/%s%s",      ".nv12",    PUSHDATA_TYPE_USERID    },
    {   "userui",       USER_IMAGE_PATH,        "%s/%s%s",      ".bmp",     PUSHDATA_TYPE_USERUI    },
    {   "upgrade",      UPGRADE_PACKAGE_PATH,   "%s/%s",        NULL,       PUSHDATA_TYPE_UPGRADE   }
};

static void cloud_response(fdr_mp_cloud_t *fc, uint16_t type, cJSON *rsp, int retcode);
static void cloud_read_cb(struct bufferevent *bev, void *ctx);
static void cloud_write_cb(struct bufferevent *bev, void *ctx);
static void cloud_event_cb(struct bufferevent *bev, short what, void *ctx);
static void cloud_reset_socket(fdr_mp_cloud_t *fc);
static int  connect_to_cloud(fdr_mp_cloud_t *fc);

static int cloud_sign(time_t ts, const char *devid, char **b58str){
    sha256_context ctx;
    char checksum[32];
    char timestamp[32];
    uint8_t p_signature[ECC_BYTES*2];
    uint8_t p_privateKey[ECC_BYTES];
    char *signature;
    size_t len = 256;
    int rc;

    memcpy(p_privateKey, fdr_get_dev_prikey(), ECC_BYTES);

    sprintf(timestamp, "%ld", ts);

    sha256_init(&ctx);
    sha256_hash(&ctx, (uint8_t*)devid, strlen(devid));
    sha256_hash(&ctx, (uint8_t*)timestamp, strlen(timestamp));
    sha256_done(&ctx, (uint8_t*)checksum);

    rc = ecdsa_sign(p_privateKey, (uint8_t*)checksum, p_signature);
    if(rc != 1){
        log_warn("cloud_sign => ecdsa_sign fail\n");
        return -1;
    }

#if 0
    log_info("cloud_sign => p_signature:\n");
    for(int i = 0; i < 64; i ++){
        printf("%02x", (uint8_t)p_signature[i]);
    }
    printf("\n");

    log_info("cloud_sign => checksum:\n");
    for(int i = 0; i < 32; i ++){
        printf("%02x", (uint8_t)checksum[i]);
    }
    printf("\n");

    log_info("cloud_sign => p_privateKey:\n");
    for(int i = 0; i < 32; i ++){
        printf("%02x", (uint8_t)p_privateKey[i]);
    }
    printf("\n");
#endif
    signature = fdr_malloc(len);
    if(signature == NULL){
        log_warn("cloud_sign => no memory\n");
        return -1;
    }

    if(base58_encode(p_signature, ECC_BYTES*2, signature, &len) != NULL){
        *b58str = signature;
        return 0;
    }else{
        fdr_free(signature);
        return -1;
    }

    return 0;
}

static int cloud_verify(const char *b58str, const char *devid, time_t ts){
    sha256_context ctx;
    char checksum[32];
    char timestamp[32];
    uint8_t p_signature[ECC_BYTES*2];
    uint8_t p_publicKey[ECC_BYTES+1];
    size_t len = ECC_BYTES*2;
    int rc;

    memcpy(p_publicKey, fdr_get_svr_pubkey(), ECC_BYTES+1);

    sprintf(timestamp, "%ld", ts);

    sha256_init(&ctx);
    sha256_hash(&ctx, (uint8_t*)devid, strlen(devid));
    sha256_hash(&ctx, (uint8_t*)timestamp, strlen(timestamp));
    sha256_done(&ctx, (uint8_t*)checksum);

    len = base58_decode(b58str, strlen(b58str), p_signature, len);
    if(len <= 0){
        log_warn("cloud_verify => b58tobin fail\n");
        return -1;
    }

#if 0
    log_info("cloud_sign => p_signature:\n");
    for(int i = 0; i < 64; i ++){
        printf("%02x", (uint8_t)p_signature[i]);
    }
    printf("\n");

    log_info("cloud_verify => checksum:\n");
    for(int i = 0; i < 32; i ++){
        printf("%02x", (uint8_t)checksum[i]);
    }
    printf("\n");

    log_info("cloud_verify => p_publicKey:\n");
    for(int i = 0; i < 32; i ++){
        printf("%02x", (uint8_t)p_publicKey[i]);
    }
    printf("\n");
#endif
    rc = ecdsa_verify(p_publicKey, (uint8_t*)checksum, p_signature);
    if(rc != 1){
        log_warn("cloud_verify => ecdsa_verify fail\n");
        return -1;
    }

    return 0;
}


static int cloud_check_state(fdr_mp_cloud_t *fc){
    return fc->state == CLOUD_STATE_AUTHED;
}

static void cloud_auth_response(fdr_mp_cloud_t *fc, cJSON *req, cJSON *rsp){
    char *devid = NULL;
    char *action;
    char *version;
    char *signature;
    cJSON *item;
    time_t ts;
    int rc;

    action = cJSON_GetObjectString(req, "action");
    if((action == NULL) || (strcasecmp(action, "auth") != 0)){
        log_warn("cloud_auth_response => action not auth\n");
        return ;
    }

    version = cJSON_GetObjectString(req, "version");
    if((version == NULL) || (strcasecmp(version, FDR_VERSION) != 0)){
        log_warn("cloud_auth_response => version wrong\n");
        return ;
    }

    signature = cJSON_GetObjectString(req, "signature");
    if(signature == NULL){
        log_warn("cloud_auth_response => without signature\n");
        return ;
    }

    item = cJSON_GetObjectItem(req, "timestamp");
    if(item == NULL){
        log_warn("cloud_auth_response => without timestamp\n");
        return ;
    }
    ts = (time_t)item->valuedouble;

    item = cJSON_GetObjectItem(req, "retcode");
    if(item == NULL){
        log_warn("cloud_auth_response => without retcode\n");
        return ;
    }
    rc = item->valueint;
    if(rc != 0){
        log_warn("cloud_auth_response => retcode %d\n", rc);
        return ;
    }

    rc = fdr_dbs_config_get("device-id", &devid);
    if(rc < 0){
        log_warn("cloud_auth_response => read device-id fail\n");
        return;
    }
    
    rc = cloud_verify(signature, devid, ts);
    if(rc == 0){
        fc->state = CLOUD_STATE_AUTHED;
    }

    log_info("cloud_auth_response => auth to cloud ok\n");
    fdr_free(devid);
}

static int_least32_t cloud_auth_request(fdr_mp_cloud_t *fc){
    char *devid = NULL;
    time_t timestamp = time(NULL) + 60;    //  60 seconds
    char *signature = NULL;
    cJSON *req;
    int rc;

    rc = fdr_dbs_config_get("device-id", &devid);
    if(rc < 0){
        log_warn("cloud_auth_request => read device-id fail\n");
        return -1;
    }

    req = cJSON_CreateObject();
    if(req == NULL){
        log_warn("cloud_auth_request => create request json\n");
        fdr_free(devid);
        return -1;
    }

    rc = cloud_sign(timestamp, devid, &signature);
    if(rc != 0){
        log_warn("cloud_auth_request => cloud_sign fail\n");
        fdr_free(devid);
        cJSON_Delete(req);
        return -1;
    }

    cJSON_AddStringToObject(req, "action",      "login");
    cJSON_AddStringToObject(req, "device-id",   devid);
    cJSON_AddStringToObject(req, "version",     FDR_VERSION);
    cJSON_AddNumberToObject(req, "timestamp",   timestamp);
    cJSON_AddStringToObject(req, "signature",   signature);

    fc->send_header.seqence = 0;
    fc->send_header.session = 0;

    fdr_free(devid);
    fdr_free(signature);

    cloud_response(fc, BCP_PMT_AUTH_REQ, req, 0);

    return 0;
}

static void cloud_response(fdr_mp_cloud_t *fc, uint16_t type, cJSON *rsp, int retcode){
    int len;
    int reserver;


    fc->send_header.type     = htons(type);
    fc->send_header.seqence  = htonl(fc->recv_seqence);
    fc->send_header.session  = fc->recv_header.session;       // keep it unchange

    if(cJSON_GetObjectItem(rsp, "retcode") == NULL){
        cJSON_AddNumberToObject(rsp, "retcode", retcode);
    }

    len = cJSON_PrintPreallocated(rsp, fc->send_buffer, BCP_PM_PAYLOAD_MAX - 1, 1);
    cJSON_free(rsp);
    if(len == 0){
        log_warn("cloud_response => response cjson to buffer fail\n");
        return ;
    }

    len = strlen(fc->send_buffer);
    reserver = len % 4;
    if(reserver > 0)
        reserver = 4 - reserver;

    fc->send_header.size = htons(len + reserver);

    if(reserver > 0){
        memset(fc->send_buffer + len, 0, reserver);
    }

    bufferevent_write(fc->conn, &fc->send_header, sizeof(bcp_pm_header_t));
    bufferevent_write(fc->conn, fc->send_buffer, len + reserver);
}

static int cloud_getpath(const char *type, const char *filename, int64_t timestamp, cloud_datatrans_t *dt){
    cloud_path_t *path;
    int i; 

    for(i = 0; i < sizeof(paths)/ sizeof(cloud_path_t); i++){
        path = &paths[i];

        if(strcasecmp(type, path->type) == 0){
            dt->type = path->ops;

            switch(path->ops){
            case PUSHDATA_TYPE_UPGRADE:
                sprintf(dt->path, path->fmt, path->path, filename);
                break;
            case PULLDATA_TYPE_LOGIMAGE:
                fdr_util_imagepath(dt->path, timestamp);
                break;
            case PULLDATA_TYPE_LOGFILE:
                fdr_util_oplogpath(dt->path, timestamp);
                break;

            case PUSHDATA_TYPE_USERID:
            case PUSHDATA_TYPE_USERUI:
                log_info("cloud_getpath => %s - %s, %s %s\n", path->fmt, path->path, filename, path->fix);
                sprintf(dt->path, path->fmt, path->path, filename, path->fix);
                break;
            default:
                return -1;
            }

            break;
        }
    }

    return 0;
}

static int cloud_datatrans_create(fdr_mp_cloud_t *fc, uint16_t reqtype, const char *type, const char *fname, int64_t timestamp, size_t *fsize){
    cloud_datatrans_t  *dt;
    cloud_datatrans_t  *temp;
    int rc;

    log_info("cloud_datatrans_create => %s - %s, %d\n", type, fname == NULL ? "null" : fname,  *fsize);

    dt = (cloud_datatrans_t *)fdr_malloc(sizeof(cloud_datatrans_t));
    if(dt == NULL){
        log_warn("cloud_datatrans_create => malloc for data trans fail!\n");
        return -1;
    }
    memset(dt, 0, sizeof(cloud_datatrans_t));

    dt->timestamp = time(NULL);

    rc = cloud_getpath(type, fname, timestamp, dt);
    if(rc < 0){
        log_warn("cloud_datatrans_create => get file path fail!\n");
        fdr_free(dt);
        return -1;
    }

    list_for_each_entry(temp, &fc->fileids, node){
        if(strcmp(dt->path, temp->path) == 0){
            rc = dt->fileid;
            // log_info("cloud_datatrans_create => file %s will reopen!\n", temp->path);
            fdr_free(dt);
            *fsize = temp->fsize;
            return temp->fileid;
        }
    }

    if(reqtype == BCP_PMT_PUSHDATA_REQ){
        log_info("cloud_datatrans_create => open file %s \n", dt->path);
        dt->fd = fopen(dt->path, "w+");
        dt->fsize = *fsize;
    }else if (reqtype == BCP_PMT_PULLDATA_REQ){
        struct stat st;

        // log_info("cloud_datatrans_create => Pull %s\n", dt->path);

        rc = stat(dt->path, &st);
        if(rc == 0){
            dt->fsize = st.st_size;
            *fsize = dt->fsize;

            dt->fd = fopen(dt->path, "r+");
        }
    }

    if(dt->fd == NULL){
        log_warn("cloud_datatrans_create => file not exist!\n");
        fdr_free(dt);
        return -1;
    }

    dt->fileid = ++fc->fileid;
    
    list_add(&dt->node, &fc->fileids);

    return dt->fileid;
}

static int  cloud_datatrans_delete(fdr_mp_cloud_t *fc){
    cloud_datatrans_t  *dt;
    cloud_datatrans_t  *temp;

    if(list_empty(&fc->fileids)){
        return 0;
    }

    list_for_each_entry_safe(dt, temp, &fc->fileids, node){
        list_del(&dt->node);

        fclose(dt->fd);
        fdr_free(dt);
    }

    return 0;
}

static uint16_t cloud_datatrans_write(fdr_mp_cloud_t *fc, int fileid, const char *buf, uint32_t offset, uint16_t len){
    cloud_datatrans_t  *dt = NULL;
    size_t wlen = 0;
    char *userid;

    if(list_empty(&fc->fileids)){
        return 0;
    }

    list_for_each_entry(dt, &fc->fileids, node){
        if(dt->fileid == fileid){
            // log_info("cloud_datatrans_write => write to: %d, size:%d\n", offset, len);
            fseek(dt->fd, offset, SEEK_SET);
            wlen = fwrite(buf, 1, len, dt->fd);
            //dt->opsize += wlen;

            if((offset + wlen) >= dt->fsize){
                // log_info("cloud_datatrans_write => all data %d recved, close fd\n", dt->fsize);
                list_del(&dt->node);

                // user add feature
                if((dt->type == PUSHDATA_TYPE_USERID) ||(dt->type == PUSHDATA_TYPE_USERUI)){
                    // strip suffix .nv12 / .bmp
                    userid = strrchr(dt->path, '.');
                    *userid = '\0';  // 
                    // strip prifix path
                    userid = strrchr(dt->path, '/');
                    userid ++;

                    if(dt->type == PUSHDATA_TYPE_USERUI){
                        fdr_dbs_user_setstatus(userid, FDR_DBS_USER_STATUS_UIIMAGE);

                        fdr_cp_usrdispatch(FDR_CP_EVT_USER_UPDATE, userid);
                    }else{
                        size_t len;
                        char *buf = (char *)fdr_malloc(dt->fsize);
                        if(buf != NULL){
                            fseek(dt->fd, 0, SEEK_SET);
                            len = fread(buf, 1, dt->fsize, dt->fd);
                            if(len == dt->fsize){
                                fdr_mp_update_feature(userid, buf, len);
                            }else{
                                log_warn("cloud_datatrans_write => read data size wrong\n");
                            }
                            fdr_free(buf);
                        }else{
                            log_warn("cloud_datatrans_write => alloc memory fail for feature\n");
                        }
                    }
                }else if(dt->type == PUSHDATA_TYPE_UPGRADE){
                    // FIXME : call upgrade 

                }
                
                fclose(dt->fd);
                fdr_free(dt);
            }

            break;
        }
    }

    return wlen;
}

static uint16_t cloud_datatrans_read(fdr_mp_cloud_t *fc, int fileid, char *buf, uint32_t offset, uint16_t len){
    cloud_datatrans_t  *dt = NULL;
    size_t wlen = 0;

    if(list_empty(&fc->fileids)){
        return 0;
    }

    list_for_each_entry(dt, &fc->fileids, node){
        if(dt->fileid == fileid){
            fseek(dt->fd, offset, SEEK_SET);
            wlen = fread(buf, 1, len, dt->fd);

            if((offset + wlen) >= dt->fsize){
                list_del(&dt->node);
                fclose(dt->fd);
                fdr_free(dt);
            }
            
            break;
        }
    }


    return (uint16_t)wlen;
}

static int cloud_handler_pushdata(fdr_mp_cloud_t *fc, cJSON *req, cJSON *rsp){
    const char *action = NULL;
    const char *type   = NULL;
    const char *fname  = NULL;
    cJSON *item;
    size_t fsize;
    int fileid;

    // log_info("cloud_handler_pushdata => \n");

    action = cJSON_GetObjectString(req, "action");
    if((action == NULL) || (strcasecmp(action, "pushdata") != 0)){
        return FDRSDK_ERRCODE_INVALID;
    }

    type = cJSON_GetObjectString(req, "type");
    if(type == NULL){
        return FDRSDK_ERRCODE_INVALID;
    }

    fname = cJSON_GetObjectString(req, "filename");
    if(fname == NULL){
        return FDRSDK_ERRCODE_INVALID;
    }

    item = cJSON_GetObjectItem(req, "filesize");
    if(item == NULL){
        return FDRSDK_ERRCODE_INVALID;
    }
    fsize = (size_t)item->valuedouble;

    fileid = cloud_datatrans_create(fc, BCP_PMT_PUSHDATA_REQ, type, fname, 0, &fsize);
    if(fileid < 0){
        return FDRSDK_ERRCODE_NOEXIT;
    }

    cJSON_AddStringToObject(rsp, "action", "pushdata");
    cJSON_AddNumberToObject(rsp, "retcode", 0);
    cJSON_AddNumberToObject(rsp, "fileid", fileid);

    return 0;
}

static int cloud_handler_pulldata(fdr_mp_cloud_t *fc, cJSON *req, cJSON *rsp){
    const char *action = NULL;
    const char *type   = NULL;
    const char *userid  = NULL;
    int64_t  timestamp = 0;
    size_t   fsize = 0;
    cJSON *item;
    int   fileid;

    action = cJSON_GetObjectString(req, "action");
    if((action == NULL) || (strcasecmp(action, "pulldata") != 0)){
        return FDRSDK_ERRCODE_INVALID;
    }

    type = cJSON_GetObjectString(req, "type");
    if(type == NULL){
        return FDRSDK_ERRCODE_INVALID;
    }

    userid = cJSON_GetObjectString(req, "userid");
    item = cJSON_GetObjectItem(req, "timestamp");
    if(item != NULL){
        timestamp = (int64_t)item->valuedouble;
    }

    fileid = cloud_datatrans_create(fc, BCP_PMT_PULLDATA_REQ, type, userid, timestamp, &fsize);
    if(fileid < 0){
        return FDRSDK_ERRCODE_NOEXIT;
    }

    cJSON_AddStringToObject(rsp, "action", "pulldata");
    cJSON_AddNumberToObject(rsp, "retcode", 0);
    cJSON_AddNumberToObject(rsp, "fileid", fileid);
    cJSON_AddNumberToObject(rsp, "filesize", fsize);

    return 0;
}

static int handler_device(cJSON *req, cJSON *rsp){
    const char *action = NULL;

    action = cJSON_GetObjectString(req, "action");
    if(action == NULL){
        cJSON_AddNumberToObject(rsp, "retcode", FDRSDK_ERRCODE_INVALID);
        return FDRSDK_ERRCODE_INVALID;
    }

    if(strcasecmp(action, "reboot") == 0){
        bisp_hal_restart();
    }else if(strcasecmp(action, "reset") == 0){
        // FIXME:
        // clean user/logger data
        // set appliance to inactivation mode
        // fdr_config_set();
        log_info("handler_device => reset device\n");
    }else{
        cJSON_AddNumberToObject(rsp, "retcode", FDRSDK_ERRCODE_INVALID);
        return FDRSDK_ERRCODE_INVALID;
    }

    cJSON_AddNumberToObject(rsp, "retcode", 0);

    return 0;
}

static void cloud_management_request(fdr_mp_cloud_t *fc){
    cJSON *req;
    cJSON *rsp;
    int rc;

    log_info("cloud_management_request => %d - %d  - %s\n", fc->recv_header.type, fc->recv_bufflen, fc->recv_buffer);

    if((fc->recv_header.type != BCP_PMT_AUTH_RSP) && (!cloud_check_state(fc))){
        log_warn("cloud_management_request => get other request before auth\n");
        cloud_reset_socket(fc);
        return ;
    }

    req = cJSON_Parse(fc->recv_buffer);
    if(req == NULL){
        log_warn("cloud_management_request => parse json fail:%d-%s\n", fc->recv_bufflen, fc->recv_buffer);
        return ;
    }

    rsp = cJSON_CreateObject();
    if(rsp == NULL){
        log_warn("cloud_management_request => create rsp fail\n");
        cJSON_Delete(req);
        return ;
    }

    switch(fc->recv_header.type){
    case BCP_PMT_AUTH_RSP:
        cloud_auth_response(fc, req, rsp);
        cJSON_Delete(req);
        cJSON_Delete(rsp);
        return ;

    case BCP_PMT_INFO_REQ:
        rc = handler_info(req, rsp);
        break;

    case BCP_PMT_USER_REQ:
        rc = handler_user(req, rsp);
        break;

    case BCP_PMT_CONF_REQ:
        rc = handler_config(req, rsp);
        break;

    case BCP_PMT_LOGGER_REQ:
        rc = handler_logger(req, rsp);
        break;

    case BCP_PMT_GUEST_REQ:
        rc = handler_guest(req, rsp);
        break;

    case BCP_PMT_PUSHDATA_REQ:
        rc = cloud_handler_pushdata(fc, req, rsp);
        break;

    case BCP_PMT_PULLDATA_REQ:
        rc = cloud_handler_pulldata(fc, req, rsp);
        break;
    
    case BCP_PMT_DEVICE_REQ:
        rc = handler_device(req, rsp);
        break;

    default:
        log_warn("cloud_management_request => unkown req %d\n", fc->recv_header.type);
        cJSON_Delete(req);
        cJSON_Delete(rsp);
        cloud_reset_socket(fc);
        return;
    }

    cloud_response(fc, fc->recv_header.type + 1, rsp, rc);
    cJSON_Delete(req);
}

static void cloud_settimer(struct event *tmr, int seconds){
    if(seconds > 0){
        struct timeval tv = {seconds, 0};
        event_add(tmr, &tv);
    }else{
        event_del(tmr);
    }
}

static void cloud_heartbeat_proc(int fd, short events, void *args){
    fdr_mp_cloud_t *fc = (fdr_mp_cloud_t *)args;
    fdr_config_t *config = fdr_config();

    cloud_settimer(&fc->ev_heartbeat, config->heartbeat);

    if(fc->state < CLOUD_STATE_CONNECTED){
        fc->ts_heartbeat = time(NULL); // reset last heartbeat timestamp
        return ;
    }

    if((fc->ts_heartbeat + config->heartbeat*3) < time(NULL)){
        cloud_reset_socket(fc);     // reset socket
        return ;
    }

    fc->send_header.type     = htons(BCP_PMT_HEARTBEAT_REQ);
    fc->send_header.seqence  = 0;
    fc->send_header.session  = 0;
    fc->send_header.size     = 0;

    bufferevent_write(fc->conn, &fc->send_header, sizeof(bcp_pm_header_t));
}

static void cloud_conn_timeout_proc(int fd, short events, void *args){
    fdr_mp_cloud_t *fc = (fdr_mp_cloud_t *)args;
    log_info("cloud_conn_timeout_proc => in \n");

    if(fc->state >= CLOUD_STATE_CONNECTED){
        log_info("cloud_conn_timeout_proc => fc->state %d \n", fc->state);
        return ;
    }

    log_info("cloud_conn_timeout_proc => reconnectting ...\n");

    cloud_reset_socket(fc);
}

static void cloud_datatrans_request(fdr_mp_cloud_t *fc){
    bcp_pm_transfer_data_t  *tdr, *tds;
    sha256_context  ctx;
    uint8_t checksum[BCP_PM_CHECKSUM_MAX];
    cJSON *rsp;
    uint16_t size;
    int reserver;
    char dummy[4] = {};
    
    tdr = (bcp_pm_transfer_data_t*)fc->recv_buffer;
    tdr->fileid  = ntohs(tdr->fileid);
    tdr->datalen = ntohs(tdr->datalen);
    tdr->offset  = ntohl(tdr->offset);

    if(fc->recv_header.size > sizeof(bcp_pm_transfer_data_t)){
        // push data

        sha256_init(&ctx);
        sha256_hash(&ctx, (uint8_t *)fc->recv_buffer + sizeof(bcp_pm_transfer_data_t), tdr->datalen);
        sha256_done(&ctx, checksum);
        if(memcmp(checksum, tdr->checksum, BCP_PM_CHECKSUM_MAX) != 0){
            log_warn("cloud_transfer_data => checksum invalid\n");
            return ;
        }

        size = cloud_datatrans_write(fc, tdr->fileid, fc->recv_buffer + sizeof(bcp_pm_transfer_data_t), 
                                    tdr->offset, tdr->datalen);

        rsp = cJSON_CreateObject();
        if(rsp == NULL){
            log_warn("cloud_transfer_data => create response rsp fail\n");
            return ;
        }

        cJSON_AddStringToObject(rsp, "action", "pushdata");
        cJSON_AddNumberToObject(rsp, "fileid", tdr->fileid);    
        cJSON_AddNumberToObject(rsp, "retcode", (size == tdr->datalen) ?  0 : FDRSDK_ERRCODE_INVALID);

        cloud_response(fc, BCP_PMT_PUSHDATA_RSP,  rsp,  0);
    }else{
        // pull data
        if(tdr->datalen > BCP_PM_SLICE_MAX){
            tdr->datalen = BCP_PM_SLICE_MAX;
        }

        tds = (bcp_pm_transfer_data_t*)fc->send_buffer;
        size = cloud_datatrans_read(fc, tdr->fileid, fc->send_buffer + sizeof(bcp_pm_transfer_data_t), 
                                    tdr->offset, tdr->datalen);

        sha256_init(&ctx);
        sha256_hash(&ctx, (uint8_t *)fc->send_buffer + sizeof(bcp_pm_transfer_data_t), size);
        sha256_done(&ctx, tds->checksum);

        fc->send_header.type = htons(BCP_PMT_DATATRANS_RSP);

        reserver = size % 4;
        if(reserver > 0)
            reserver = 4 - reserver;

        fc->send_header.size = htons(sizeof(bcp_pm_transfer_data_t) + size + reserver);
        fc->send_header.seqence  = htonl(fc->recv_seqence);
        fc->send_header.session  = fc->recv_header.session;       // keep it unchange
            
        tds->fileid  = htons(tdr->fileid);
        tds->datalen = htons(size);
        tds->offset  = htonl(tdr->offset);

        bufferevent_write(fc->conn, &fc->send_header, sizeof(bcp_pm_header_t));
        bufferevent_write(fc->conn, fc->send_buffer, sizeof(bcp_pm_transfer_data_t) + size);
        if(reserver > 0){
            bufferevent_write(fc->conn, dummy, reserver);
        }
    }
}

static void cloud_reset_socket(fdr_mp_cloud_t *fc){         
    connect_to_cloud(fc);
}

static void cloud_read_cb(struct bufferevent *bev, void *ctx){
    fdr_mp_cloud_t *fc = (fdr_mp_cloud_t *)ctx;
    size_t len;

    do{
        len = evbuffer_get_length(bufferevent_get_input(bev));
        if(len <= 0){
            break;
        }

        // log_info("cloud_read_cb => buffered data size:%d\n", len);

        if(fc->recv_pendding){
            len = bufferevent_read(bev, &fc->recv_buffer[fc->recv_bufflen], fc->recv_header.size - fc->recv_bufflen);
            fc->recv_bufflen += len;

            // log_info("cloud_read_cb => pendding reading type:%d, read:%d, left:%d\n", fc->recv_header.type, len, fc->recv_header.size - fc->recv_bufflen);

            if(fc->recv_bufflen < fc->recv_header.size){
                continue;  // wait for next read to get total pdu
            }
            
            fc->recv_pendding = 0;

            if(fc->recv_header.type == BCP_PMT_DATATRANS_REQ){
                cloud_datatrans_request(fc);  // push or pull data
            }else{
                fc->recv_buffer[len] = '\0';
                // log_info("cloud_read_cb => body %s\n", fc->recv_buffer);
                cloud_management_request(fc);   // normal request
            }

            continue;
        }

        len = bufferevent_read(bev, &fc->recv_header, sizeof(bcp_pm_header_t));
        if(len < sizeof(fc->recv_header)){  
            // have alread set watermark, reset 
            cloud_reset_socket(fc);
            break;
        }

        // normal message processing
        fc->recv_header.type     = ntohs(fc->recv_header.type);
        fc->recv_header.size     = ntohs(fc->recv_header.size);
        fc->recv_header.seqence  = ntohl(fc->recv_header.seqence);

        // heartbeat message without body
        if(fc->recv_header.type == BCP_PMT_HEARTBEAT_RSP){
            // update heartbeat timestamp, and drop it
            fc->ts_heartbeat = time(NULL);
            continue;
        }

        if(fc->recv_header.seqence != fc->recv_seqence){
            // log_warn("cloud_read_cb => seqence wrong %d - %d\n", fc->recv_header.seqence, fc->recv_seqence);
            cloud_response(fc, fc->recv_header.type + 1, NULL, FDRSDK_ERRCODE_INVALID);
            cloud_reset_socket(fc);
            break;
        }
        fc->recv_seqence++;

        // log_info("cloud_read_cb => header t:%d, s:%d, s:%d, s:%d\n", fc->recv_header.type, 
        //                  fc->recv_header.size, fc->recv_header.seqence, fc->recv_header.session);
        
        len = bufferevent_read(bev, fc->recv_buffer, fc->recv_header.size);
        fc->recv_bufflen = len;

        if(len < fc->recv_header.size){
            fc->recv_pendding = 1;
            
            // log_info("cloud_read_cb => pendding, read:%d, total:%d\n", len, fc->recv_header.size);
            continue;
        }
        
        if(fc->recv_header.type == BCP_PMT_DATATRANS_REQ){
            cloud_datatrans_request(fc);
        }else{
            fc->recv_buffer[len] = '\0';
            // log_info("cloud_read_cb => body %s\n", fc->recv_buffer);
            cloud_management_request(fc);
        }

        continue;

    }while(1);
}

static void cloud_write_cb(struct bufferevent *bev, void *ctx){
    // fdr_mp_cloud_t *fc = (fdr_mp_cloud_t *)ctx;

    // log_info("cloud_write_cb => %d\n", evbuffer_get_length(bufferevent_get_input(bev)));
    // do nothing...
}

static void cloud_event_cb(struct bufferevent *bev, short what, void *ctx){
    fdr_mp_cloud_t *fc = (fdr_mp_cloud_t *)ctx;
    cloud_datatrans_t  *dt, *temp;
    fdr_config_t *config = fdr_config();

    log_info("cloud_event_cb => event : 0x%x\n", what);

    if(what & (BEV_EVENT_ERROR|BEV_EVENT_TIMEOUT|BEV_EVENT_EOF)){
        fc->state = CLOUD_STATE_START;
        log_info("cloud_event_cb => error code %d\n", bufferevent_socket_get_dns_error(fc->conn));

        if(fc->conn != NULL){
		    bufferevent_free(fc->conn);
            fc->conn = NULL;
        }

        // reset socket
        // evutil_closesocket(fc->fd);
        fc->fd = -1;

        log_info("cloud_event_cb => reset timer %d\n", config->cloud_reconnect);
        cloud_settimer(&fc->ev_conn, config->cloud_reconnect);
        return;
    }

    if(what & BEV_EVENT_CONNECTED){
        fc->state = CLOUD_STATE_CONNECTED;
        fc->recv_seqence = 0;
        fc->recv_pendding = 0;
        fc->recv_bufflen = 0;
        fc->send_seqence = 0;
        fc->send_bufflen = 0;
        fc->fileid = 0;
        fc->ts_heartbeat = time(NULL);
        
        cloud_settimer(&fc->ev_conn, 0);

        list_for_each_entry_safe(dt, temp, &fc->fileids, node){
            fclose(dt->fd);
            list_del(&dt->node);
            fdr_free(dt);
        }

        evutil_make_socket_nonblocking(fc->fd);

        log_info("cloud_event_cb => authen ...\n");

        cloud_auth_request(fc);
    }
}

static int connect_to_cloud(fdr_mp_cloud_t *fc){
    fdr_config_t *config = fdr_config();

    log_info("connect_to_cloud => connectting ...\n");

    if(fc->conn != NULL){
        bufferevent_free(fc->conn);
        fc->conn = NULL;
        fc->fd = -1;
    }

    cloud_settimer(&fc->ev_conn, config->cloud_reconnect);

    fc->fd = socket(AF_INET, SOCK_STREAM, 0);
    fc->conn = bufferevent_socket_new(fdr_mp_getbase(fc->mp), fc->fd, BEV_OPT_CLOSE_ON_FREE);
    if(fc->conn == NULL){
        log_warn("connect_to_cloud => create bufferevent failed!\n");
        return -1;
    }

    //low_warn("connect_to_cloud => socket %d\n", fc->fd);

    bufferevent_enable(fc->conn, EV_WRITE|EV_READ|EV_CLOSED);
    bufferevent_setcb(fc->conn, cloud_read_cb, cloud_write_cb, cloud_event_cb, fc);
    
    fc->state = CLOUD_STATE_CONNECTING;

    if(fc->dns_base != NULL){
        evdns_base_clear_host_addresses(fc->dns_base);
    }else{
        fc->dns_base = evdns_base_new(fdr_mp_getbase(fc->mp), EVDNS_BASE_INITIALIZE_NAMESERVERS);
        if(fc->dns_base == NULL){
            log_warn("connect_to_cloud => new evdns fail!\n");
            return -1;
        }
    }

    return bufferevent_socket_connect_hostname(fc->conn, fc->dns_base, AF_INET, config->cloud_addr, config->cloud_port);

}


int fdr_mp_create_cloud(fdr_mp_t *mp){
    fdr_mp_cloud_t *fc;
    fdr_config_t *config = fdr_config();

    assert(mp != NULL);

    log_info("fdr_mp_create_client ...\n");

    fc = (fdr_mp_cloud_t*)fdr_malloc(sizeof(fdr_mp_cloud_t));
    if(fc == NULL){
        log_warn("fdr_mp_create_client => no more memory\n");
        return -1;
    }
    memset(fc, 0, sizeof(fdr_mp_cloud_t));
    fc->fd = -1;

#if 0
    fc->dns_base = evdns_base_new(fdr_mp_getbase(mp), EVDNS_BASE_INITIALIZE_NAMESERVERS);
    if(fc->dns_base == NULL){
        log_warn("cloud_dns_init => new evdns fail!\n");
        fdr_free(fc);
        return -1;
    }
#endif

    fdr_mp_setcloud(mp, fc);

    fc->state = CLOUD_STATE_START;
    fc->mp = mp;
    INIT_LIST_HEAD(&fc->fileids);

    event_assign(&fc->ev_heartbeat, fdr_mp_getbase(mp), -1, EV_TIMEOUT, cloud_heartbeat_proc, fc);
    cloud_settimer(&fc->ev_heartbeat, config->heartbeat);

    event_assign(&fc->ev_conn,      fdr_mp_getbase(mp), -1, EV_TIMEOUT, cloud_conn_timeout_proc, fc);
    
    // FIXME: hostname
    log_info("fdr_mp_create_client => connect_to_cloud ...\n");
    connect_to_cloud(fc);
#if 0
    if(connect_to_cloud(fc) < 0){
        log_warn("fdr_mp_create_client => reset ev_conn %d\n", config->cloud_reconnect);
        cloud_settimer(&fc->ev_conn, config->cloud_reconnect);
    }
#endif
    return 0;
}

int fdr_mp_destroy_cloud(fdr_mp_t *mp){
    fdr_mp_cloud_t *fc = fdr_mp_getcloud(mp);

    assert(mp != NULL);

    cloud_datatrans_delete(fc);
    evdns_base_free(fc->dns_base, 1);
    fdr_free(fc);

    fdr_mp_setcloud(mp, NULL);

    return 0;
}

void fdr_mp_cloud_report(fdr_mp_t *mp, fdr_mp_rlog_t *rl){
    fdr_mp_cloud_t *fc = fdr_mp_getcloud(mp);
    bcp_pm_header_t *header;
    char *body;
    int len;
    int reserver;
    struct bufferevent *conn = fc->conn;

    if(fc->state != CLOUD_STATE_AUTHED){
        return;
    }

    header = (bcp_pm_header_t *)fc->send_buffer;
    body   = fc->send_buffer + sizeof(bcp_pm_header_t);

    len = snprintf(body, 250, "{\"octime\":%llu, \"seqnum\":%llu, \"score\":%f, \"userid\":\"%s\"}", rl->octime, rl->seqnum, rl->score, rl->userid);
    body[len] = 0;

    log_info("fdr_mp_cloud_report => report to cloud:%s\n", body);

    header->type     = htons(BCP_PMT_EVENT_REQ);
    header->seqence  = 0;
    header->session  = 0;           // keep it unchange

    reserver = len % 4;
    if(reserver > 0){
        reserver = 4 - reserver;
        memset(body+len, 0, reserver);
    }

    header->size = htons(len + reserver);
    //if(reserver > 0){
        // evbuffer_add(evb, dummy, reserver);
    //}
    bufferevent_write(conn, fc->send_buffer, sizeof(bcp_pm_header_t) + len + reserver);
}
