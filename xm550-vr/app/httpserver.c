/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "httpserver.h"

#include "cJSON.h"
#include "sha256.h"
#include "logger.h"
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

#include "linux/reboot.h"
#include "sys/reboot.h"

#include "config.h"
#include "vr.h"

#define UPGRADE_FILENAME    "/var/sd/upgrade/upgrade.bin"

static const char *snapshot_path = "/var/sd/snapshots";
static const char *upgrade_path = UPGRADE_FILENAME;

int b58_to_binary(const char *b58, size_t len, void *bin, size_t *outlen){
    bool rc;

    rc = b58tobin(bin, outlen, b58, len);
    if(rc){
        return 0;
    }
    
    return -1;
}

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
        free(cbuf);
    }

    evbuffer_free(evbuf);
    return 0;
}

static int check_param(cJSON *req, const char *key, const char *val){
    char *keyval;

    assert((req != NULL) && (key != NULL) && (val != NULL));

    keyval = cJSON_GetObjectString(req, key);
    if(keyval == NULL){
        log_warn("check_param =>  %s not exist\n", key);
        return HTTPSERVER_ERRCODE_INVALID;
    }

    if(strcasecmp(val, keyval) == 0){
        return 0;
    }

    log_warn("check_param =>  %s value %s, not %s\n", key, keyval, val);

    return HTTPSERVER_ERRCODE_INVALID;
}

static int evhttp_get_reqjson(struct evhttp_request *req, cJSON **reqJson){
    struct evbuffer *evbuf;
    size_t size;
    char *cbuf;
    cJSON *json;

    evbuf = evhttp_request_get_input_buffer(req);
    size  = evbuffer_get_length(evbuf);
    cbuf = malloc(size + 1);
    if(cbuf == NULL){
        log_warn("evhttp_get_reqjson => alloc buffer for json parse fail!\n");
        return HTTPSERVER_ERRCODE_NOMEM;
    }

    evbuffer_copyout(evbuf, cbuf, size);
    cbuf[size] = '\0';

    json = cJSON_Parse(cbuf);
    free(cbuf);
    if(json == NULL){
        log_warn("evhttp_get_reqjson =>  parse json fail!\n");
        return HTTPSERVER_ERRCODE_INVALID;
    }

    *reqJson = json;

    return 0;
}

static void handler_upgrade(struct evhttp_request* req, void *args){
    httpserver_t *server = (httpserver_t *)args;
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
    unsigned char csheader[32];  // SHA256
    unsigned char csbody[32];  // SHA256
    sha256_context sc;

    hikv = evhttp_request_get_input_headers(req);

    fn = evhttp_find_header(hikv, "Filename");
    if(fn == NULL){
        log_warn("handler_upgrade => without file name in header!\n");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        return ;
    }

    sequence = evhttp_find_header(hikv, "Sequence");
    if(sequence == NULL){
        log_warn("handler_upgrade => without sequence in header!\n");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        return ;
    }
    seq = atoi(sequence);
    if(seq < 0){
        log_warn("handler_upgrade => sequence wrong \n");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        return ;
    }

    cs = evhttp_find_header(hikv, "Checksum");
    if(cs == NULL){
        log_warn("handler_upgrade => without checksum in header!\n");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        return ;
    }
    cslen = 32;
    if(b58_to_binary(cs, strlen(cs), csheader, &cslen) != 0){
        log_warn("handler_upgrade => chekcsum wrong!\n");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        return ;

    }

    evbuf = evhttp_request_get_input_buffer(req);
    total = evbuffer_get_length(evbuf);

    if(total > 0){
        buf = malloc(total);
        if(buf == NULL){
            log_warn("handler_upgrade => alloc buffer fail!\n");
            send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
            return ;
        }
        evbuffer_remove(evbuf, buf, total);

        sha256_init(&sc);
        sha256_hash(&sc, buf, total);
        sha256_done(&sc, csbody);

        if(memcmp(csheader, csbody, 32) != 0){
            log_warn("handler_upgrade => checksum fail !\n");
            send_reply_params(req, NULL, HTTPSERVER_ERRCODE_CHECKSUM);
            free(buf);
            return ;
        }

        strcpy(server->image_path, upgrade_path);
        //strcat(server->image_path, fn);

        if(0 == seq) {
            f = fopen(server->image_path, "wb");
        } else {
            f = fopen(server->image_path, "rb+");
        }
        
        if(f == NULL){
            log_warn("handler_upgrade => open file fail !\n");
            send_reply_params(req, NULL, HTTPSERVER_ERRCODE_NOEXIT);
            free(buf);
            return ;
        }

        fseek(f, seq*HTTPSERVER_UPGRADE_SLICE_MAX, SEEK_SET);

        rc = fwrite(buf, 1, total, f);
        fclose(f);
        free(buf);

        if(rc != total){
            log_warn("handler_upgrade => write file fail %d!\n", rc);
            send_reply_params(req, NULL, HTTPSERVER_ERRCODE_NOEXIT);
            return ;
        }

        rc = 0;
    }

    if((total == 0) || (total < HTTPSERVER_UPGRADE_SLICE_MAX)){

        log_info("handler_upgrade => excuting\n");

        // last slice, call hwp upgrade
        // FIX: 
        //rc = fdr_upgrade(fn);
    }

    send_reply_params(req, NULL, rc);
}


