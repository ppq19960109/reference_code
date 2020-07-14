/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr-mp.h"
#include "fdr-cp.h"
#include "fdr.h"

#include "fdr-handler.h"
#include "fdr-dbs.h"
#include "fdr-path.h"
#include "fdr-qrcode.h"

#include "bisp_aie.h"
#include "bisp_hal.h"

#include "logger.h"

#include "cJSON.h"
#include "sha256.h"
#include "base58.h"

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event_struct.h>
#include <event2/thread.h>
#include <event2/util.h>

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

#define FDR_MP_LOCAL_ADDR_MAX           64
#define FDR_MP_LOCAL_URL_MAX            1024
#define FDR_FILE_BUFFER                 (1024*8)
#define FDR_MP_LOCAL_PATH_MAX           128
#define FDR_MP_LOCAL_FILE_BUFFER        (16*1024)
#define FDR_MP_LOCAL_TOKEN              32

static const char *image_path       = CAPTURE_IMAGE_PATH;
static const char *user_image_path  = USER_IMAGE_PATH;
static const char *upgrade_path     = UPGRADE_PACKAGE_PATH;

#define FDR_UPGRADE_SLICE_MAX           (1024*128)

struct fdr_mp_local{
    fdr_mp_t *mp;

    struct evhttp   *httpserver;

    char image_path[FDR_MP_LOCAL_PATH_MAX];

    char token[FDR_MP_LOCAL_TOKEN];
};


typedef int (*server_handler)(cJSON *req, cJSON *rsp);
typedef struct fdr_server_router{
    char *api_path;  
    server_handler  handler;
    fdr_mp_local_t  *fs;
}fdr_server_router_t;

// static global variable
static fdr_server_router_t router_handlers[] = {
    {"/api/info",       handler_info,        NULL    },
    {"/api/config",     handler_config,      NULL    },
    {"/api/user",       handler_user,        NULL    },
    {"/api/guest",      handler_guest,       NULL    },
    {"/api/restart",    handler_restart,     NULL    },
    {"/api/logger",     handler_logger,      NULL    }
};

static int send_reply_params(struct evhttp_request* req, cJSON *rsp, int errcode){
    struct evbuffer* evbuf = evbuffer_new();
    char *cbuf;

    if (evbuf == NULL){
        log_warn("fdr server create evbuffer failed!\n");
        return -1;
    }

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");

    if(rsp == NULL){
        evbuffer_add_printf(evbuf, "{\"retcode\": %d}\n", errcode);
        evhttp_send_reply(req, HTTP_OK, "OK", evbuf);
    }else{
        if(!cJSON_HasObjectItem(rsp, "retcode"))
            cJSON_AddNumberToObject(rsp, "retcode", errcode);

        cbuf = cJSON_Print(rsp);
        cJSON_Delete(rsp);

        evbuffer_add(evbuf, cbuf, strlen(cbuf));
        evhttp_send_reply(req, HTTP_OK, "OK", evbuf);
        free(cbuf); // buffer alloced by cJSON_Print
    }

    evbuffer_free(evbuf);
    return 0;
}


static void httpserver_handler_unkown(struct evhttp_request* req, void *args){
    const struct evhttp_uri  *uri;

    uri = evhttp_request_get_evhttp_uri(req);
    const char *path = evhttp_uri_get_path(uri);

    log_info("httpserver_handler_unkown => path :%s access invalid\n", path);

    send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
}

static int evhttp_get_reqjson(struct evhttp_request *req, cJSON **reqJson){
    struct evbuffer *evbuf;
    size_t size;
    char *cbuf;
    cJSON *json;

    evbuf = evhttp_request_get_input_buffer(req);
    size  = evbuffer_get_length(evbuf);
    cbuf = fdr_malloc(size + 1);
    if(cbuf == NULL){
        log_warn("evhttp_get_reqjson => alloc buffer for json parse fail!\n");
        return FDRSDK_ERRCODE_NOMEM;
    }

    evbuffer_copyout(evbuf, cbuf, size);
    cbuf[size] = '\0';

    json = cJSON_Parse(cbuf);
    fdr_free(cbuf);
    if(json == NULL){
        log_warn("evhttp_get_reqjson =>  parse json fail!\n");
        return FDRSDK_ERRCODE_INVALID;
    }

    *reqJson = json;

    return 0;
}

