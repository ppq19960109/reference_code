/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr-cp.h"
#include "fdr-mp.h"
#include "fdr.h"

#include "fdr-logger.h"
#include "fdr-qrcode.h"
#include "fdr-path.h"
#include "fdr-dbs.h"

#include "bisp_gui.h"
#include "bisp_aie.h"
#include "bisp_hal.h"

#include "logger.h"
#include "base58.h"

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

#define CPXMUI_TRACK_MAX               32
#define CPXMUI_FACEID_MAX              6

#define CPXMUI_PATH_MAX                128

// state machine declerations
/*
 *                    |<------------|(4)
 * active(0) ---> config(1) ---> facerec <---------------|
 *                               (2)| --- visitor(3) --->|
 */
typedef enum {
    CPXMUI_STATE_FACEREC = 0,      // face recognization state
    CPXMUI_STATE_VISITOR_QRSCAN,   // visitor, scan qrcode mode
    CPXMUI_STATE_VISITOR_KBINPUT,  // visitor, keyboard input simple code mode
    CPXMUI_STATE_CONFIG,           // config system
    CPXMUI_STATE_ACTIVATE,         // activate & register
    CPXMUI_STATE_MAX
}cpxmui_state_e;

typedef struct {
    // timer for door operations
    struct event ev_door;

    // timer for state machine
    struct event ev_state;

    // sate machine
    int state;

    time_t keytime; // key pressed time
}cpxmui_state_t;

// face tracking declerations
typedef enum {
    FDR_PERM_NONE = 0,  // have no permission
    FDR_PERM_AUTHEN,    // have permission to open the door
    FDR_PERM_EXPIRED,   // permission is expired
    FDR_PERM_NOEXIST    // person is not exist
}fdr_perm_e;

typedef struct {
    int faceid;         // faceid from algorim
    int idx;            // tracked face index from ai engine

    int64_t start;      // start tracked time
    int64_t current;    // last tracked time

    int stranger;       // stranger or recognized
    fdr_perm_e perm;    // open door permition
    int     show_mode;  // show mode : permit, expired, not permit
    char userid[FDR_DBS_USERID_SIZE+1];
}face_track_t;

// instance
typedef struct fdr_cp_xmui{
    fdr_cp_t *fcp;

    cpxmui_state_t state;

    // internal used for simplify code
    char image_path[CPXMUI_PATH_MAX];

    void *image;
    int   buflen;

    fdr_cp_user_t *last;
    
    // track filter
    int track_num;
    face_track_t tracks[CPXMUI_TRACK_MAX];
}fdr_cp_xmui_t;

static const char *user_image_path  = USER_IMAGE_PATH;

const static unsigned char qrmagic_visitor[] = "BFAV";

static void state_settimer(cpxmui_state_t *state, int seconds){
    
    log_info("state_settimer ==> state %d, seconds : %d\n", state->state, seconds);

    if(seconds > 0){
        struct timeval tv = {seconds, 0};
        event_add(&state->ev_state, &tv);
    }else{
        event_del(&state->ev_state);
    }
}

// check all result in tracker
static void face_tracking(fdr_cp_xmui_t *xmui, int64_t oct, int num, bisp_aie_frval_t *rfs){
    bisp_aie_frval_t  *frv;
    face_track_t *ft;
    int i, j;
    int slot = -1;
    int find = 0;

    for(i = 0; i < num; i++){
        frv = &rfs[i];
        slot = -1;
        find = 0;

        log_info("face_tracking => %d -> id:%d\n", i, frv->id);

        for(j = 0; j < CPXMUI_TRACK_MAX; j++){
            ft = &xmui->tracks[j];
            if(ft->faceid == -1){
                if(slot == -1)
                    slot = j;
                continue;   // empty slot, skip
            }

            if(frv->id == ft->faceid){
                find = 1;   // already in tracks

                ft->current = oct;
                ft->idx = i;
                break;      // check next face in frval
            }
        }

        // add to track
        if((find == 0)&&(slot != -1)){
            ft = &xmui->tracks[slot];

            ft->faceid = frv->id;
            ft->idx = i;
            ft->current = oct;
            ft->perm = FDR_PERM_NONE;
            ft->show_mode = BISP_GUI_FDRMODE_STRANGER;
            ft->stranger = 1;

            xmui->track_num ++;

            if(xmui->track_num >= CPXMUI_TRACK_MAX){
                log_warn("face_tracking => track buffer is overflow\n");
            }
        }
    }
}