static int32_t gen_snapshot_name(char *filename, uint32_t size){
    struct timeval tv;
    
    if(NULL == filename){
        log_warn("get gmtime fail\n");
        return -1;
    }
    gettimeofday(&tv, NULL);
    snprintf(filename, size - 1, "%u.%u.jpg", (uint32_t)tv.tv_sec, (uint32_t)tv.tv_usec);
    log_info("save image to : %s\r\n", filename);

    return 0;
}

static void handler_snapshot(struct evhttp_request* req, void *args){
    // httpserver_t *server = (httpserver_t *)args;
    cJSON *reqJSON;
    cJSON *rspJSON;
    char snapshot[64] = {0};
    int rc;

    rc = evhttp_get_reqjson(req, &reqJSON);
    if(rc != 0){
        log_warn("handler_snapshot => without json in body\n");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        return ;
    }

    rc = check_param(reqJSON, "action", "snapshot");
    cJSON_Delete(reqJSON);

    if(rc != 0){
        log_warn("handler_snapshot => without action command\n");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        return ;
    }

    log_info("handler_snapshot => excuting\n");
    // FIXME : snapshot & reco
    if(!gen_snapshot_name(snapshot, 64)) {
        vr_snapshot(snapshot_path, snapshot);
    }
    
    rspJSON = cJSON_CreateObject();
    if(rspJSON == NULL){
        log_warn("handler_snapshot => create rsp fail\n");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
    }
    cJSON_AddNumberToObject(rspJSON, "retcode", 0);
    cJSON_AddStringToObject(rspJSON, "snapshot", snapshot);

    send_reply_params(req, rspJSON, 0);
}


static void handler_download(struct evhttp_request* req, void *args){
    httpserver_t *server = (httpserver_t *)args;
    cJSON *reqJSON;
    const char *snapshot;
    struct evkeyvalq    *outheader;
    struct evbuffer* evb;
    int32_t fd = -1;
	struct stat st;
    int rc;

    outheader = evhttp_request_get_output_headers(req);

    rc = evhttp_get_reqjson(req, &reqJSON);
    if(rc != 0){
        log_warn("handler_download => without json in body\n");
        evhttp_add_header(outheader, "FetchResult", "fail");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        return ;
    }

    rc = check_param(reqJSON, "action", "download");
    if(rc != 0){
        log_warn("handler_download => without action command\n");
        evhttp_add_header(outheader, "FetchResult", "fail");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        cJSON_Delete(reqJSON);
        return ;
    }

    snapshot = cJSON_GetObjectString(reqJSON, "snapshot");
    if(snapshot == NULL){
        log_warn("handler_download => without snapshot\n");
        evhttp_add_header(outheader, "FetchResult", "fail");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        cJSON_Delete(reqJSON);
        return;
    }
    sprintf(server->image_path, "%s/%s", snapshot_path, snapshot);

    cJSON_Delete(reqJSON);

    log_info("handler_download =>  file : [%s]\n", server->image_path);

	if ((fd = open(server->image_path, O_RDONLY)) < 0){
        log_warn("handler_download =>  open file fail!\r\n");
        evhttp_add_header(outheader, "FetchResult", "fail");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        return ;
	}

	if (fstat(fd, &st)<0) {
        log_warn("downlhandler_downloadoad =>  open file fail!\n");
        evhttp_add_header(outheader, "FetchResult", "fail");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        close(fd);
        return ;
	}
    log_info("handler_download =>  file size: %d\n", st.st_size);

	evb = evbuffer_new();
	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/octet-stream");
	evbuffer_add_file(evb, fd, 0, st.st_size); 

    evhttp_add_header(outheader, "FetchResult", "ok");
	evhttp_send_reply(req, 200, "OK", evb);
	evbuffer_free(evb);
    close(fd);
}