static int check_param(cJSON *req, const char *key, const char *val){
    char *keyval;

    assert((req != NULL) && (key != NULL) && (val != NULL));

    keyval = cJSON_GetObjectString(req, key);
    if(keyval == NULL){
        log_warn("check_param =>  %s not exist\n", key);
        return FDRSDK_ERRCODE_INVALID;
    }

    if(strcasecmp(val, keyval) == 0){
        return 0;
    }

    log_warn("check_param =>  %s value %s, not %s\n", key, keyval, val);

    return FDRSDK_ERRCODE_INVALID;
}

static int check_token(fdr_mp_local_t *fs, struct evhttp_request *req){
    struct evkeyvalq * hikv;
    const char *token;

    hikv = evhttp_request_get_input_headers(req);

    // check http header token
    token = evhttp_find_header(hikv, "Token");
    if(token == NULL){
        log_warn("check_token => request without token\n");
        return FDRSDK_ERRCODE_TOKEN;
    }

    if(strcasecmp(token, fs->token) != 0){
        log_warn("check_token => request token:%s  != %s\n", token, fs->token);
        return FDRSDK_ERRCODE_TOKEN;
    }

    return 0;
}


static int check_header(struct evhttp_request *req, const char *key, const char *val){
    struct evkeyvalq * hikv;
    const char *keyval;

    assert((req != NULL) && (key != NULL)  && (val != NULL));

    hikv = evhttp_request_get_input_headers(req);

    // check http header token
    keyval = evhttp_find_header(hikv, key);
    if((keyval != NULL) && (strcasecmp(keyval, val) == 0)){
        return 0;
    }

    //log_warn("check_header =>  val %s  != %s\n", val, keyval == NULL ? "notexist" : keyval);
    return FDRSDK_ERRCODE_INVALID;
}

typedef struct fdr_httpheader_keyval {
    char *key;
    char *val;
}fdr_httpheader_keyval_t;

static fdr_httpheader_keyval_t upload_key_check[] = {
    {   "Content-Type",         "application/image"    },
    {   "Content-Length",       NULL            },
    {   "User-ID",              NULL            },
    {   "Upload",               NULL            }
};

