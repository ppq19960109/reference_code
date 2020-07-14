/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __FDR_MP_H__
#define __FDR_MP_H__

#include "fdr-dbs.h"
#include "bisp_aie.h"
#include "bcp.h"

#include <inttypes.h>
#include <pthread.h>

#define FDR_MAX_FEATURE_VECTOR      BISP_AIE_FDRVECTOR_MAX

// message between mp & cp
typedef struct {
    int64_t octime;
    int64_t seqnum;
    float   score;
    char userid[FDR_DBS_USERID_SIZE+1];
    char username[FDR_DBS_USERNAME_SIZE + 1];
}fdr_mp_rlog_t;

struct event_base;

struct fdr_mp_local;
typedef struct fdr_mp_local fdr_mp_local_t;

struct fdr_mp_cloud;
typedef struct fdr_mp_cloud fdr_mp_cloud_t;

struct fdr_mp;
typedef struct fdr_mp fdr_mp_t;

// create & destroy fdr server
int fdr_mp_init();
int fdr_mp_fini();

void fdr_mp_report_logger(int64_t octime, int64_t seqnum, float score, char *userid, char *username);

struct event_base *fdr_mp_getbase(fdr_mp_t *mp);

int fdr_mp_create_cloud(fdr_mp_t *mp);
int fdr_mp_destroy_cloud(fdr_mp_t *mp);
fdr_mp_cloud_t *fdr_mp_getcloud(fdr_mp_t *mp);
void fdr_mp_setcloud(fdr_mp_t *mp, fdr_mp_cloud_t *cloud);
void fdr_mp_cloud_report(fdr_mp_t *mp, fdr_mp_rlog_t *rl);

int fdr_mp_create_local(fdr_mp_t *mp);
int fdr_mp_destroy_local(fdr_mp_t *mp);
fdr_mp_local_t *fdr_mp_getlocal(fdr_mp_t *mp);
void fdr_mp_setlocal(fdr_mp_t *mp, fdr_mp_local_t *local);
void fdr_mp_local_report(fdr_mp_t *mp, fdr_mp_rlog_t *rl);

int fdr_mp_update_feature(const char *userid, void *buf, size_t len);

#endif // __FDR_MP_H__
