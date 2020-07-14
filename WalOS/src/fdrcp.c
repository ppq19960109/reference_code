/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */
 
#include "fdrcp.h"
#include "fdrcp-utils.h"

#include "lvgui.h"

#include "bisp.h"


#include "logger.h"

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
#include <pthread.h>
#include <arpa/inet.h>

/* On OSX spinlock = mutex */
#ifdef MACOSX_EMU
    #define pthread_spinlock_t      pthread_mutex_t

    #define pthread_spin_lock       pthread_mutex_lock
    #define pthread_spin_unlock     pthread_mutex_unlock

    #define pthread_spin_init(__lock,__share)       pthread_mutex_init(__lock, NULL)
    #define pthread_spin_destroy    pthread_mutex_destroy
#endif

#define FDRCP_DATA_BUF_MAX          16
#define FDRCP_USER_BUF_MAX          16
#define FDRCP_VCODE_BUF_MAX         16

#define FDRCP_STRANGE_MAX           256
#define FDRCP_STRANGE_TIPS          (1000*2)

#define FDRCP_VOICE_TYPE_USER       0
#define FDRCP_VOICE_TYPE_VISITOR    1

#define FDRCP_EVENT_ADDR            "127.0.0.1"
#define FDRCP_EVENT_PORT            6544
#define FDRCP_EVENT_STRANGE_UID     "0000-STRANGE-ID"
#define FDRCP_EVENT_STRANGE_UNAME   "0000-STRANGE-NAME"

typedef struct {
    int64_t timestamp;
    int tfid;
    int logr;
}fdrcp_track_t;

enum {
    FDRCP_STATE_NONE = 0,
    FDRCP_STATE_INIT,
    FDRCP_STATE_RECO,
    FDRCP_STATE_QRCODE
};

typedef struct {
    int64_t octime;

    int64_t last;

    char uid[FDRCP_USRID_MAX];
    char uname[FDRCP_UNAME_MAX];

    int x;
    int y;
    int w;
    int h;

    float score;
    float sharp;
    float therm;

}fdrcp_logr_t;

// controll plane instance
typedef struct {
    // thread for main event loop
    pthread_t thread;

    pthread_spinlock_t lock;
    struct list_head free_buffers;  // fdrcp_data_t
    struct list_head free_users;    // fdrcp_user_t
    struct list_head free_vcode;    // fdrcp_vcode_t

    // user repo
    int user_max;
    int user_cur;
    fdrcp_user_t *users;

    // visitor code repo
    int vcode_max;
    int vcode_cur;
    fdrcp_vcode_t *vcodes;

    // user cache
    struct list_head lrus;
    struct list_head rpus;  // user in repo

    // last
    int64_t last_ts;

    // strange track
    int strange_max;
    fdrcp_track_t stranges[FDRCP_STRANGE_MAX];

    // event loop
    struct event_base* base;
    int status;

    fdrcp_logr_t logr;

    // event timer
    struct event ev_state;
    struct event ev_logr;

    struct event ev_relay;
    struct event ev_rs485;

    // struct event *ev_disc;

    // key state
    time_t key_down;

    // communication with walnuts luascripts
    evutil_socket_t udpsock;

    // comunication with other thread
    struct bufferevent *bepair[2];
}fdrcp_t;

typedef struct {
    int type;

    union{
        void *data;
        char *qrcode;
        int key;
        float therm;
        int64_t timestamp;
    };

}fdrcp_evmsg_t;

static void fdrcp_settimer(struct event *ev, int seconds, int useconds);

static void  fdrcp_event_handler(struct bufferevent *bev, void *ctx);
static float fdrcp_xma_match(float *feature1, float *feature2, int len);

static int fdrcp_user_append(fdrcp_t *cp, fdrcp_user_t *user);
static int fdrcp_user_update(fdrcp_t *cp, fdrcp_user_t *user);
static int fdrcp_user_delete(fdrcp_t *cp, fdrcp_user_t *user);

static int fdrcp_vcode_match(fdrcp_t *cp, const char *qrcode);
static int fdrcp_vcode_delete(fdrcp_t *cp, fdrcp_vcode_t *vcode);
static int fdrcp_vcode_append(fdrcp_t *cp, fdrcp_vcode_t *vcode);

static int fdrcp_rule_match(fdrcp_t *cp, int rule);
static int fdrcp_rule_append(fdrcp_t *cp, fdrcp_rule_t *rule);
static int fdrcp_rule_delete(fdrcp_t *cp, fdrcp_rule_t *rule);

static int   fdrcp_match(fdrcp_t *cp, float *feature, float thresh_hold, float *score);
static float fdrcp_xma_match(float *feature1, float *feature2, int len);

static void fdrcp_logr_set(fdrcp_t *fcp, int64_t oct, fdrcp_fd_t *fd, fdrcp_user_t *user, float score);
static void fdrcp_logr_report(fdrcp_t *fcp);

static fdrcp_t *fdrcp_get(){
    static fdrcp_t cp = {};

    return &cp;
}

fdrcp_data_t *fdrcp_acquire(){
    fdrcp_t *cp = fdrcp_get();
    fdrcp_data_t *buff = NULL;

    pthread_spin_lock(&cp->lock);

    if(!list_empty(&cp->free_buffers)){
        buff = list_first_entry(&cp->free_buffers, fdrcp_data_t, node);
        list_del(&buff->node);
        INIT_LIST_HEAD(&buff->node);
    }

    pthread_spin_unlock(&cp->lock);

    return buff;
}