static void httpserver_handler_upload(struct evhttp_request* req, void *args){
    fdr_mp_local_t *fs = (fdr_mp_local_t *)args;
    struct evbuffer* evbuf;
    const char *temp;
    size_t size, total;
    int rc = -1;
    struct evkeyvalq * hikv;
    const char *userid;
    FILE *fimg;
    int i;
    char *buf;
    float *feature;
    int add_feature = 0;

    hikv = evhttp_request_get_input_headers(req);

    rc = check_token(fs, req);
    if(rc != 0){
        log_warn("upload => token  not right\n");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_TOKEN);
        return ;
    }

    for(i = 0; i < sizeof(upload_key_check)/sizeof(fdr_httpheader_keyval_t); i++){
        temp = evhttp_find_header(hikv, upload_key_check[i].key);
        if((upload_key_check[i].val != NULL) && (strcasecmp(temp, upload_key_check[i].val) != 0)){
            log_warn("upload => header is invalid!\n");
            send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
            return ;
        }
    }

    // check user id valid
    userid = evhttp_find_header(hikv, "User-ID");
    if(userid == NULL){
        log_warn("upload => without user id in header!\n");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        return ;
    }
    rc = fdr_dbs_user_exist(userid);
    if(rc == 0){
        log_warn("upload => userid:%s not exist!\n", userid);
        send_reply_params(req, NULL, FDRSDK_ERRCODE_NOEXIT);
        return ;
    }


    // write image to file
    evbuf = evhttp_request_get_input_buffer(req);
    total = evbuffer_get_length(evbuf);
    buf = fdr_malloc(total);
    if(buf == NULL){
        log_warn("upload => alloc buffer fail!\n");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_NOEXIT);
        return ;
    }

    strcpy(fs->image_path, user_image_path);
    strcat(fs->image_path, userid);
    
    temp = evhttp_find_header(hikv, "upload");
    if(temp == NULL){
        log_warn("upload => without Upload!\n");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        fdr_free(buf);
        return ;
    }

    if(check_header(req, "upload", "user-image") == 0){
        strcat(fs->image_path, ".nv12");
        add_feature = FDR_DBS_USER_STATUS_FEATURE;
    }else if(check_header(req, "upload", "ui-image") == 0){
        strcat(fs->image_path, ".bmp");
        add_feature = FDR_DBS_USER_STATUS_UIIMAGE;
    }else{
        log_warn("upload => unkown image type:%s !\n", temp);
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        fdr_free(buf);
        return ;
    }

    fimg = fopen(fs->image_path, "w");
    if(fimg == NULL){
        log_warn("upload => open image file fail:%s!\n", fs->image_path);
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        fdr_free(buf);
        return ;
    }
    
    evbuffer_remove(evbuf, buf, total);
    fwrite(buf, 1, total, fimg);

    fclose(fimg);

    if(add_feature & FDR_DBS_USER_STATUS_FEATURE){
        // call AI Engine
        feature = (float *)fdr_malloc(sizeof(float) * BISP_AIE_FDRVECTOR_MAX);
        if(feature == NULL){
            log_warn("upload => alloc memory failed!\n");
            send_reply_params(req, NULL, FDRSDK_ERRCODE_NOMEM);
            fdr_free(buf);
            return ;
        }

        // FIXME : 
        size = sizeof(float) * BISP_AIE_FDRVECTOR_MAX;
        rc = bisp_aie_get_fdrfeature(buf, 0, BISP_AIE_IMAGE_WIDTH, BISP_AIE_IMAGE_HEIGHT, feature, &size);
        fdr_free(buf);

        if(rc != 0){
            log_warn("upload =>  get image feature failed!\n");
            send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID_FACE);
            fdr_free(feature);
            return ;
        }

        if(fdr_dbs_feature_lookup(userid, NULL, NULL) == 0){
            rc = fdr_dbs_feature_update(userid, feature, size);
            fdr_cp_usrdispatch(FDR_CP_EVT_USER_UPDATE, userid);
        }else{
            rc = fdr_dbs_feature_insert(userid, feature, size);
            if(rc == 0){
                fdr_dbs_user_setstatus(userid, FDR_DBS_USER_STATUS_FEATURE);
                fdr_cp_usrdispatch(FDR_CP_EVT_USER_UPDATE, userid);
            }
        }
        fdr_free(feature);
        if(rc != 0){
            log_warn("upload => update feature failed!\n");
            send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
            return ;
        }
        
    }else if (add_feature & FDR_DBS_USER_STATUS_UIIMAGE){
        fdr_free(buf);
        fdr_dbs_user_setstatus (userid, FDR_DBS_USER_STATUS_UIIMAGE);
        //fdr_cp_usrdispatch(FDR_CP_EVT_USER_UPDATE, userid);
    }else{
        fdr_free(buf);
    }
    
    send_reply_params(req, NULL, 0);
}