static void handler_config(struct evhttp_request* req, void *args){
    // httpserver_t *server = (httpserver_t *)args;
    cJSON *reqJSON;
    int rc;

    rc = evhttp_get_reqjson(req, &reqJSON);
    if(rc != 0){
        log_warn("handler_config => without json in body\n");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        return ;
    }

    rc = check_param(reqJSON, "action", "config");
    if(rc != 0){
        log_warn("handler_config => without action command\n");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        cJSON_Delete(reqJSON);
        return ;
    }

    if(config_json_set_value(reqJSON)) {
        log_warn("handler_config => without config\n");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        cJSON_Delete(reqJSON);
        return;
    }
    config_commit();
    log_info("handler_config ok\r\n");
    send_reply_params(req, NULL, 0);
    cJSON_Delete(reqJSON);
}

static void system_reboot(void){
    sync();
    reboot(LINUX_REBOOT_CMD_RESTART);
}

static void handler_reboot(struct evhttp_request* req, void *args){
    // httpserver_t *server = (httpserver_t *)args;
    cJSON *reqJSON;
    int rc;

    rc = evhttp_get_reqjson(req, &reqJSON);
    if(rc != 0){
        log_warn("handler_reboot => without json in body\n");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        return ;
    }

    rc = check_param(reqJSON, "action", "reboot");
    if(rc != 0){
        log_warn("handler_reboot => without action command\n");
        send_reply_params(req, NULL, HTTPSERVER_ERRCODE_INVALID);
        cJSON_Delete(reqJSON);
        return ;
    }
    cJSON_Delete(reqJSON);

    log_info("handler_reboot =>  call system reboot \n");
    
    send_reply_params(req, NULL, 0);
    // FIXME : reboot system
    system_reboot();
}

static void handler_unkown(struct evhttp_request* req, void *args){
    const struct evhttp_uri  *uri;

    uri = evhttp_request_get_evhttp_uri(req);
    const char *path = evhttp_uri_get_path(uri);

    log_info("fdrserver path :%s access invalid\n", path);

    send_reply_params(req, NULL, -100);
}

static void *httpserver_proc(void *args){
    httpserver_t *server = (httpserver_t *)args;
    
    server->base = event_base_new();
    if (server->base == NULL)
    {
        log_warn("httpserver_proc => create event_base failed!\n");
        return (void*)-1;
    }

    server->httpserver = evhttp_new(server->base);
    if (server->httpserver == NULL)
    {
        log_warn("httpserver_proc => thread create evhttp failed!\n");
        event_base_free(server->base);
        return (void*)-1;
    }

    while (evhttp_bind_socket(server->httpserver, "0.0.0.0", 8080) != 0) {
        log_warn("httpserver_proc => thread bind address failed!\n");
        //evhttp_free(server->httpserver);
        //event_base_free(server->base);
        //return (void*)-1;
        sleep(2);
    }

    evhttp_set_cb(server->httpserver, "/api/upgrade",  handler_upgrade,  server);
    evhttp_set_cb(server->httpserver, "/api/snapshot", handler_snapshot, server);
    evhttp_set_cb(server->httpserver, "/api/download", handler_download, server);
    evhttp_set_cb(server->httpserver, "/api/config",   handler_config, server);
    evhttp_set_cb(server->httpserver, "/api/reboot",   handler_reboot, server);

    evhttp_set_gencb(server->httpserver, handler_unkown, server);

    log_info("httpserver_proc => loop running ... \n");

    event_base_loop(server->base, EVLOOP_NO_EXIT_ON_EMPTY);
    
    log_info("httpserver_proc => loop exit ... \n");
    
    evhttp_free(server->httpserver);
    event_base_free(server->base);

    return NULL;
}

int httpserver_init(httpserver_t *server){
    int rc;

    memset(server, 0, sizeof(httpserver_t));

    rc = pthread_create(&server->thrd, NULL, httpserver_proc, server);
    if(rc != 0){
        log_warn("httpserver_init => create fdr server thread fail:%d\n", rc);
        return rc;
    }

    return 0;
}


int httpserver_fini(httpserver_t *server){
    event_base_loopbreak(server->base);
    return 0;
}