void fdrcp_release(fdrcp_data_t *buff){
    fdrcp_t *cp = fdrcp_get();

    pthread_spin_lock(&cp->lock);

    INIT_LIST_HEAD(&buff->node);
    list_add_tail(&buff->node, &cp->free_buffers);

    pthread_spin_unlock(&cp->lock);
}

static int fdrcp_buffer_init(fdrcp_t *cp, int num){
    fdrcp_data_t *buff = NULL;
    int i;

    for(i = 0; i < num; i++){
        buff = (fdrcp_data_t*)fdrcp_malloc(sizeof(fdrcp_data_t));
        if(buff == NULL){
            log_warn("fdrcp_buffer_init => alloc memory fail\n");
            return -1;
        }
        memset(buff, 0, sizeof(fdrcp_data_t));

        INIT_LIST_HEAD(&buff->node);
        list_add(&buff->node, &cp->free_buffers);
    }

    return 0;
}

static void fdrcp_buffer_fini(fdrcp_t *cp){
    fdrcp_data_t *buff = NULL;

    while(!list_empty(&cp->free_buffers)){
        buff = list_first_entry(&cp->free_buffers, fdrcp_data_t, node);

        list_del(&buff->node);
        fdrcp_free(buff);
    }
}

fdrcp_user_t *fdrcp_acquire_user(){
    fdrcp_t *cp = fdrcp_get();
    fdrcp_user_t *user = NULL;

    pthread_spin_lock(&cp->lock);

    if(!list_empty(&cp->free_users)){
        user = list_first_entry(&cp->free_users, fdrcp_user_t, node);
        list_del(&user->node);
    }

    pthread_spin_unlock(&cp->lock);

    return user;
}

void fdrcp_release_user(fdrcp_user_t *user){
    fdrcp_t *cp = fdrcp_get();

    pthread_spin_lock(&cp->lock);

    list_add_tail(&user->node, &cp->free_users);

    pthread_spin_unlock(&cp->lock);
}

static int fdrcp_user_init(fdrcp_t *cp, int num){
    fdrcp_user_t *user = NULL;
    int i;

    for(i = 0; i < num; i++){
        user = (fdrcp_user_t*)fdrcp_malloc(sizeof(fdrcp_user_t));
        if(user == NULL){
            log_warn("fdrcp_buffer_init => alloc memory fail\n");
            return -1;
        }
        memset(user, 0, sizeof(fdrcp_user_t));

        INIT_LIST_HEAD(&user->node);
        list_add(&user->node, &cp->free_users);
    }

    return 0;
}

static void fdrcp_user_fini(fdrcp_t *cp){
    fdrcp_user_t *user = NULL;

    while(!list_empty(&cp->free_users)){
        user = list_first_entry(&cp->free_users, fdrcp_user_t, node);

        list_del(&user->node);
        fdrcp_free(user);
    }
}


fdrcp_vcode_t *fdrcp_acquire_vcode(){
    fdrcp_t *cp = fdrcp_get();
    fdrcp_vcode_t *vcode = NULL;

    pthread_spin_lock(&cp->lock);

    if(!list_empty(&cp->free_vcode)){
        vcode = list_first_entry(&cp->free_vcode, fdrcp_vcode_t, node);
        list_del(&vcode->node);
    }

    pthread_spin_unlock(&cp->lock);

    return vcode;
}

void fdrcp_release_vcode(fdrcp_vcode_t *vcode){
    fdrcp_t *cp = fdrcp_get();

    pthread_spin_lock(&cp->lock);

    list_add_tail(&vcode->node, &cp->free_vcode);

    pthread_spin_unlock(&cp->lock);
}

static int fdrcp_vcode_init(fdrcp_t *cp, int num){
    fdrcp_vcode_t *vcode = NULL;
    int i;

    for(i = 0; i < num; i++){
        vcode = (fdrcp_vcode_t*)fdrcp_malloc(sizeof(fdrcp_vcode_t));
        if(vcode == NULL){
            log_warn("fdrcp_vcode_init => alloc memory fail\n");
            return -1;
        }
        memset(vcode, 0, sizeof(fdrcp_vcode_t));

        INIT_LIST_HEAD(&vcode->node);
        list_add(&vcode->node, &cp->free_vcode);
    }

    return 0;
}

static void fdrcp_vcode_fini(fdrcp_t *cp){
    fdrcp_vcode_t *vcode = NULL;

    while(!list_empty(&cp->free_vcode)){
        vcode = list_first_entry(&cp->free_vcode, fdrcp_vcode_t, node);

        list_del(&vcode->node);
        fdrcp_free(vcode);
    }
}

static fdrcp_user_t *fdrcp_cache_match(fdrcp_t *fcp, int64_t timestamp, fdrcp_fr_t *fr){
    fdrcp_conf_t *conf = fdrcp_conf_get();
    fdrcp_user_t *pos, *n, *find = NULL;
    float score;

    list_for_each_entry_safe(pos, n, &fcp->lrus, node){
        // drop old cached user
        if(((fcp->last_ts - pos->timestamp) > FDRCP_STRANGE_TIPS) && (pos->tfid != fr->tfid)){
            list_del(&pos->node);
            list_add(&pos->node, &fcp->rpus);
            continue;
        }

        score = fdrcp_xma_match(pos->feature, fr->feature, FDRCP_FEATURE_MAX);

        log_info("fdrcp_cache_match -> %f - %f\n", score, conf->algm_threshold);

        if(score >= conf->algm_threshold){
            pos->timestamp = timestamp;
            find = pos;
            break;
        }
    }

    return find;
}

