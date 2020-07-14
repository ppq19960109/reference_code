/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __FDR_CP_H__
#define __FDR_CP_H__

#include "bisp_aie.h"
#include "fdr-dbs.h"

#include "list.h"

#include <time.h>
#include <pthread.h>

#define FDR_CP_RS485_BUFFER            128
#define FDR_CP_AIERESULTS               4

// cache for face match
typedef struct fdr_cp_user{
    char *userid;
    char *name;
    char *desc;
    char *others;
    int  perm;

    time_t expired;
    float *feature;
}fdr_cp_user_t;

// cache for face match
typedef struct fdr_cp_repo{
    int max;
    int num;

    fdr_cp_user_t users[0];
}fdr_cp_repo_t;

// event message type declerations
typedef enum{
    FDR_CP_EVT_ACTIVATE = 0,   // activate from management plane
    FDR_CP_EVT_KEYDOWN,        // key press down
    FDR_CP_EVT_KEYUP,          // key press up
    FDR_CP_EVT_KEYVALUE,       // key value in keyboard mode
    FDR_CP_EVT_FACEDR,         // face detect & recognize
    FDR_CP_EVT_QRCODE,         // qrcode scan
    FDR_CP_EVT_QRCODE_SIMPLE,  // qrcode scan, simple code
    FDR_CP_EVT_RS485,          // rs485 receive data
    FDR_CP_EVT_WIEGAND,        // wiegand read RF card message
    FDR_CP_EVT_GPIO,           // GPIO input alarm
    FDR_CP_EVT_TOUCH,          // touch screen
    FDR_CP_EVT_USER_DELETE,    // delete user in repo
    FDR_CP_EVT_USER_UPDATE,    // update user in repo
}fdr_cp_evtype_e;

typedef struct {
    struct list_head node;

    int dnum;       // number of detected face
    void *dfs;      // detected face infomation

    int rnum;       // number of recognized face
    bisp_aie_frval_t  rfs[BISP_AIE_FRFACEMAX]; // face feature vector and postions

    void *frame;    // data of image
}fdr_cp_aie_result_t;

typedef struct {
    int evtype;       // type of this message

    union{
        void *evdata;     // message body is define by caller
        int   evkeystate; // key state -> 0:PRESS DOWN, 1:PRESS UP
        int   evkeyvalue; // key value -> number & alphabeta
        char *evqrstr;    // scaned qrcode string
        int   evvalue;    //
    };
}fdr_cp_event_t;

struct bufferevent;
struct event_base;

// controll plane instance
typedef struct fdr_cp{
    // thread for main event loop
    pthread_t thread;

    void *handle;   // implement handle

    pthread_mutex_t lock;
    struct list_head aie_results;

    // user info in memory, for face match
    fdr_cp_repo_t  *repo;

    // event loop
    struct event_base* base;
    int status;

    struct bufferevent *bepair[2];
}fdr_cp_t;

int fdr_cp_init();
int fdr_cp_fini();

fdr_cp_aie_result_t *fdr_cp_acquire_aieresult(fdr_cp_t *cp);
void fdr_cp_release_aieresult(fdr_cp_t *cp, fdr_cp_aie_result_t *rslt);

// set as callback function by bisp
int fdr_cp_fdrdispatch(int dnum, void *dfs, int rnum, bisp_aie_frval_t *rfs, void *frame);
int fdr_cp_keydispatch(int status);
int fdr_cp_qrcdispatch(const char *qrcode, int len);
int fdr_cp_usrdispatch(int type, const char *userid);
//int fdr_cp_activate(int type);


// for internal implementations
int fdr_cp_create_instance(fdr_cp_t *fcp);
int fdr_cp_destroy_instance(fdr_cp_t *fcp);
void fdr_cp_event_proc(struct bufferevent *bev, void *ctx);

int fdr_cp_repo_delete(fdr_cp_repo_t *repo, const char *userid);
int fdr_cp_repo_update(fdr_cp_repo_t *repo, const char *userid);

int fdr_cp_repo_match(fdr_cp_repo_t *repo, float *feature, float thresh_hold, float *score);

#endif // __FDR_CP_H__