static void xmui_door_enable(fdr_cp_xmui_t *xmui, int perm){
    fdr_config_t *config = fdr_config();

    if(perm == FDR_PERM_AUTHEN){
        bisp_hal_enable_relay(0, 1);    // open door

        struct timeval tv = {config->door_open_latency, 0};
        event_add(&xmui->state.ev_door, &tv);
    }
}
/*
static void xmui_door_timerout_reset(fdr_cp_xmui_t *xmui, int perm){
    fdr_config_t *config = fdr_config();
    // log_info("===> reset door open relay\n");
    
    if(perm == FDR_PERM_AUTHEN){
        struct timeval tv = {config->door_open_latency, 0};
        bisp_hal_enable_relay(0, 1);    // open door
        event_add(&xmui->state.ev_door, &tv);
    }
}*/

static void xmui_uirec_show(fdr_cp_xmui_t *xmui, const char *userid, int perm){
    // fdr_cp_t *fcp = xmui->fcp;
    //fdr_config_t *config = fdr_config();
    FILE    *fp;
    size_t  sz;

    sprintf(xmui->image_path, "%s/%s.bmp", user_image_path, userid);
    fp =  fopen(xmui->image_path, "rb");
    if(fp == NULL){
        log_warn("open user image fail:%s\n", xmui->image_path);
        return ;
    }
    
    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    if(xmui->buflen < sz){
        if(xmui->image != NULL){
            fdr_free(xmui->image);
        }
        
        xmui->image = fdr_malloc(sz);
        if(xmui->image == NULL){
            log_warn("alloc image buffer fail!\n");
            xmui->buflen = 0;
            fclose(fp);
            return ;
        }

        xmui->buflen = sz;
    }

    if(fread(xmui->image, 1, sz, fp) != sz){
        log_warn("read user image fail!\n");
        fclose(fp);
        return ;
    }
    fclose(fp);

    if(NULL == xmui->last || strcmp(xmui->last->userid, userid)) {
        bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_FACEREC);
    }
    bisp_gui_show_fdrwin(xmui->image+54, 160, 480, perm);
}