static int fdrcp_cache_update(fdrcp_t *fcp, int64_t timestamp, fdrcp_user_t *user){
    fdrcp_user_t *pos, *n;
    int find = 0;

    list_for_each_entry_safe(pos, n, &fcp->lrus, node){
        // drop old cached user
        if((fcp->last_ts - timestamp) > FDRCP_STRANGE_TIPS){
            if(strcmp(pos->usrid, user->usrid) == 0){
                pos->timestamp = timestamp;
                find = 1;
                continue;
            }

            list_del(&pos->node);
            list_add(&pos->node, &fcp->rpus);
            continue;
        }
    }

    if(!find){
        list_del(&user->node);
        user->timestamp = timestamp;
        list_add(&user->node, &fcp->lrus);
    }

    return find;
}

static int fdrcp_cache_add(fdrcp_t *fcp, int64_t timestamp, fdrcp_user_t *user){

    user->timestamp = timestamp;

    list_del(&user->node);
    list_add(&user->node, &fcp->lrus);

    return 0;
}

static fdrcp_user_t *fdrcp_user_match(fdrcp_t *fcp, float *feature, float *score){
    fdrcp_conf_t *conf = fdrcp_conf_get();
    fdrcp_user_t *pos = NULL;
    int idx;

    *score = 0.0;
    idx = fdrcp_match(fcp, feature, conf->algm_threshold, score);
    if((idx >= 0) && (idx < fcp->user_cur)){
        pos = &fcp->users[idx];
    }

    return pos;
}

static void fdrcp_gui_update(fdrcp_t *fcp, int64_t timestamp, fdrcp_fd_t *fd, fdrcp_user_t *user){
    lvgui_user_t *lu = lvgui_alloc_user();

    if(lu == NULL){
        log_warn("fdrcp_user_reco -> no lvgui user buffer\n");
        lvgui_free_user(lu);
        return ;
    }
    
    lu->fid = user->tfid;
    
    strncpy(lu->uid, user->usrid, LVGUI_UID_MAX - 1);
    strncpy(lu->name, user->uname, LVGUI_NAME_MAX - 1);
    strncpy(lu->desc, user->udesc, LVGUI_DESC_MAX - 1);
    
    lu->mask = (timestamp / 1000) > user->expired ? LVGUI_MASK_EXPIRED|LVGUI_MASK_RECOGN : LVGUI_MASK_PASS|LVGUI_MASK_RECOGN;
    
    lu->xpos  = fd->rect.x;
    lu->ypos  = fd->rect.y;
    lu->width = fd->rect.w;
    lu->heigh = fd->rect.h;

    lvgui_dispatch(lu);
}

static void fdrcp_voice_play(fdrcp_t *fcp, int type, const char *uname, int again){
    fdrcp_conf_t *conf = fdrcp_conf_get();

    log_info("fdrcp_voice_play -> play voice for user %s\n", uname);

    switch(conf->voice_mode){
    case FDRCP_VOICE_MODE_CLOSED:
        // do nothing
        break;

    case FDRCP_VOICE_MODE_DEFAULT:
        if(!again){
            // FIXME : play open voice
        }
        break;

    case FDRCP_VOICE_MODE_USER:
        if(!again){
            // FIXME : play open voice by name
        }
        break;

    default:
        break;
    }
}

static void fdrcp_relay_control(fdrcp_t *fcp, int again){
    fdrcp_conf_t *conf = fdrcp_conf_get();

    log_info("fdrcp_relay_control -> mode %d\n", conf->relay_mode);

    if(conf->relay_mode){
        bisp_hal_enable_relay(0, 1);
        fdrcp_settimer(&fcp->ev_relay, conf->door_duration, 0);
    }
}

static void fdrcp_rs485_control(fdrcp_t *fcp, const char *uname, int again){
    fdrcp_conf_t *conf = fdrcp_conf_get();

    log_info("fdrcp_rs485_control -> rs484 for user %s\n", uname);

    switch(conf->rs485_mode){
    case FDRCP_RS485_MODE_NONE:
        // do nothing
        break;

    case FDRCP_RS485_MODE_DOOR:
        if(!again){
            // FIXME: send data 
            fdrcp_settimer(&fcp->ev_rs485, conf->door_duration, 0);
        }
        break;

    case FDRCP_RS485_MODE_VOICE:
        if(!again){
            // FIXME: send voice data
        }
        break;

    case FDRCP_RS485_MODE_USER:
        if(!again){
            // FIXME: send data with name
            fdrcp_settimer(&fcp->ev_rs485, conf->door_duration, 0);
        }
        break;
    }
}

static int fdrcp_user_reco(fdrcp_t *fcp, const char *uname, int again){

    // 1. voice play
    fdrcp_voice_play(fcp, FDRCP_VOICE_TYPE_USER, uname, again);

    // 2. relay control
    fdrcp_relay_control(fcp, again);

    // 3. rs485 control
    fdrcp_rs485_control(fcp, uname, again);

    return 0;
}

