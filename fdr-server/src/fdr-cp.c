/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr-cp.h"
#include "fdr.h"

#include "fdr-logger.h"
#include "fdr-qrcode.h"
#include "fdr-path.h"
#include "fdr-dbs.h"

#include "bisp_gui.h"
#include "bisp_aie.h"
#include "bisp_hal.h"

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

static void fdr_cp_user_free(fdr_cp_user_t *user);
static void fdr_cp_repo_free(fdr_cp_repo_t *repo);
static int fdr_cp_repo_load(fdr_cp_repo_t *repo);

static fdr_cp_t *fdr_cp_get(){
    static fdr_cp_t cp = {};

    return &cp;
}

static void fdr_cp_fini_aieresult(fdr_cp_t *cp){
    fdr_cp_aie_result_t *rslt;
    
    while(!list_empty(&cp->aie_results)){
        rslt = list_first_entry(&cp->aie_results, fdr_cp_aie_result_t, node);
        list_del(&rslt->node);

        fdr_free(rslt);
    }
}

static int fdr_cp_init_aieresult(fdr_cp_t *cp){
    fdr_cp_aie_result_t *rslt;
    int i;

    INIT_LIST_HEAD(&cp->aie_results);

    for(i = 0; i < FDR_CP_AIERESULTS; i++){
        rslt = fdr_malloc(sizeof(fdr_cp_aie_result_t));
        if(rslt == NULL){
            fdr_cp_fini_aieresult(cp);
            return -1;
        }
        memset(rslt, 0, sizeof(fdr_cp_aie_result_t));

        list_add(&rslt->node, &cp->aie_results);
    }

    return 0;
}

static void *fdr_cp_proc(void *args){
    fdr_cp_t *fcp = (fdr_cp_t *)args;
    int rc;
    
    fcp->base = event_base_new();
    if (fcp->base == NULL)
    {
        log_warn("fdr_cp_proc => create event_base failed!\n");
        return (void*)-1;
    }

    rc = bufferevent_pair_new(fcp->base, BEV_OPT_THREADSAFE, fcp->bepair);
    if(0 != rc){
        log_warn("fdr_cp_proc  => create event pair fail:%d\n", rc);
        event_base_free(fcp->base);
        return (void*)-1;
    }

    bufferevent_enable(fcp->bepair[1], EV_READ);
    bufferevent_enable(fcp->bepair[0], EV_WRITE);
    bufferevent_setcb(fcp->bepair[1], fdr_cp_event_proc, NULL, NULL, fcp);

    log_info("activation : %d\n", fdr_config()->activation);

    if(fdr_cp_create_instance(fcp) != 0){
        log_warn("fdr_cp_proc  => create instance fail:%d\n", rc);
        bufferevent_free(fcp->bepair[0]);
        bufferevent_free(fcp->bepair[1]);
        event_base_free(fcp->base);
        return (void*)-1;
    }

    log_info("fdr_cp_proc => controller plane looping ... \n");
    fcp->status = 1;

    event_base_loop(fcp->base, EVLOOP_NO_EXIT_ON_EMPTY);

    fdr_cp_destroy_instance(fcp);

    bufferevent_free(fcp->bepair[0]);
    bufferevent_free(fcp->bepair[1]);
    
    event_base_free(fcp->base);

    fdr_cp_repo_free(fcp->repo);
    fdr_free(fcp->repo);

    fdr_cp_fini_aieresult(fcp);

    pthread_mutex_destroy(&fcp->lock);

    return NULL;
}