#include "hal_log.h"
static void xmui_state_fdr_proc(fdr_cp_xmui_t *xmui, fdr_cp_aie_result_t *cf){
    fdr_cp_t *fcp = xmui->fcp;
    fdr_cp_user_t  *user;
    fdr_config_t *config = fdr_config();
    bisp_aie_frval_t    *frv;
    face_track_t        *ft;

    cpxmui_state_t *state = &xmui->state;
    
    int64_t octime;
    int64_t seqnum;
    struct timeval tv;

    int find = 0;

    float score = 0;
    int i, idx;
    int show_mode;
    int saved = 0;

    log_info("xmui_state_fdr_proc => state %d\n", state->state);

    // in other status receive recognized message, just ignore it.
    if(state->state != CPXMUI_STATE_FACEREC){
        bisp_aie_release_frame(cf->frame);
        cf->frame = NULL;
        
        fdr_cp_release_aieresult(fcp, cf);

        return ;
    }

    gettimeofday(&tv, NULL);
    octime = tv.tv_sec;
    octime = (octime * 1000) + (tv.tv_usec / 1000);

    face_tracking(xmui, octime, cf->rnum, cf->rfs);

    for(i = 0; i < CPXMUI_TRACK_MAX; i++){
        ft = &xmui->tracks[i];

        // skip empty slot
        if(ft->faceid == -1)
            continue;

        // skip not existed face in this time
        if(ft->current != octime){
            // drop timeout : 2 seconds tracked face
            if(octime > (ft->current + 2000)){
                ft->faceid = -1;
                ft->start = 0;
                xmui->track_num --;
            }

            continue;
        }
#if 1
	// already recognized face
        if(ft->stranger == 0){
            // FIX: Fast switch problem, faceid keep same, but poeple switched
            if(xmui->last != NULL){
                float score;
                frv = &cf->rfs[ft->idx];
                score = bisp_aie_matchfeature(frv->feature.vector, xmui->last->feature, BISP_AIE_FDRVECTOR_MAX);
		hal_info("score - thresh_hold: %f - %f\r\n", score, config->recognize_thresh_hold);
                if(score >= config->recognize_thresh_hold){
                    // already recognized people in this image
                    //xmui_uirec_show(xmui, ft->userid, ft->show_mode);
                    //xmui_door_timerout_reset(xmui, ft->perm);
                    //state_settimer(state, config->show_tips_latency);
                    //find = 1;
                    //continue;
                } else {
		    hal_info("reset trace face\r\n");
		    xmui->last = NULL;
		}
            }
        }
#else
	// already recognized face
        if(ft->stranger == 0){
            // already recognized people in this image
            xmui_uirec_show(xmui, ft->userid, ft->show_mode);
            xmui_door_timerout_reset(xmui, ft->perm);
            state_settimer(state, config->show_tips_latency);
            find = 1;
            continue;
        }
#endif
        if(ft->idx > cf->rnum){
            log_warn("xmui_state_fdr_proc => tracked face idx overflow:%d-%d\n", cf->rnum, ft->idx);
            continue;
        }

        frv = &cf->rfs[ft->idx];

        idx = fdr_cp_repo_match(fcp->repo, frv->feature.vector, config->recognize_thresh_hold, &score);
        if((idx >= 0) && (idx < fcp->repo->num)){
            user = &fcp->repo->users[idx];
            // update have recognized in track
            strncpy(ft->userid, user->userid, FDR_DBS_USERID_SIZE);
            ft->stranger = 0;

            // logger info;
            seqnum = fdr_logger_append(octime, &frv->rect, 0, score, user->userid);
            
            fdr_mp_report_logger(octime, seqnum, score, user->userid, user->name);

            show_mode = user->perm ? BISP_GUI_FDRWIN_PERM : BISP_GUI_FDRWIN_NOPERM;
            if(user->expired < tv.tv_sec){
                log_info("expire: %u - now: %u\r\n", user->expired, tv.tv_sec);
                show_mode = BISP_GUI_FDRWIN_EXPIRED;  // expired
            }

            if(show_mode == BISP_GUI_FDRWIN_PERM){
                // open the door
                xmui_door_enable(xmui, show_mode); 
            }

            ft->show_mode = show_mode;
            
            // show image of people
            xmui_uirec_show(xmui, user->userid, show_mode);
            state_settimer(state, config->show_tips_latency);
            // set last recognized face
            xmui->last = user; 
            
            saved = 1;
            find = 1;
        }else{
            if (ft->start == 0){   // new stranger
                log_info("**stranger tracked in %d\r\n", frv->id);
                seqnum = fdr_logger_append(octime, &frv->rect, 0, 0, NULL);

                fdr_mp_report_logger(octime, seqnum, 0.0,  NULL, NULL);

                ft->start = octime;
                saved = 1;
            }//else{
             //   log_info("**stranger already tracked %d\r\n", frv->id);
            //}
        }
    }

    if(!find){
        // show stranger
        //log_info("show stranger\r\n");
        bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_STRANGER);
        state_settimer(state, config->show_tips_latency);
        //log_info("show stranger end\r\n");
    }

    if(saved){
        fdr_logger_saveimage(octime, cf->frame, -1, -1);
//        ctrller_save_image(fc, octime, cf->frame); 
    }
    
    bisp_aie_release_frame(cf->frame);
    cf->frame = NULL;

    fdr_cp_release_aieresult(fcp, cf);
}