static fdrcp_track_t *fdrcp_strange_add(fdrcp_t *fcp, int64_t timestamp, int tfid){
    fdrcp_track_t *strange;
    int slot = fcp->strange_max;
    int i;

    for(i = 0; i < fcp->strange_max; i++){
        strange = &fcp->stranges[i];
        
        // cleared, continue
        if(strange->timestamp == 0){
            if(slot == fcp->strange_max)
                slot = i;
            continue;
        }

        if(strange->tfid == tfid){
            if((timestamp - strange->timestamp) >= FDRCP_STRANGE_TIPS){
                return strange;
            }
        }

        // clear old tracked face id
        if((fcp->last_ts - strange->timestamp) >= FDRCP_STRANGE_TIPS){
            strange->timestamp = 0;
            strange->logr = 0;
            if(slot == fcp->strange_max){
                slot = i;
            }
        }
    }

    if((slot == fcp->strange_max) && (fcp->strange_max == FDRCP_STRANGE_MAX)){
        log_warn("fdrcp_strange_add -> track face id is overflow!\n");
    }else{
        strange = &fcp->stranges[slot];
        strange->tfid = tfid;
        strange->timestamp = timestamp;
        strange->logr = 0;

        if(slot == fcp->strange_max){
            fcp->strange_max ++;
        }
    }

    return NULL;
}

static fdrcp_fd_t *fdrcp_data_fd(fdrcp_data_t *data, int tfid){
    fdrcp_fd_t  *fd;
    int i;

    for(i = 0; i < data->fdnum; i++){
        fd = &data->fds[i];
        if(fd->tfid == tfid){
            return fd;
        }
    }

    return NULL;
}

static void fdrcp_evmsg_data(fdrcp_t *fcp, fdrcp_data_t *data){
    fdrcp_conf_t *conf = fdrcp_conf_get();
    fdrcp_user_t *user;
    fdrcp_fd_t   *fd;
    fdrcp_fr_t   *fr;
    fdrcp_track_t *str;
    float score = 0.0;
    int logr = 0;
    int i;


    for(i = 0; i < data->frnum; i++){
        
        fr = &data->frs[i];
        fd = fdrcp_data_fd(data, fr->tfid);
        if(fd == NULL){
            log_warn("fdrcp_evmsg_data -> fr without fd data\n");
            continue;
        }
        
        // cache user search
        user = fdrcp_cache_match(fcp, data->timestamp, fr);
        if(user != NULL){
            // 1. update cache
            fdrcp_cache_update(fcp, data->timestamp, user);
            
            // 2. gui update
            fdrcp_gui_update(fcp, data->timestamp, fd, user);

            if(conf->therm_mode){
            // 3. update therm last timestamp
                fcp->logr.last = data->timestamp;
            }else{
            // 3. update controller
                fdrcp_user_reco(fcp, user->uname, 1);
            }

            continue;
        }

        // repo search
        user = fdrcp_user_match(fcp, fr->feature, &score);
        if(user != NULL){
            // 1. add to cache
            fdrcp_cache_add(fcp, data->timestamp, user);

            // 2. gui update
            fdrcp_gui_update(fcp, data->timestamp, fd, user);

            // 3. set logr
            fdrcp_logr_set(fcp, data->timestamp, fd, user, score);
            if(!conf->therm_mode){
            // 4. set controller
                fdrcp_user_reco(fcp, user->uname, 0);
                fdrcp_logr_report(fcp);
            }

            logr = 1;   // new recognized face
        }else{
            str = fdrcp_strange_add(fcp, data->timestamp, fr->tfid);
            if(str != NULL){
                lvgui_setmode(LVGUI_MODE_TIPS, conf->tips_duration);
                if(str->logr == 0){
                    str->logr = 1;

                    fdrcp_logr_set(fcp, data->timestamp, fd, user, score);
                    strncpy(fcp->logr.uid, FDRCP_EVENT_STRANGE_UID, FDRCP_USRID_MAX);
                    fdrcp_logr_report(fcp);
                    fcp->logr.octime = 0;

                    logr = 1;   // strange
                }
            }
        }
    }

    fcp->last_ts = data->timestamp;

    if(logr){
        // save frame
        fdrcp_utils_logr(data->timestamp, data->frame);
    }
}

static void fdrcp_evmsg_key(fdrcp_t *fcp, int key){
    fdrcp_conf_t *conf = fdrcp_conf_get();
    time_t now;

    switch(key){
    case 1: // key down
        fcp->key_down = time(NULL);
        break;
    
    case 0: // key up
        now = time(NULL);
        if((now - fcp->key_down) > conf->info_duration){
            lvgui_setmode(LVGUI_MODE_INFO, conf->info_duration);
            bisp_aie_set_mode(BISP_AIE_MODE_NONE);
            fdrcp_settimer(&fcp->ev_state, conf->info_duration, 0);
        }else{
            lvgui_setmode(LVGUI_MODE_VISI, conf->visi_duration);
            bisp_aie_set_mode(BISP_AIE_MODE_QRCODE);
            fdrcp_settimer(&fcp->ev_state, conf->visi_duration, 0);
        }

        break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        // TODO : add keyword support
        break;
    }
}