int fdr_cp_init(){
    fdr_cp_t    *cp = fdr_cp_get();
    fdr_config_t *config = fdr_config();
    int rc;

    memset(cp, 0, sizeof(fdr_cp_t));

    log_info("fdr_cp_create => user_max %d, size %d\n", config->user_max, sizeof(fdr_cp_repo_t) + config->user_max*sizeof(fdr_cp_user_t));

    cp->repo = (fdr_cp_repo_t*)fdr_malloc(sizeof(fdr_cp_repo_t) + config->user_max*sizeof(fdr_cp_user_t));
    if(cp->repo == NULL){
        log_warn("fdr_cp_create => alloc memory for repo fail!\n");
        return -1;
    }
    memset(cp->repo, 0, sizeof(fdr_cp_repo_t) + config->user_max*sizeof(fdr_cp_user_t));
    cp->repo->max = config->user_max;

    rc = fdr_cp_repo_load(cp->repo);
    if(rc < 0){
        log_warn("fdr_cp_create => load repo fail!\n");
        fdr_free(cp->repo);
        return -1;
    }

    rc = fdr_cp_init_aieresult(cp);
    if(rc != 0){
        log_warn("fdr_cp_create => fdr_cp_init_aieresult fail:%d\n", rc);
        fdr_cp_repo_free(cp->repo);
        fdr_free(cp->repo);

        return rc;
    }

    pthread_mutex_init(&cp->lock, NULL);

    rc = pthread_create(&cp->thread, NULL, fdr_cp_proc, cp);
    if(rc != 0){
        log_warn("fdr_cp_create => create thread fail:%d\n", rc);
        fdr_cp_repo_free(cp->repo);
        fdr_free(cp->repo);
        fdr_cp_init_aieresult(cp);

        return rc;
    }

    while(!cp->status){
        usleep(10);
    }

    return 0;
}

int fdr_cp_fini(){
    fdr_cp_t    *cp = fdr_cp_get();

    event_base_loopexit(cp->base, NULL);
    return 0;
}


fdr_cp_aie_result_t *fdr_cp_acquire_aieresult(fdr_cp_t *cp){
    fdr_cp_aie_result_t *rslt = NULL;

    pthread_mutex_lock(&cp->lock);

    if(!list_empty(&cp->aie_results)){
        rslt = list_first_entry(&cp->aie_results, fdr_cp_aie_result_t, node);
        list_del(&rslt->node);
    }

    pthread_mutex_unlock(&cp->lock);

    return rslt;
}

void fdr_cp_release_aieresult(fdr_cp_t *cp, fdr_cp_aie_result_t *rslt){

    pthread_mutex_lock(&cp->lock);
    
    list_add_tail(&rslt->node, &cp->aie_results);

    pthread_mutex_unlock(&cp->lock);
}

int fdr_cp_fdrdispatch(int dnum, void *dfs, int rnum, bisp_aie_frval_t *rfs, void *frame){
    fdr_cp_t    *cp = fdr_cp_get();
    fdr_cp_aie_result_t *ar;
    fdr_cp_event_t sev;

    if(rnum > BISP_AIE_FRFACEMAX){
        log_warn("fdr_cp_fdrdispatch => fr face beyond max:%d!\n", rnum);
        return -1;
    }

    ar = fdr_cp_acquire_aieresult(cp);
    if(ar == NULL){
        log_warn("fdr_cp_fdrdispatch => alloc memory for result fail!\n");
        return -1;
    }

    // TODO : add detech support later
    ar->dnum = dnum;
    ar->dfs = dfs;

    // copy rec result
    ar->rnum = rnum;
    memcpy(ar->rfs, rfs, sizeof(bisp_aie_frval_t)*rnum);

    // get frame image data
    ar->frame = frame;

    sev.evtype = FDR_CP_EVT_FACEDR;
    sev.evdata = ar;

    if(bufferevent_write(cp->bepair[0], &sev, sizeof(sev))) {
        log_warn("fdr_cp_fdrdispatch => bufferevent_write failed\r\n");
        return -1;
    }

    return 0;
}

int fdr_cp_qrcdispatch(const char *qrcode, int len){
    fdr_cp_t    *cp = fdr_cp_get();
    fdr_cp_event_t  sev;
    char *qrc;

    log_info("fdr_cp_qrcdispatch => %d, %s\n", len, qrcode);
    
    // qrc = strdup(qrcode);
    qrc = fdr_malloc(strlen(qrcode) + 1);
    if(qrc == NULL){
        log_warn("fdr_cp_qrcdispatch => strdup qrcode fail!\n");
        return -1;
    }
    strcpy(qrc, qrcode);

    sev.evtype = FDR_CP_EVT_QRCODE;
    sev.evqrstr = qrc;

    if(bufferevent_write(cp->bepair[0], &sev, sizeof(sev))) {
        log_warn("fdr_cp_qrcdispatch => bufferevent_write failed\r\n");
        return -1;
    }

    return 0;
}