static void xmui_state_qrc_proc(fdr_cp_xmui_t *xmui, const char *qrcode){
    // fdr_cp_t *fcp = xmui->fcp;
    fdr_qrcode_visitor_t visitor;
    cpxmui_state_t *state = &xmui->state;
    fdr_dbs_user_t *user;
    fdr_config_t *config = fdr_config();
    char userid[32];
    int duration;
    size_t len;
    int rc;

    log_info("xmui_state_qrc_proc => state %d, len: %d\n", state->state, strlen(qrcode));

    switch (state->state){
    case CPXMUI_STATE_ACTIVATE:
    case CPXMUI_STATE_FACEREC:
    case CPXMUI_STATE_CONFIG:
    case CPXMUI_STATE_VISITOR_KBINPUT:
        log_warn("xmui_state_qrc_proc => wrong state:%d\n", state->state);
        break;
    
    case CPXMUI_STATE_VISITOR_QRSCAN:
        len = sizeof(visitor);
        rc = base58_decode(qrcode, strlen(qrcode), (uint8_t*)&visitor, len);
        // rc = b58_to_binary(qrcode, strlen(qrcode), &visitor, &len);
        if(rc > 0){
            log_warn("xmui_state_qrc_proc => qrcode decode fail\n");
            state_settimer(state, config->visitor_duration);
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_VI_FAILURE);
            return ;
        }

        rc = fdr_qrcode_verify(&visitor, sizeof(visitor) - FDR_CHECKSUM_MAX, visitor.checksum);
        if((rc != 0) || (memcmp(visitor.magic, qrmagic_visitor, 4) != 0)){
            log_warn("xmui_state_qrc_proc => qrcode verify fail : %d!\n", rc);
            state_settimer(state, config->visitor_duration);
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_VI_FAILURE);
            return ;
        }
        
        duration = ntohl(visitor.duration);
        if(duration < time(NULL)){
            log_warn("xmui_state_qrc_proc => visitor qrcode timeout : %s", ctime((const time_t*)&duration));
            state_settimer(state, config->visitor_duration);
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_VI_FAILURE);
            return ;
        }

        len = 32;
        base58_encode(visitor.userid, 16, userid, &len);
        //binary_to_b58(visitor.userid, 16, &userid, &len);
        rc = fdr_dbs_user_lookup(userid, &user);
        fdr_free(userid);
        if((rc == 0) && (user->perm > 0) && (user->exptime > time(NULL))){
            fdr_dbs_user_free(user);        // release user data
            state_settimer(state, config->show_tips_latency);
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_VI_SUCCESS);

            // door open
            xmui_door_enable(xmui, 1);
        }else{
            if(rc == 0){
                // release user data
                fdr_dbs_user_free(user);
            }
            state_settimer(state, config->visitor_duration);
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_VI_FAILURE);
        }
        break;
    
    default:
        log_warn("xmui_state_qrc_proc => wrong state:%d\n", state->state);
        break;
    }
}

static void xmui_state_qrs_proc(fdr_cp_xmui_t *xmui, const char *qrcode){
    // fdr_cp_t *fcp = xmui->fcp;
    fdr_qrcode_simple_t visitor;
    cpxmui_state_t *state = &xmui->state;
    fdr_config_t *config = fdr_config();
    time_t expired;
    int len;
    int rc;

    log_info("xmui_state_qrs_proc => state %d, len: %d\n", state->state, strlen(qrcode));

    switch (state->state){
    case CPXMUI_STATE_ACTIVATE:
    case CPXMUI_STATE_FACEREC:
    case CPXMUI_STATE_CONFIG:
    case CPXMUI_STATE_VISITOR_KBINPUT:
        log_warn("xmui_state_qrs_proc => wrong state:%d\n", state->state);
        break;
    
    case CPXMUI_STATE_VISITOR_QRSCAN:
        len = sizeof(visitor);
        rc = base58_decode(qrcode, strlen(qrcode), (uint8_t*)&visitor, len);
        if(rc > 0){
            log_warn("xmui_state_qrc_proc => qrcode decode fail\n");
            state_settimer(state, config->visitor_duration);
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_VI_FAILURE);
            return ;
        }

        rc = fdr_dbs_guest_exist((const char *)visitor.code, &expired);

        if((rc == 0) && (expired > time(NULL))){
            state_settimer(state, config->show_tips_latency);
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_VI_SUCCESS);

            // door open
            xmui_door_enable(xmui, 1);
        }else{
            state_settimer(state, config->visitor_duration);
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_VI_FAILURE);
        }
        break;
    
    default:
        log_warn("xmui_state_qrs_proc => wrong state:%d\n", state->state);
        break;
    }
}