static void fdrcp_evmsg_qrcode(fdrcp_t *fcp, const char *qrcode){
    fdrcp_conf_t *conf = fdrcp_conf_get();

    // check visitor code
    if(fdrcp_vcode_match(fcp, qrcode) == 0){
        // 1. gui update
        // FIXME : tips for VISI PASS
        lvgui_setmode(LVGUI_MODE_VISI, conf->visi_duration);

        // 2. voice play
        fdrcp_voice_play(fcp, FDRCP_VOICE_TYPE_VISITOR, NULL, 0);

        // 3. relay control
        fdrcp_relay_control(fcp, 0);

        // 4. rs485 control
        fdrcp_rs485_control(fcp, NULL, 0);

        // FIXME : log visitor
    }else{

        // FIXME : tips for VISI DENY, try again
        lvgui_setmode(LVGUI_MODE_VISI, conf->visi_duration);
    }
}

static void fdrcp_evmsg_therm(fdrcp_t *fcp, float therm){
    fdrcp_conf_t *conf = fdrcp_conf_get();
    
    if(conf->therm_mode == 0){
        // log_warn("fdrcp_evmsg_therm => not in therm mode\n");
        return ;
    }

    therm += conf->therm_fix;

    // FIXME: send to gui
    lvgui_therm_update(therm);

    if((fcp->logr.octime != 0) && (therm >= conf->therm_threshold)){
        fcp->logr.therm = therm;

        fdrcp_user_reco(fcp, fcp->logr.uname, 0);

        fdrcp_logr_report(fcp);
        
        fcp->logr.octime = 0;
    }
}

static void  fdrcp_event_handler(struct bufferevent *bev, void *ctx){
    fdrcp_t    *fcp = (fdrcp_t*)ctx;
    fdrcp_evmsg_t evm;

    struct evbuffer *input = bufferevent_get_input(bev);
    
    while(evbuffer_get_length(input) >= sizeof(evm)){
        evbuffer_remove(input, &evm, sizeof(evm));

        switch(evm.type){
        case FDRCP_EVMSG_DATA:
            fdrcp_evmsg_data(fcp, (fdrcp_data_t *)evm.data);
            fdrcp_release((fdrcp_data_t *)evm.data);
            break;

        case FDRCP_EVMSG_KEY:
            fdrcp_evmsg_key(fcp, evm.key);
            break;

        case FDRCP_EVMSG_QRCODE:
            fdrcp_evmsg_qrcode(fcp, evm.qrcode);
            break;

        case FDRCP_EVMSG_THERM:
            fdrcp_evmsg_therm(fcp, evm.therm);
            break;

        case FDRCP_EVMSG_START:
            bisp_aie_set_mode(BISP_AIE_MODE_FACE);
            break;

        case FDRCP_EVMSG_USER_APPEND:
            fdrcp_user_append(fcp, (fdrcp_user_t *)evm.data);
            fdrcp_release_user((fdrcp_user_t *)evm.data);
            break;

        case FDRCP_EVMSG_USER_UPDATE:
            fdrcp_user_update(fcp, (fdrcp_user_t *)evm.data);
            fdrcp_release_user((fdrcp_user_t *)evm.data);
            break;

        case FDRCP_EVMSG_USER_DELETE:
            fdrcp_user_delete(fcp, (fdrcp_user_t *)evm.data);
            fdrcp_release_user((fdrcp_user_t *)evm.data);
            break;

        case FDRCP_EVMSG_VCODE_INSERT:
            fdrcp_vcode_append(fcp, (fdrcp_vcode_t *)evm.data);
            fdrcp_release_vcode((fdrcp_vcode_t *)evm.data);
            break;

        case FDRCP_EVMSG_VCODE_DELETE:
            fdrcp_vcode_delete(fcp, (fdrcp_vcode_t *)evm.data);
            fdrcp_release_vcode((fdrcp_vcode_t *)evm.data);
            break;

        default:
            log_warn("fdrcp_event_handler => event %d unkown\n", evm.type);
            return ;
        }
    }

    return;
}

static void fdrcp_settimer(struct event *ev, int seconds, int useconds){
    
    if(seconds > 0){
        struct timeval tv = {seconds, useconds};
        event_add(ev, &tv);
    }else{
        event_del(ev);
    }
}

static void fdrcp_state_proc(evutil_socket_t sock, short events, void *args){
    bisp_aie_set_mode(BISP_AIE_MODE_FACE);
}


static void fdrcp_logr_proc(evutil_socket_t sock, short events, void *args){
    fdrcp_t *fcp = (fdrcp_t *)args;

    if(fcp->logr.octime != 0){
        fcp->logr.octime = 0;
        // fdrcp_logr_report(fcp);
    }
}

static void fdrcp_relay_proc(evutil_socket_t sock, short events, void *args){
    fdrcp_conf_t *conf = fdrcp_conf_get();

    log_info("fdrcp_relay_proc -> mode %d\n", conf->relay_mode);

    if(conf->relay_mode){
        bisp_hal_enable_relay(0, 0);
    }
}

static void fdrcp_rs485_proc(evutil_socket_t sock, short events, void *args){
    fdrcp_conf_t *conf = fdrcp_conf_get();

    log_info("fdrcp_rs485_proc -> mode %d\n", conf->rs485_mode);

    switch(conf->rs485_mode){
    case FDRCP_RS485_MODE_NONE:
        break;

    case FDRCP_RS485_MODE_DOOR:
        break;

    case FDRCP_RS485_MODE_VOICE:
        break;

    case FDRCP_RS485_MODE_USER:
        break;
    }
}