int fdr_cp_keydispatch(int status){
    fdr_cp_t    *cp = fdr_cp_get();
    fdr_cp_event_t  sev;

    status = (status == 0) ? FDR_CP_EVT_KEYUP :  FDR_CP_EVT_KEYDOWN;

    sev.evtype = status;
    sev.evkeystate = status;

    bufferevent_write(cp->bepair[0], &sev, sizeof(sev));

    return 0;
}

int fdr_cp_activate(int type){
    fdr_cp_t    *cp = fdr_cp_get();
    fdr_cp_event_t  sev;

    sev.evtype = FDR_CP_EVT_ACTIVATE;
    sev.evvalue = type;

    bufferevent_write(cp->bepair[0], &sev, sizeof(sev));

    return 0;
}

int fdr_cp_usrdispatch(int type, const char *userid){
    fdr_cp_t    *cp = fdr_cp_get();
    fdr_cp_event_t  sev;

    log_info("handle fdr_cp_usrdispatch: %s\r\n", userid);
    sev.evtype = type;
    sev.evdata = fdr_malloc(strlen(userid) + 1);
    if(sev.evdata == NULL){
        log_warn("fdr_cp_usrdispatch  => alloc memory for user event fail!\n");
        return -1;
    }
    strcpy(sev.evdata, userid);

    bufferevent_write(cp->bepair[0], &sev, sizeof(sev));

    return 0;
}

static void fdr_cp_user_free(fdr_cp_user_t *user){
    if(user->userid != NULL){
        fdr_free(user->userid);
        user->userid = NULL;
    }

    if(user->name != NULL){
        fdr_free(user->name);
        user->name = NULL;
    }

    if(user->desc != NULL){
        fdr_free(user->desc);
        user->desc = NULL;
    }

    if(user->others != NULL){
        fdr_free(user->others);
        user->others = NULL;
    }

    if(user->feature != NULL){
        fdr_free(user->feature);
        user->feature = NULL;
    }
}

int fdr_cp_repo_update(fdr_cp_repo_t *repo, const char *userid){
    fdr_dbs_user_t *du = NULL;
    fdr_cp_user_t  *cu = NULL;
    int find = 0;
    int rc;
    int i;

    for(i = 0; i < repo->num; i++){
        cu = &repo->users[i];
        if(strcmp(cu->userid, userid) == 0){
            find = 1;
            break;
        }
    }

    if(!find){
        if(repo->num >= repo->max){
            log_warn("fdr_cp_repo_update => user overflow\n");
            return -1;
        }

        // append to end
        cu = &repo->users[repo->num];
    }
    
    rc = fdr_dbs_user_lookup(userid, &du);
    if(rc != 0){
        log_warn("fdr_cp_repo_update => user not exist\n");
        return -1;
    }

    rc = fdr_dbs_feature_lookup(userid, &du->feature, &du->feature_len);
    if(rc != 0){
        log_info("fdr_cp_repo_update => user feature  exist\n");
        
        du->feature = NULL;
        du->feature_len = 0;
    }

    if(find){
        fdr_free(cu->name);
        fdr_free(cu->desc);

        if(cu->others != NULL){
            fdr_free(cu->others);
        }
    }else{
        cu->userid = du->userid;
        du->userid = NULL;
    }

    cu->name = du->name;
    du->name = NULL;

    cu->desc = du->desc;
    du->desc = NULL;
    
    cu->others = du->others;
    du->others = NULL;

    cu->perm = du->perm;
    cu->expired = du->exptime;
    
    if((cu->feature == NULL) || (du->feature != NULL)){
        
        if(cu->feature != NULL){
            fdr_free(cu->feature);
        }

        cu->feature = du->feature;
        du->feature = NULL;
    }
    
    fdr_dbs_user_free(du);

    if(!find){
        repo->num++;
    }

    return 0;
}

