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

#include "fdr-logger.h"

#include "bisp_aie.h"
#include "bisp_hal.h"

#include "cJSON.h"
#include "sha256.h"

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

#define FDR_MP_URL_MAX          1024

struct fdr_mp{
    pthread_t thrd;   // thread for main event loop

    // event loop
    struct event_base* base;
    int status;

    struct event ev_idle;
    struct bufferevent *bepair[2];

    fdr_mp_local_t *local;
    fdr_mp_cloud_t *cloud;
};

typedef void (*fdr_mp_idle_func)();

typedef struct fdr_mp_idle{
    fdr_mp_idle_func  cbfn;
}fdr_mp_idle_t;

static fdr_mp_idle_t idle_funcs[] = {
    {   fdr_logger_clean        },  // clean logger recorder & image
    {   fdr_dbs_guest_clean     },  // clean guest code recorder
    {   fdr_dbs_user_clean      }   // clean user recorder & image
};

static fdr_mp_t *fdr_mp_get(){
    static fdr_mp_t mp = {};

    return &mp;
}

static void fdr_mp_idle_proc(int fd, short events, void *args);

static void mp_settimer(struct event *tmr, int seconds){
    if(seconds > 0){
        struct timeval tv = {seconds, 0};
        event_add(tmr, &tv);
    }else{
        event_del(tmr);
    }
}

void fdr_mp_report_logger(int64_t octime, int64_t seqnum, float score, char *userid, char *username){
    fdr_mp_t    *mp = fdr_mp_get();
    fdr_mp_rlog_t rl;

    rl.octime = octime;
    rl.seqnum = seqnum;
    rl.score  = score;
    if(userid == NULL || NULL == username){
        snprintf(rl.userid, FDR_DBS_USERID_SIZE+1,  "0000%012llX", octime);
        snprintf(rl.username, FDR_DBS_USERNAME_SIZE, "unkown");
    }else{
        strncpy(rl.userid, userid, FDR_DBS_USERID_SIZE);
        strncpy(rl.username, username, FDR_DBS_USERNAME_SIZE);
    }
    
    bufferevent_write(mp->bepair[0], &rl, sizeof(rl));
    //log_info("userid %s, oct:%llu, seqnum:%lld\n", rl.userid, rl.octime, rl.seqnum);
}

static void http_request_done(struct evhttp_request *req, void *arg)
{
    // FIXME : release bufferevent_socket_new handle ??
	log_info("report to manage-server ok...\n");
}

void fdr_mp_local_report(fdr_mp_t *mp, fdr_mp_rlog_t *rl){

    struct bufferevent *bev;
	struct evhttp_connection *evcon = NULL;
	struct evhttp_request *req;
	struct evkeyvalq *output_headers;
    fdr_config_t *config = fdr_config();

    char url[FDR_MP_URL_MAX];

    int retries = 1;
    int timeout = 5;
    int rc;

	bev = bufferevent_socket_new(mp->base, -1, BEV_OPT_CLOSE_ON_FREE);
    if(bev == NULL){
        return ;
    }

	evcon = evhttp_connection_base_bufferevent_new(mp->base, NULL, bev, config->mgmt_addr, config->mgmt_port);
	if (evcon == NULL) {
        bufferevent_free(bev);
		log_warn("report_logger => evhttp_connection_base_bufferevent_new() failed\n");
        return ;
	}

	evhttp_connection_set_retries(evcon, retries);
	evhttp_connection_set_timeout(evcon, timeout);
    
	// Fire off the request
	req = evhttp_request_new(http_request_done, bev);
	if (req == NULL) {
        bufferevent_free(bev);
        evhttp_connection_free(evcon);
		log_warn("report_logger => evhttp_request_new() failed\n");
        return ;
	}

	output_headers = evhttp_request_get_output_headers(req);
	evhttp_add_header(output_headers, "Connection", "close");
	evhttp_add_header(output_headers, "Content-Type", "text/json");

    snprintf(url, FDR_MP_URL_MAX, "%s:%d", config->mgmt_addr, config->mgmt_port);
    evhttp_add_header(output_headers, "host", url);

    //http://$manager-server-address:$manager-server-port/$manager-server-path?devid=$device-id&userid=xxxx&logseq=xxx&octime=1502920
    snprintf(url, FDR_MP_URL_MAX, "http://%s:%d/%s?devid=%s&octime=%lld&userid=%s&username=%s&seqnum=%lld&score=%f", 
            config->mgmt_addr, config->mgmt_port, config->mgmt_path, 
            config->device_id, rl->octime, rl->userid, rl->username, rl->seqnum, rl->score);

	rc = evhttp_make_request(evcon, req, EVHTTP_REQ_GET, url);
	if (rc != 0) {
		log_warn("report_logger => evhttp_make_request() failed\n");
        return;
	}

    return;
}

static void report_logger_cbfn(struct bufferevent *bev, void *ctx){
    fdr_mp_t *mp = (fdr_mp_t *)ctx;
    fdr_mp_rlog_t  rl;
    fdr_config_t *config = fdr_config();

    struct evbuffer *input = bufferevent_get_input(bev);

    while(evbuffer_get_length(input) >= sizeof(rl)){
        evbuffer_remove(input, &rl, sizeof(rl));
        
        log_info("userid %s, oct:%llu, seqnum:%lld, score:%f\n", rl.userid, rl.octime, rl.seqnum, rl.score);

        if(config->device_mode == 0){
            fdr_mp_local_report(mp, &rl);
        }else{
            fdr_mp_cloud_report(mp, &rl);
        }
    }
}