static void httpserver_handler_upgrade(struct evhttp_request* req, void *args){
    fdr_mp_local_t *fs = (fdr_mp_local_t *)args;
    struct evbuffer* evbuf;
    int rc = -1;
    struct evkeyvalq * hikv;
    int total;
    FILE *f;
    unsigned char *buf;
    const char *fn;
    const char *sequence;
    int seq;
    const char *cs;
    size_t cslen;
    unsigned char csheader[SHA256_HASH_BYTES];  // SHA256
    unsigned char csbody[SHA256_HASH_BYTES];  // SHA256
    sha256_context sc;

    rc = check_token(fs, req);
    if(rc != 0){
        log_warn("upgrade => token not right\n");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_TOKEN);
        return ;
    }

    hikv = evhttp_request_get_input_headers(req);
    fn = evhttp_find_header(hikv, "Filename");
    if(fn == NULL){
        log_warn("upgrade =>  without file name in header!\n");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        return ;
    }

    sequence = evhttp_find_header(hikv, "Sequence");
    if(sequence == NULL){
        log_warn("upgrade =>  without sequence in header!\n");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        return ;
    }
    seq = atoi(sequence);
    if(seq < 0){
        log_warn("upgrade =>  sequence wrong \n");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        return ;
    }

    cs = evhttp_find_header(hikv, "Checksum");
    if(cs == NULL){
        log_warn("upgrade =>  without checksum in header!\n");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        return ;
    }
    cslen = SHA256_HASH_BYTES;
    cslen = base58_decode(cs, strlen(cs), csheader, cslen);
    //if(b58_to_binary(cs, strlen(cs), csheader, &cslen) != 0){
    if(cslen <= 0){
        log_warn("upgrade =>  chekcsum wrong!\n");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        return ;

    }

    evbuf = evhttp_request_get_input_buffer(req);
    total = evbuffer_get_length(evbuf);

    if(total > 0){
        buf = fdr_malloc(total);
        if(buf == NULL){
            log_warn("handler_upgrade alloc buffer fail!\n");
            send_reply_params(req, NULL, FDRSDK_ERRCODE_NOMEM);
            return ;
        }
        evbuffer_remove(evbuf, buf, total);

        sha256_init(&sc);
        sha256_hash(&sc, buf, total);
        sha256_done(&sc, csbody);

        if(memcmp(csheader, csbody, 32) != 0){
            fdr_free(buf);
            log_warn("upgrade =>  checksum fail !\n");
            send_reply_params(req, NULL, FDRSDK_ERRCODE_CHECKSUM);
            return ;
        }

        strcpy(fs->image_path, upgrade_path);
        strcat(fs->image_path, fn);
        f = fopen(fs->image_path, "a");
        if(f == NULL){
            fdr_free(buf);
            log_warn("handler_upgrade open file fail: %s !\n", fs->image_path);
            send_reply_params(req, NULL, FDRSDK_ERRCODE_NOEXIT);
            return ;
        }

        fseek(f, seq*FDR_UPGRADE_SLICE_MAX, SEEK_SET);

        rc = fwrite(buf, 1, total, f);
        fclose(f);
        fdr_free(buf);

        if(rc != total){
            log_warn("handler_upgrade write file fail %d!\n", rc);
            send_reply_params(req, NULL, FDRSDK_ERRCODE_NOEXIT);
            return ;
        }

        rc = 0;
    }

    if((total == 0) || (total < FDR_UPGRADE_SLICE_MAX)){
        // last slice, call hwp upgrade
        // FIXME: 
        //rc = fdr_upgrade(fn);
    }

    send_reply_params(req, NULL, rc);
}