//  key drive state machine process function
static void xmui_state_key_proc(fdr_cp_xmui_t *xmui, int status){
    // fdr_cp_t *fcp = xmui->fcp;
    cpxmui_state_t *state = &xmui->state;
    fdr_config_t *config = fdr_config();

    log_info("xmui_state_key_proc => state %d, keytouch:%d\n", state->state, status);

    switch(state->state){
    case CPXMUI_STATE_ACTIVATE:
        break;

    case CPXMUI_STATE_FACEREC:
        if(status == FDR_CP_EVT_KEYDOWN){
            state->keytime = time(NULL);
            state_settimer(state, config->config_active_duration);
            break; 
        }

        if (status == FDR_CP_EVT_KEYUP){
            state_settimer(state, 0);
            if(state->keytime == 0)
                break;

            state->keytime = 0;
            state->state = CPXMUI_STATE_VISITOR_QRSCAN;
            state_settimer(state, config->visitor_duration);
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_VI_SCAN);
            bisp_aie_set_mode(BISP_AIE_MODE_QRCODE);
            break;
        }
        break;

    case CPXMUI_STATE_CONFIG:
    case CPXMUI_STATE_VISITOR_KBINPUT:
    case CPXMUI_STATE_VISITOR_QRSCAN:
        if(status == FDR_CP_EVT_KEYDOWN){
            state_settimer(state, 0);
            state->state = CPXMUI_STATE_FACEREC;
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_FACEREC);
            bisp_aie_set_mode(BISP_AIE_MODE_FACE);
        }
        break;

    default:
        state->state = CPXMUI_STATE_FACEREC;
        state_settimer(state, 0);
        bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_FACEREC);
        bisp_aie_set_mode(BISP_AIE_MODE_FACE);
        break;
    }
}


void fdr_cp_event_proc(struct bufferevent *bev, void *ctx){
    fdr_cp_t *fcp = (fdr_cp_t *)ctx;
    fdr_cp_xmui_t *xmui = (fdr_cp_xmui_t *)fcp->handle;
    fdr_cp_event_t  cev;

    struct evbuffer *input = bufferevent_get_input(bev);

    while(evbuffer_get_length(input) >= sizeof(cev)){
        evbuffer_remove(input, &cev, sizeof(cev));

        log_info("fdr_cp_event_proc receive event : %d\n", cev.evtype);
        
        switch(cev.evtype){
        case FDR_CP_EVT_ACTIVATE:
            xmui->state.state = CPXMUI_STATE_FACEREC;
            bisp_aie_set_mode(BISP_AIE_MODE_FACE);
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_FACEREC);
            break;

        case FDR_CP_EVT_KEYDOWN:
        case FDR_CP_EVT_KEYUP:
            xmui_state_key_proc(xmui, cev.evtype);
            break;
            
        case FDR_CP_EVT_QRCODE:
            xmui_state_qrc_proc(xmui, cev.evqrstr);
            fdr_free(cev.evqrstr);
            break;

        case FDR_CP_EVT_QRCODE_SIMPLE:
            xmui_state_qrs_proc(xmui, cev.evqrstr);
            fdr_free(cev.evqrstr);
            break;

        case FDR_CP_EVT_FACEDR:
            xmui_state_fdr_proc(xmui, (fdr_cp_aie_result_t*)cev.evdata);
            break;

        case FDR_CP_EVT_USER_DELETE:
            // for simple, just reset last to NULL
            xmui->last = NULL;
            fdr_cp_repo_delete(fcp->repo, (char *)cev.evdata);
            fdr_free(cev.evdata);
            break;

        case FDR_CP_EVT_USER_UPDATE:
            // for simple, just reset last to NULL
            xmui->last = NULL;

            fdr_cp_repo_update(fcp->repo, (const char*)cev.evdata);
            fdr_free(cev.evdata);
            break;
        
        default:
            log_warn("fdr_cp_event_proc => receive unkown message:%d\n", cev.evtype);
            break;
        }
    }
}