static void *fdrcp_thread_proc(void *args){
    fdrcp_t *fcp = (fdrcp_t *)args;
    int rc;
    
    fcp->base = event_base_new();
    if (fcp->base == NULL){
        log_warn("fdrcp_thread_proc => create event_base failed!\n");
        return (void*)-1;
    }

    rc = bufferevent_pair_new(fcp->base, BEV_OPT_THREADSAFE, fcp->bepair);
    if(0 != rc){
        log_warn("fdrcp_thread_proc  => create event pair fail:%d\n", rc);
        event_base_free(fcp->base);
        return (void*)-1;
    }

    bufferevent_enable(fcp->bepair[1], EV_READ);
    bufferevent_enable(fcp->bepair[0], EV_WRITE);
    bufferevent_setcb(fcp->bepair[1], fdrcp_event_handler, NULL, NULL, fcp);

    event_assign(&fcp->ev_state, fcp->base,  -1,  EV_TIMEOUT, fdrcp_state_proc, fcp);
    event_assign(&fcp->ev_state, fcp->base,  -1,  EV_TIMEOUT, fdrcp_logr_proc, fcp);

    event_assign(&fcp->ev_relay, fcp->base,  -1,  EV_TIMEOUT, fdrcp_relay_proc, fcp);
    event_assign(&fcp->ev_rs485, fcp->base,  -1,  EV_TIMEOUT, fdrcp_rs485_proc, fcp);

    fcp->udpsock = socket(AF_INET, SOCK_DGRAM, 0);
    if(fcp->udpsock == -1) {
        log_warn("fdrcp_thread_proc => create udp socket fail \n");
        event_base_free(fcp->base);
        return (void*)-1;
    }

    log_info("fdrcp_thread_proc => control plane looping ... \n");
    fcp->status = 1;

    event_base_loop(fcp->base, EVLOOP_NO_EXIT_ON_EMPTY);

    bufferevent_free(fcp->bepair[0]);
    bufferevent_free(fcp->bepair[1]);
    
    event_base_free(fcp->base);

    pthread_spin_destroy(&fcp->lock);

    fdrcp_buffer_fini(fcp);
    fdrcp_user_fini(fcp);
    fdrcp_vcode_fini(fcp);

    return NULL;
}

int fdrcp_init(){
    fdrcp_t      *cp = fdrcp_get();
    fdrcp_conf_t *config = fdrcp_conf_get();
    fdrcp_user_t *user;
    int i;
    int rc;

    log_info("fdrcp_init => init ...\n");

    // init controller
    memset(cp, 0, sizeof(fdrcp_t));

    INIT_LIST_HEAD(&cp->free_buffers);
    fdrcp_buffer_init(cp, FDRCP_DATA_BUF_MAX);

    INIT_LIST_HEAD(&cp->free_users);
    fdrcp_user_init(cp, FDRCP_USER_BUF_MAX);

    INIT_LIST_HEAD(&cp->free_vcode);
    fdrcp_vcode_init(cp, FDRCP_VCODE_BUF_MAX);

    log_info("fdrcp_init => init repo \n");

    // init lru list
    INIT_LIST_HEAD(&cp->lrus);
    INIT_LIST_HEAD(&cp->rpus);

    // init user repo
    cp->user_max = config->user_max;
    cp->users = (fdrcp_user_t*)fdrcp_malloc(cp->user_max*sizeof(fdrcp_user_t));
    if(cp->users == NULL){
        log_warn("fdrcp_init => alloc memory for repo fail!\n");
        return -1;
    }
    memset(cp->users, 0, cp->user_max*sizeof(fdrcp_user_t));
    for(i = 0; i < cp->user_max; i++){
        user = &cp->users[i];
        list_add(&user->node, &cp->rpus);
    }

    pthread_spin_init(&cp->lock, 0);

    log_info("fdrcp_init => create mainloop ...\n");

    rc = pthread_create(&cp->thread, NULL, fdrcp_thread_proc, cp);
    if(rc != 0){
        log_warn("fdrcp_init => create thread fail:%d\n", rc);
        fdrcp_free(cp->users);

        return rc;
    }

    while(!cp->status){
        usleep(10*1000);
    }

    return 0;
}

int fdrcp_fini(){
    fdrcp_t    *cp = fdrcp_get();

    event_base_loopexit(cp->base, NULL);
    usleep(10000);

    fdrcp_free(cp->users);
    cp->status = 0;

    return 0;
}

int fdrcp_fdrdispatch(fdrcp_data_t *fdr){
    fdrcp_t    *cp = fdrcp_get();
    fdrcp_evmsg_t evm;

    evm.type = FDRCP_EVMSG_DATA;
    evm.data = fdr;

    if(bufferevent_write(cp->bepair[0], &evm, sizeof(evm))) {
        log_warn("fdrcp_fdrdispatch => bufferevent_write failed\n");
        return -1;
    }

    return 0;
}

int fdrcp_qrcdispatch(const char *qrcode){
    fdrcp_t    *cp = fdrcp_get();
    fdrcp_evmsg_t evm;
    char *qrc;

    evm.type = FDRCP_EVMSG_QRCODE;

    qrc = fdrcp_malloc(strlen(qrcode) + 1);
    if(qrc == NULL){
        log_warn("fdrcp_qrcdispatch => fdrcp_malloc fail!\n");
        return -1;
    }
    strcpy(qrc, qrcode);

    evm.qrcode = qrc;

    if(bufferevent_write(cp->bepair[0], &evm, sizeof(evm))) {
        log_warn("fdrcp_qrcdispatch => bufferevent_write failed\n");
        return -1;
    }

    return 0;
}