static void httpserver_handler_download(struct evhttp_request* req, void *args){
    fdr_mp_local_t *fs = (fdr_mp_local_t *)args;
    struct evbuffer* evb;
    struct evkeyvalq    *outheader;
    cJSON *reqJSON;
    const char *timestamp; 
    const char *userid;
    int64_t octime ; 
    time_t tim; 
    struct tm t;
    int fd;
	struct stat st;
    int rc;

    outheader = evhttp_request_get_output_headers(req);

    rc = check_token(fs, req);
    if(rc != 0){
        log_warn("download => request token failed\n");
        evhttp_add_header(outheader, "FetchResult", "fail");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_TOKEN);
        return ;
    }

    rc = evhttp_get_reqjson(req, &reqJSON);
    if(rc != 0){
        log_warn("download => without json in body\n");
        evhttp_add_header(outheader, "FetchResult", "fail");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        return ;
    }

    rc = check_param(reqJSON, "action", "download");
    if(rc != 0){
        log_warn("download => without action command\n");
        evhttp_add_header(outheader, "FetchResult", "fail");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        cJSON_Delete(reqJSON);
        return ;
    }

    if(check_param(reqJSON, "image", "user") == 0){
        userid = cJSON_GetObjectString(reqJSON, "userid");
        if(userid == NULL){
            log_warn("download => without userid\n");
            evhttp_add_header(outheader, "FetchResult", "fail");
            send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
            cJSON_Delete(reqJSON);
            return;
        }
        sprintf(fs->image_path, "%s/%s.nv12", user_image_path, userid);
    }else if (check_param(reqJSON, "image", "logger") == 0){
        timestamp = cJSON_GetObjectString(reqJSON, "timestamp");
        if(timestamp == NULL){
            log_warn("download => without timestamp\n");
            evhttp_add_header(outheader, "FetchResult", "fail");
            send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
            cJSON_Delete(reqJSON);
            return;
        }

        octime = atoll(timestamp);    
        tim = (octime / 1000);

        if(localtime_r(&tim, &t) == NULL){
            log_warn("download => get gmtime fail\n");
            evhttp_add_header(outheader, "FetchResult", "fail");
            send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
            cJSON_Delete(reqJSON);
            return ;
        }
    
        sprintf(fs->image_path, "%s/%04d-%02d-%02d/%02d-%02d-%02d.%03lu.jpg", image_path, t.tm_year+1900, t.tm_mon+1, t.tm_mday,
                                    t.tm_hour, t.tm_min, t.tm_sec, (unsigned long)(octime%1000));
    }else{
        log_warn("download => unkown image\n");
        evhttp_add_header(outheader, "FetchResult", "fail");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        cJSON_Delete(reqJSON);
        return ;
    }
    
    cJSON_Delete(reqJSON);

    log_info("download =>  file : %s\n", fs->image_path);
    
    
	if ((fd = open(fs->image_path, O_RDONLY)) < 0){
        log_warn("download =>  open file fail!\n");
        evhttp_add_header(outheader, "FetchResult", "fail");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        return ;
	}

	if (fstat(fd, &st)<0) {
        log_warn("download =>  open file fail!\n");
        evhttp_add_header(outheader, "FetchResult", "fail");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        close(fd);
        return ;
	}

	evb = evbuffer_new();
	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/image");
	evbuffer_add_file(evb, fd, 0, st.st_size); 

    evhttp_add_header(outheader, "FetchResult", "ok");
	evhttp_send_reply(req, 200, "OK", evb);
	evbuffer_free(evb);
    close(fd);
}


static void httpserver_handler(struct evhttp_request* req, void *args){
    fdr_server_router_t  *fsr = (fdr_server_router_t  *)args;
    cJSON *reqJSON, *rspJSON; 
    int rc;

    log_info("httpserver_handler => %ld : from %s request for %s\n", time(NULL), evhttp_request_get_host(req), evhttp_request_get_uri(req));

    rc = evhttp_get_reqjson(req, &reqJSON);
    if(rc != 0){
        send_reply_params(req, NULL, rc);
        return ;
    }
    
    if(strcmp(fsr->api_path, "/api/info") != 0){
        rc = check_token(fsr->fs, req);
        if(rc != 0){
            log_warn("httpserver_handler => call token fail\n");
            send_reply_params(req, NULL, rc);
            cJSON_Delete(reqJSON);
            return ;
        }
    }

    rspJSON = cJSON_CreateObject();
    if(rspJSON == NULL){
        log_warn("httpserver_handler => create response cjson object fail!\n");
        cJSON_Delete(reqJSON);
        send_reply_params(req, NULL, FDRSDK_ERRCODE_NOMEM);
        return ;
    }
    
    rc = fsr->handler(reqJSON, rspJSON);


    cJSON_Delete(reqJSON);
    if(rc != 0){
        log_warn("httpserver_handler => call handler fail : %d\n", rc);
    }

    send_reply_params(req, rspJSON, rc);
    return ;
}