static void *fdr_mp_proc(void *args){
    fdr_mp_t *mp = (fdr_mp_t *)args;
    fdr_config_t *config = fdr_config();
    int rc;
    
    mp->base = event_base_new();
    if (mp->base == NULL){
        log_warn("fdr_mp_proc create event_base failed!\n");
        return (void*)-1;
    }

    rc = bufferevent_pair_new(mp->base, BEV_OPT_THREADSAFE, mp->bepair);
    if( 0 != rc ){
        log_warn("fdr ctrller thread fail:%d\n", rc);
        event_base_free(mp->base);
        return 0;
    }

    bufferevent_enable(mp->bepair[1], EV_READ);
    bufferevent_enable(mp->bepair[0], EV_WRITE);
    bufferevent_setcb(mp->bepair[1], report_logger_cbfn, NULL, NULL, mp);

    event_assign(&mp->ev_idle,   mp->base, -1, EV_TIMEOUT, fdr_mp_idle_proc, mp);
    mp_settimer(&mp->ev_idle, config->idle_run_interval);

    if(!config->activation){
        // not acitvate, start all of them;
        fdr_mp_create_cloud(mp);
        fdr_mp_create_local(mp);
    }else{
        //if(config->device_mode){
            // activated device, in cloud mode
            fdr_mp_create_cloud(mp);
        //}else{
            // activated device, in local mode
            fdr_mp_create_local(mp);
        //}
    }

    log_info("fdr_mp_proc => management plane looping ... \n");

    mp->status = 1;

    event_base_loop(mp->base, EVLOOP_NO_EXIT_ON_EMPTY);
    
    bufferevent_free(mp->bepair[0]);
    bufferevent_free(mp->bepair[1]);

    if(mp->cloud != NULL){
        fdr_mp_destroy_cloud(mp);
    }

    if(mp->local != NULL){
        fdr_mp_destroy_local(mp);
    }

    return NULL;
}

static void fdr_mp_idle_proc(int fd, short events, void *args){
    fdr_mp_t *mp = (fdr_mp_t *)args;
    fdr_config_t *config = fdr_config();
    fdr_mp_idle_t *idle;
    int i;

    mp_settimer(&mp->ev_idle, config->idle_run_interval);

    for(i = 0; i < sizeof(idle_funcs) / sizeof(fdr_mp_idle_t); i++){
        idle = &idle_funcs[i];

        idle->cbfn();
    }
}

int fdr_mp_init(){
    fdr_mp_t    *mp = fdr_mp_get();
    int rc;

    memset(mp, 0, sizeof(fdr_mp_t));

    rc = pthread_create(&mp->thrd, NULL, fdr_mp_proc, mp);
    if(rc != 0){
        log_warn("fdr_mp_create thread fail:%d\n", rc);
        return rc;
    }

    while(!mp->status){
        usleep(10);
    }

    return 0;
}


int fdr_mp_fini(){
    fdr_mp_t    *mp = fdr_mp_get();
    event_base_loopbreak(mp->base);
    return 0;
}


struct event_base *fdr_mp_getbase(fdr_mp_t *mp){
    return mp->base;
}


fdr_mp_local_t *fdr_mp_getlocal(fdr_mp_t *mp){
    return mp->local;
}

void fdr_mp_setlocal(fdr_mp_t *mp, fdr_mp_local_t *local){
    mp->local = local;
}

fdr_mp_cloud_t *fdr_mp_getcloud(fdr_mp_t *mp){
    return mp->cloud;
}

void fdr_mp_setcloud(fdr_mp_t *mp, fdr_mp_cloud_t *cloud){
    mp->cloud = cloud;
}

int fdr_mp_update_feature(const char *userid, void *buf, size_t len){
    float *feature;
    size_t size;
    int rc;

    if(!fdr_dbs_user_exist(userid)){
        log_warn("fdr_mp_update_feature => user %s not exist!\n", userid);
        return -1;
    }

    // call AI Engine
    feature = (float *)fdr_malloc(sizeof(float) * BISP_AIE_FDRVECTOR_MAX);
    if(feature == NULL){
        log_warn("fdr_mp_update_feature => alloc memory failed!\n");
        return -1;
    }

    // FIXME : 
    size = sizeof(float) * BISP_AIE_FDRVECTOR_MAX;
    rc = bisp_aie_get_fdrfeature(buf, 0, BISP_AIE_IMAGE_WIDTH, BISP_AIE_IMAGE_HEIGHT, feature, &size);
    if(rc != 0){
        log_warn("fdr_mp_update_feature =>  get image feature failed!\n");
        fdr_free(feature);
        return -1;
    }

    if(fdr_dbs_feature_lookup(userid, NULL, NULL) == 0){
        rc = fdr_dbs_feature_update(userid, feature, size);
    }else{
        rc = fdr_dbs_feature_insert(userid, feature, size);
    }
    fdr_free(feature);

    if(rc == 0){
        fdr_cp_usrdispatch(FDR_CP_EVT_USER_UPDATE, userid);
    }

    return rc;
}