int fdr_cp_keydispatch(int status){
    fdrcp_t    *cp = fdrcp_get();
    fdrcp_evmsg_t evm;

    evm.type = FDRCP_EVMSG_KEY;
    evm.key = status;

    if(bufferevent_write(cp->bepair[0], &evm, sizeof(evm))) {
        log_warn("fdr_cp_keydispatch => bufferevent_write failed\n");
        return -1;
    }

    return 0;
}

int fdrcp_thermdispatch(float therm){
    fdrcp_t    *cp = fdrcp_get();
    fdrcp_evmsg_t evm;

    evm.type = FDRCP_EVMSG_THERM;
    evm.therm = therm;

    if(bufferevent_write(cp->bepair[0], &evm, sizeof(evm))) {
        log_warn("fdrcp_thermdispatch => bufferevent_write failed\n");
        return -1;
    }

    return 0;
}

int fdrcp_start(){
    fdrcp_t    *cp = fdrcp_get();
    fdrcp_evmsg_t evm;

    evm.type = FDRCP_EVMSG_START;

    log_info("fdrcp_start => start\n");

    if(bufferevent_write(cp->bepair[0], &evm, sizeof(evm))) {
        log_warn("fdrcp_start => bufferevent_write failed\n");
        return -1;
    }

    return 0;
}

int fdrcp_usrdispatch(int type, fdrcp_user_t *user){
    fdrcp_t    *cp = fdrcp_get();
    fdrcp_evmsg_t evm;

    switch(type){
    case FDRCP_EVMSG_USER_APPEND:
    case FDRCP_EVMSG_USER_UPDATE:
    case FDRCP_EVMSG_USER_DELETE:
        evm.type = type;
        evm.data = user;
        break;
    default:
        return -1;
    }

    if(bufferevent_write(cp->bepair[0], &evm, sizeof(evm))) {
        log_warn("fdrcp_usrdispatch => bufferevent_write failed\n");
        return -1;
    }

    return 0;
}

int fdrcp_vtcdispatch(int type, fdrcp_vcode_t *vcode){
    fdrcp_t    *cp = fdrcp_get();
    fdrcp_evmsg_t evm;

    switch(type){
    case FDRCP_EVMSG_VCODE_INSERT:
    case FDRCP_EVMSG_VCODE_DELETE:
        evm.type = type;
        evm.data = vcode;
        break;
    default:
        return -1;
    }

    if(bufferevent_write(cp->bepair[0], &evm, sizeof(evm))) {
        log_warn("fdrcp_vtcdispatch => bufferevent_write failed\n");
        return -1;
    }

    return 0;
}

static int fdrcp_user_append(fdrcp_t *cp, fdrcp_user_t *user){
    fdrcp_user_t *u;
    int rc = -1;

    if(cp->user_cur < (cp->user_max - 1)){
        u = &cp->users[cp->user_cur];
        memcpy(u, user, sizeof(fdrcp_user_t));
        INIT_LIST_HEAD(&u->node);
        list_add(&u->node, &cp->rpus);
        cp->user_cur++;
        rc = 0;
    }


    return rc;
}

static int fdrcp_user_update(fdrcp_t *cp, fdrcp_user_t *user){
    fdrcp_user_t *u;
    struct list_head head;
    int rc = -1;
    int i;

    for(i = 0; i < cp->user_cur; i++){
        u = &cp->users[i];
        if(strcmp(user->usrid, u->usrid) == 0){
            // backup list node
            memcpy(&head, &u->node, sizeof(head));
            // update buffer
            memcpy(u, user, sizeof(fdrcp_user_t));
            // restore list node
            memcpy(&u->node, &head, sizeof(head));
            rc = 0;
            break;
        }
    }

    return rc;
}

int fdrcp_user_delete(fdrcp_t *cp, fdrcp_user_t *user){
    fdrcp_user_t *u;
    int rc = -1;
    int i;

    for(i = 0; i < cp->user_cur; i++){
        u = &cp->users[i];

        if(strcmp(u->usrid, user->usrid) == 0){
            list_del(&u->node);

            if(cp->user_cur == (cp->user_max - 1)){
                cp->user_cur --;   // last one, only set point;
            }else{
                memmove(&cp->users[i], &cp->users[i+1], sizeof(fdrcp_user_t)*(cp->user_cur - i));
                cp->user_cur --;
            }
            rc = 0;
            break;
        }
    }

    return rc;
}

static int fdrcp_vcode_append(fdrcp_t *cp, fdrcp_vcode_t *vcode){
    fdrcp_vcode_t *v;
    int rc = -1;

    if(cp->vcode_cur < (cp->vcode_max - 1)){
        v = &cp->vcodes[cp->vcode_cur];
        memcpy(v, vcode, sizeof(fdrcp_vcode_t));
        cp->vcode_cur++;
        rc = 0;
    }

    return rc;
}