static void httpserver_handler_authen(struct evhttp_request* req, void *args){
    fdr_mp_local_t *fs = (fdr_mp_local_t *)args;
    struct evbuffer *evbuf;
    const char *action;
    const char *passwd;
    const char *newpasswd;
    const char *signature;
    uint8_t  sign[ECC_BYTES*2];
    size_t sign_len = ECC_BYTES*2;
    char *activate;
    char *dbpass;

    sha256_context sc;
    uint8_t shain[128];
    int64_t shaout[4];  // 32Bytes

    cJSON *temp;
    cJSON *reqJSON;
    time_t ts;
    int rc;

    rc = evhttp_get_reqjson(req, &reqJSON);
    if(rc != 0){
        send_reply_params(req, NULL, rc);
        return ;
    }

    // get common item of request json
    action = cJSON_GetObjectString(reqJSON, "action");
    if(action == NULL){
        cJSON_Delete(reqJSON);
        send_reply_params(req, NULL, rc);
        return ;
    }

    rc = check_param(reqJSON, "user", "admin");
    if(rc != 0){
        cJSON_Delete(reqJSON);
        send_reply_params(req, NULL, rc);
        return ;
    }

    passwd = cJSON_GetObjectString(reqJSON, "passwd");
    if(passwd == NULL){
        cJSON_Delete(reqJSON);
        send_reply_params(req, NULL, rc);
        return ;
    }

    // 1. activate the device
    if(fdr_dbs_config_get("activation", &activate) != 0){
        log_warn("handler_authen =>  get activate fail\n");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_DBOPS);
        return ;
    }

    if(strcmp(activate, "0") == 0){
        fdr_free(activate);
        activate = NULL;

        if(strcasecmp(action, "activate") != 0){
            log_warn("handler_authen =>  get activate fail\n");
            send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
            return ;
        }

        // verify
        temp = cJSON_GetObjectItem(reqJSON, "timestamp");
        if(temp == NULL){
            log_warn("handler_authen =>  without timestamp\n");
            send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
            return ;
        }
        ts = temp->valueint;
        signature = cJSON_GetObjectString(reqJSON, "signature");
        if(signature == NULL){
            log_warn("handler_authen =>  without timestamp\n");
            send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
            return ;
        }
        
        sign_len = base58_decode(signature, strlen(signature), sign, sign_len);
        //if(!b58tobin(sign, &sign_len, signature, strlen(signature))){
        if(sign_len <= 0){
            log_warn("handler_authen =>  b58 to bin fail\n");
            send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
            return ;
        }

        if(fdr_verify(passwd, strlen(passwd), ts, sign) != 0){
            log_warn("handler_authen =>  verify fail\n");
            send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
            return ;
        }

        fdr_dbs_config_set("admin", passwd);
        fdr_dbs_config_set("activation", "1");
        fdr_dbs_config_set("device-mode", "0");
        send_reply_params(req, NULL, 0);

        // FIXME : reboot or close cloud connections

        return ;
    }
    fdr_free(activate);
    activate = NULL;

    // 2. authen & passwd & reset
    if(fdr_dbs_config_get("admin", &dbpass) != 0){
        log_warn("handler_authen =>  get passwd fail\n");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_DBOPS);
        return ;
    }

    if(strcmp(passwd, dbpass) != 0){
        log_warn("handler_authen =>  cmp passwd fail\n");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
        fdr_free(dbpass);
        return ;
    }

    // 2.1 authen
    if(strcasecmp(action, "authen") == 0){

        snprintf((char*)shain, 128, "admin-%ld-%s-%ld", random(), passwd, time(NULL));
        sha256_init(&sc);
        sha256_hash(&sc, shain, 128);
        sha256_done(&sc, (uint8_t*)shaout);
        snprintf(fs->token, 32, "%llx%llx%llx%llx", shaout[0], shaout[1], shaout[2], shaout[3]);

        goto exit_normal;
    }

    // 2.2 passwd
    if(strcasecmp(action, "passwd") == 0){
        newpasswd = cJSON_GetObjectString(reqJSON, "newpasswd");
        if(newpasswd != 0){
            cJSON_Delete(reqJSON);
            fdr_free(dbpass);
            send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
            return ;
        }

        fdr_dbs_config_set("admin", newpasswd);
        goto exit_normal;
    }

    // 2.3 reset
    if(strcasecmp(action, "reset") == 0){
        // FIXME : clean all data

        #if 0
        // clear dbs.sqlite
        fdr_util_rm(DBS_SQLITE_FILENAME);

        // clear feature.sqlite
        // delete features

        // clear logger.sqlite
        fdr_util_rm(LOG_SQLITE_FILENAME);

        // delete users directory
        fdr_util_rm(USER_IMAGE_PATH);

        // delete image directory
        fdr_util_rm(CAPTURE_IMAGE_PATH);

        // delete upgrade packages
        fdr_util_rm(UPGRADE_PACKAGE_PATH);
        #endif

        cJSON_Delete(reqJSON);
        fdr_free(dbpass);
        send_reply_params(req, NULL, 0);
        return ;
    }

    // 3. others
    cJSON_Delete(reqJSON);
    fdr_free(dbpass);
    send_reply_params(req, NULL, FDRSDK_ERRCODE_INVALID);
    return ;