// door timer proc
static void xmui_door_timer_proc(int fd, short events, void *args){
    fdr_cp_xmui_t *xmui = (fdr_cp_xmui_t *)args;

    bisp_hal_enable_relay(0, 0); // close door

    // clear timeout last user
    xmui->last = NULL; 
}

static void xmui_state_timer_proc(int fd, short events, void *args){
    fdr_cp_xmui_t *xmui = (fdr_cp_xmui_t *)args;
    // fdr_cp_t *fcp = xmui->fcp;
    cpxmui_state_t *state = &xmui->state;
    fdr_config_t *config = fdr_config();
    time_t duration;

    log_info("xmui_state_timer_proc ==> state %d, receive\n", state->state);

    switch(state->state){
    case CPXMUI_STATE_ACTIVATE:
        bisp_aie_set_mode(BISP_AIE_MODE_NONE);

        if(config->cloud_connected){
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_AC_CONNECTED);
        }else{
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_AC_UNCONNECTED);
        }

        state_settimer(&xmui->state, config->show_tips_latency);
        break;

    case CPXMUI_STATE_FACEREC:
        // long press key timeout for activate configuration 
        duration = time(NULL) - state->keytime;
        if((state->keytime == 0) || (duration < 0)){
            // GUI timer out, reset it 
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_FACEREC);
            break;
        }

        if((duration >= 0) && (duration < config->config_active_duration)){
            state_settimer(state, config->config_active_duration - duration);
            break;
        }

        state->keytime = 0;
        state->state = CPXMUI_STATE_CONFIG;
        state_settimer(state, config->config_duration);

        bisp_aie_set_mode(BISP_AIE_MODE_NONE);

        if(config->device_mode == 0){
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_CF_LOCAL);
        }else{
            if(config->cloud_connected){
                bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_CF_CLOUD_CONNECTED);
            }else{
                bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_CF_CLOUD_UNCONNECTED);
            }
        }
        break;

    case CPXMUI_STATE_CONFIG:
    case CPXMUI_STATE_VISITOR_QRSCAN:
    case CPXMUI_STATE_VISITOR_KBINPUT:
        state_settimer(state, 0);
        state->state = CPXMUI_STATE_FACEREC;
        bisp_aie_set_mode(BISP_AIE_MODE_FACE);
        bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_FACEREC);
        break;
        

    default:
        log_warn("xmui_state_timer_proc => unkown state %d\n",state->state);
        break;
    }
}


int fdr_cp_create_instance(fdr_cp_t *fcp){
    fdr_cp_xmui_t   *xmui;
    fdr_config_t *config = fdr_config();
    int i;

    xmui = fdr_malloc(sizeof(fdr_cp_xmui_t));
    if(xmui == NULL){
        log_warn("fdr_cp_create_instance => no more memory\n");
        return -1;
    }
    memset(xmui, 0, sizeof(fdr_cp_xmui_t));

    // init state
    event_assign(&xmui->state.ev_door,    fcp->base, -1, EV_TIMEOUT, xmui_door_timer_proc, xmui);
    event_assign(&xmui->state.ev_state,   fcp->base, -1, EV_TIMEOUT, xmui_state_timer_proc, xmui);

    // init track
    xmui->track_num = 0;
    for(i = 0; i < CPXMUI_TRACK_MAX; i++){
        xmui->tracks[i].faceid = -1;
    }

    if(config->activation){
        xmui->state.state = CPXMUI_STATE_FACEREC;

        bisp_aie_set_mode(BISP_AIE_MODE_FACE);
        bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_FACEREC);
    }else{
        xmui->state.state = CPXMUI_STATE_ACTIVATE;

        bisp_aie_set_mode(BISP_AIE_MODE_NONE);
        if(config->cloud_connected){
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_AC_CONNECTED);
        }else{
            bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_AC_UNCONNECTED);
        }

        state_settimer(&xmui->state, config->show_tips_latency);
    }

    xmui->fcp = fcp;
    fcp->handle = xmui;

    return 0;
}

int fdr_cp_destroy_instance(fdr_cp_t *fcp){
    fdr_cp_xmui_t   *xmui = (fdr_cp_xmui_t*)fcp->handle;

    fdr_free(xmui);
    fcp->handle = NULL;

    return 0;
}