static int fdrcp_vcode_delete(fdrcp_t *cp, fdrcp_vcode_t *vcode){
    fdrcp_vcode_t *v;
    int rc = -1;
    int i;

    for(i = 0; i < cp->vcode_cur; i++){
        v = &cp->vcodes[i];

        if(v->vcid == vcode->vcid){
            if(cp->vcode_cur == (cp->vcode_max - 1)){
                cp->vcode_cur --;   // last one, only set point;
            }else{
                memmove(&cp->vcodes[i], &cp->vcodes[i+1], sizeof(fdrcp_vcode_t)*(cp->vcode_cur - i));
                cp->vcode_cur --;
            }
            rc = 0;
            break;
        }
    }

    return rc;
}

static int fdrcp_vcode_match(fdrcp_t *cp, const char *qrcode){
    fdrcp_vcode_t *v;
    int i;

    if(qrcode == NULL){
        return -1;
    }

    for(i = 0; i < cp->vcode_cur; i++){
        v = &cp->vcodes[i];

        if(strcmp(v->vcode, qrcode) == 0){
            time_t cur = time(NULL);

            if((cur >= v->start) && (cur <= v->expire)){
                return 0;
            }
        }
    }

    return -1;
}

static int fdrcp_rule_match(fdrcp_t *cp, int rule){
    return 0;
}

static int fdrcp_rule_append(fdrcp_t *cp, fdrcp_rule_t *rule){
    return 0;
}

static int fdrcp_rule_delete(fdrcp_t *cp, fdrcp_rule_t *rule){
    return 0;
}

static int fdrcp_match(fdrcp_t *cp, float *feature, float thresh_hold, float *score){
    fdrcp_user_t *user;
    float probmax = .0f;
    float prob;
    int i;
    int rc = -1;

    // log_info("fdrcp_match -> user_cur: %d - %f\r\n", cp->user_cur, thresh_hold);

    for (i = 0; i < cp->user_cur; i++){
        user = &cp->users[i];

        prob = fdrcp_xma_match(feature, user->feature, FDRCP_FEATURE_MAX);

        if ((prob > thresh_hold) && (prob > probmax)){
            probmax = prob;
            *score = prob;
            rc = i;
        }
    }

    // log_info("fdrcp_match -> result : %d - %f \r\n", rc, *score);

#ifdef TEST
    if(probmax > thresh_hold){
        log_info("fdrcp_match => user %s score %f\n", cp->users[rc].uname, *score);
    }
#endif

    return rc;
}

static float fdrcp_xma_match(float *feature1, float *feature2, int len){
    float score = 0;
    float dot = 0;
    float norm1 = 0;
    float norm2 = 0;
    int i;

    for (i = 0; i < len; i++){
        dot = dot + feature1[i] * feature2[i];
        norm1 = norm1 + feature1[i] * feature1[i];
        norm2 = norm2 + feature2[i] * feature2[i];
    }

    norm1 = sqrt(norm1);
    norm2 = sqrt(norm2);
    score = dot / (norm1*norm2);

    return score;
}

static void fdrcp_logr_set(fdrcp_t *fcp, int64_t oct, fdrcp_fd_t *fd, fdrcp_user_t *user, float score){
    fdrcp_logr_t *lr = &fcp->logr;

    lr->octime = oct;
    lr->last = oct;

    lr->x = fd->rect.x;
    lr->y = fd->rect.y;
    lr->w = fd->rect.w;
    lr->h = fd->rect.h;

    if(user != NULL){
        strncpy(lr->uid, user->usrid, FDRCP_USRID_MAX);
        strncpy(lr->uname, user->uname, FDRCP_UNAME_MAX);
    }else{
        strncpy(lr->uid, FDRCP_EVENT_STRANGE_UID, FDRCP_USRID_MAX);
        strncpy(lr->uname,FDRCP_EVENT_STRANGE_UNAME, FDRCP_UNAME_MAX);
    }

    lr->sharp = fd->sharp;
    lr->score = score;
    lr->therm = 0.0;
}

// fdrcp_fd_t *fd, int64_t octime, float score, float bbt, char *userid
static void fdrcp_logr_report(fdrcp_t *fcp){
    fdrcp_logr_t *lr = &fcp->logr;
    fdrcp_conf_t *conf = fdrcp_conf_get();
    struct sockaddr_in si;
    char buffer[2048];
    int len;
    int64_t ts = fdrcp_timestamp();

    // no cached logr
    if(lr->octime == 0){
        return ;
    }

    // timeout without therm, drop saved frame data
    if(conf->therm_mode && (ts > lr->last)){
        fdrcp_logr_delete(lr->octime);
        lr->octime = 0;
        return ;
    }

    len = snprintf(buffer, 2046, "{ \"type\":\"logr\", \"octime\":%lld,\"face_x\":%d, \"face_y\":%d,\"face_w\":%d, \"face_h\":%d,\"sharp\":%f, \"score\":%f, \"therm\":%f, \"userid\":\"%s\",\"username\":\"%s\" }",
            lr->octime, lr->x, lr->y, lr->w, lr->h, lr->sharp, lr->score, lr->therm, lr->uid, lr->uname);

    memset(&si, 0, sizeof(si));
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr("127.0.0.1");
    si.sin_port = htons(FDRCP_EVENT_PORT);

    // log_warn("fdrcp_event_logr => port:%d, len:%d,  %s\n", FDRCP_EVENT_PORT, len, buffer);
    sendto(fcp->udpsock, buffer, len, 0, (struct sockaddr *)&si, sizeof(si));
}