exit_normal:
    cJSON_Delete(reqJSON);
    fdr_free(dbpass);

    // response to client
    evbuf = evbuffer_new();
    if (!evbuf){
        log_warn("handler_authen => create evbuffer failed!\n");
        send_reply_params(req, NULL, FDRSDK_ERRCODE_NOMEM);
        return ;
    }

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer_add_printf(evbuf, "{\"retcode\": %d, \"Token\":\"%s\" }\n", 0, fs->token);
    evhttp_send_reply(req, HTTP_OK, "OK", evbuf);
    evbuffer_free(evbuf);

    return ;
}


int fdr_mp_create_local(fdr_mp_t *mp){
    fdr_mp_local_t *fs;
    fdr_server_router_t  *fsr;
    int i;

    fs = (fdr_mp_local_t*)fdr_malloc(sizeof(fdr_mp_local_t));
    if(fs == NULL){
        log_warn("fdr_mp_create_server => alloc memory failed!\n");
        return -1;
    }
    memset(fs, 0, sizeof(fdr_mp_local_t));
    fs->mp = mp;
    
    fs->httpserver = evhttp_new(fdr_mp_getbase(mp));
    if (fs->httpserver == NULL){
        log_warn("fdr_mp_create_server => create evhttp failed!\n");
        fdr_free(fs);
        return -1;
    }

    if (evhttp_bind_socket(fs->httpserver, "0.0.0.0", 8080) != 0){
        log_warn("fdr server thread bind address failed!\n");
        evhttp_free(fs->httpserver);
        fdr_free(fs);
        return -1;
    }

    for(i = 0; i < sizeof(router_handlers)/sizeof(fdr_server_router_t); i++){
        fsr = &router_handlers[i];
        if(fsr->api_path != NULL){
            log_info("register api handler %d => %s\n", i, fsr->api_path);

            fsr->fs = fs;
            evhttp_set_cb(fs->httpserver, fsr->api_path, httpserver_handler, fsr);
        }else{
            break;
        }
    }

    evhttp_set_cb(fs->httpserver, "/api/upload",    httpserver_handler_upload, fs);
    evhttp_set_cb(fs->httpserver, "/api/upgrade",   httpserver_handler_upgrade, fs);
    evhttp_set_cb(fs->httpserver, "/api/authen",    httpserver_handler_authen, fs);
    evhttp_set_cb(fs->httpserver, "/api/download",  httpserver_handler_download, fs);
    
    evhttp_set_gencb(fs->httpserver, httpserver_handler_unkown, fs);

    fdr_mp_setlocal(mp, fs);

    return 0;
}

int fdr_mp_destroy_local(fdr_mp_t *mp){
    fdr_mp_local_t *fs = fdr_mp_getlocal(mp);

    if(fs == NULL)
        return 0;

    evhttp_free(fs->httpserver);
    fdr_free(fs);

    fdr_mp_setlocal(mp, NULL);

    return 0;
}