int fdr_cp_repo_delete(fdr_cp_repo_t *repo, const char *userid){
    fdr_cp_user_t *u;
    int i;

    for(i = 0; i < repo->num; i++){
        u = &repo->users[i];
        if(strcmp(u->userid, userid) == 0){

            // log_info("delete user %s\n", userid);
            // free delete user info
            fdr_cp_user_free(u);

            if(repo->num == repo->max){
                repo->num --;   // last one, only set point;
            }else{
                memmove(&repo->users[i], &repo->users[i+1], sizeof(fdr_cp_user_t)*(repo->num - i));
                repo->num --;
            }

            // clear user info point
            u = &repo->users[repo->num];
            u->userid = NULL;
            u->name = NULL;
            u->desc = NULL;
            u->feature = NULL;
            
            break;
        }
    }

    return 0;
}

static void fdr_cp_repo_free(fdr_cp_repo_t *repo){
    fdr_cp_user_t *user;
    int i;

    for(i = 0; i < repo->num; i++){
        user = &repo->users[i];
        fdr_cp_user_free(user);
    }
}

static inline int repo_append(fdr_dbs_user_t *user, void *handle){
    fdr_cp_repo_t *repo = (fdr_cp_repo_t *)handle;
    fdr_cp_user_t *cu;

    if(repo->num >= repo->max){
        log_warn("repo_append => repo will overflow %d -> %d\n", repo->num, repo->max);
        return -1;
    }

    if(user->feature == NULL){
        log_warn("repo_append => user without feature\n");
        return -1;
    }

    cu = &repo->users[repo->num];
    
    cu->userid = user->userid;
    user->userid = NULL;

    cu->name = user->name;
    user->name = NULL;

    cu->desc = user->desc;
    user->desc = NULL;

    cu->others = user->others;
    user->others = NULL;

    cu->perm = user->perm;
    cu->expired = user->exptime;
    
    cu->feature = user->feature;
    user->feature = NULL;

    repo->num++;

    return 0;
}

static int fdr_cp_repo_load(fdr_cp_repo_t *repo){
    int rc;

    log_info("fdr_cp_repo_load => \n");

    rc = fdr_dbs_user_foreach(repo_append, repo);
    if(rc < 0){
        log_warn("fdr_cp_repo_load => load repo fail %d\n", rc);
    }else{
        log_info("fdr_cp_repo_load => load users, count : %d\n", rc);
    }

    return rc;
}

int fdr_cp_repo_match(fdr_cp_repo_t *repo, float *feature, float thresh_hold, float *score)
{
    fdr_cp_user_t *user;
    float probmax = .0f;
    float prob;
    int i;
    int rc = -1;

    log_info("face repo num: %d\r\n", repo->num);
    for (i = 0; i < repo->num; i++){
        user = &repo->users[i];

        if(user->feature == NULL)
            continue;

        prob = bisp_aie_matchfeature(feature, user->feature, BISP_AIE_FDRVECTOR_MAX);
        //log_info("match result => userid %s: score %f\n", user->userid, prob);
        if ((prob > thresh_hold) && (prob > probmax)){
            probmax = prob;
            *score =100 * prob;
            rc = i;
        }
    }

#ifdef TEST
    if(probmax > thresh_hold){
        log_info("fdr_cp_repo_match => user %s score %f\n", repo->users[rc].name, *score);
    }
#endif

    return rc;
}

float bisp_aie_matchfeature(float *feature1, float *feature2, int len){
    float score = 0;
    float dot = 0;
    float norm1 = 0;
    float norm2 = 0;
    int i;

    for (i = 0; i < len; i++)
    {
        dot = dot + feature1[i] * feature2[i];
        norm1 = norm1 + feature1[i] * feature1[i];
        norm2 = norm2 + feature2[i] * feature2[i];
    }

    norm1 = sqrt(norm1);
    norm2 = sqrt(norm2);
    score = dot / (norm1*norm2);

    return score;
}

