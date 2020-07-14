/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr-dbs.h"
#include "fdr.h"

#include "bisp_aie.h"

#include "logger.h"
#include "sqlite3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

// declarations
typedef struct fdr_feature_db{
    sqlite3         *dbs;   // db instance

    pthread_mutex_t  mutex;
    int     status;         // 0 - uninited, 1 - inited
}fdr_feature_db_t;

// user table operations
static const char *feature_create = "CREATE TABLE IF NOT EXISTS fdr_feature( \
        userid  VARCHAR(32) PRIMARY KEY,                        \
        feature BLOB);";

static const char *feature_lookup = "SELECT * FROM fdr_feature WHERE userid = ?1;";
static const char *feature_insert = "INSERT INTO fdr_feature VALUES(?1, ?2);";
static const char *feature_delete = "DELETE FROM fdr_feature WHERE userid = ?1;";
static const char *feature_update = "UPDATE fdr_feature SET feature = ?1 where userid = ?2;";

// global variables
static fdr_feature_db_t  g_fdr_dbs_feature = {};
static inline fdr_feature_db_t* getdbs(){
    return &g_fdr_dbs_feature;
}

static inline int check_dbs(){
    return getdbs()->status;
}

#define CHECK_DBS_STATUS()      do{             \
    if(check_dbs() != 1){                       \
        log_warn("dbs not inited right!\n");    \
        return -1;                              \
    }                                           \
}while(0)


int fdr_dbs_feature_init(const char *dbfile){
    fdr_feature_db_t *dbs = getdbs();
    sqlite3_stmt *stmt;
    int rc;

    pthread_mutex_init(&dbs->mutex, NULL);
    dbs->status = 0;

    // open sqlite db
    rc = sqlite3_open_v2(dbfile, &dbs->dbs, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, NULL);
    if(rc != 0){
        pthread_mutex_destroy(&dbs->mutex);
        return -ENOMEM;
    }

    //init config table
    rc = sqlite3_prepare_v2(dbs->dbs, feature_create, strlen(feature_create), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("fdr_dbs_feature_init => prepare create stmt fail:%d\n", rc);
        sqlite3_close(dbs->dbs);
        pthread_mutex_destroy(&dbs->mutex);
        return -EINVAL;
    }

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        log_warn("fdr_dbs_feature_init => execute create config table fail:%d\n", rc);
        sqlite3_finalize(stmt);
        sqlite3_close(dbs->dbs);
        pthread_mutex_destroy(&dbs->mutex);
        return -EINVAL;
    }

    sqlite3_finalize(stmt);

    dbs->status = 1;

    return 0;
}

int fdr_dbs_feature_fini(){
    fdr_feature_db_t *dbs = getdbs();

    if(!dbs->status)
        return 0;

    sqlite3_close(dbs->dbs);
    pthread_mutex_destroy(&dbs->mutex);
    dbs->status = 0;

    return 0;
}


static inline int dbs_feature_insert(const char *userid, void *feature, int flen){
    fdr_feature_db_t *dbs = getdbs();
    sqlite3_stmt *stmt;
    int   rc;

    rc = sqlite3_prepare_v2(dbs->dbs, feature_insert, strlen(feature_insert), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("dbs_feature_insert => prepare stmt fail:%d\n", rc);
        return -1;
    }

    //log_info("dbs_feature_insert => insert %s feature len %d\n", userid, flen);
    sqlite3_bind_text(stmt, 1, userid, strlen(userid), SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, feature, flen, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if(rc != SQLITE_DONE){
        log_warn("dbs_feature_insert => insert feature:%s fail %d len %d\n", userid, rc, flen);
        log_warn("dbs_feature_insert => %s\n", sqlite3_errmsg(dbs->dbs));
        return -1;
    }

    return 0;
}

int fdr_dbs_feature_insert(const char *userid, void *feature, int flen){
    fdr_feature_db_t *dbs = getdbs();
    int rc;

    CHECK_DBS_STATUS();

    pthread_mutex_lock(&dbs->mutex);

    rc = dbs_feature_insert(userid, feature, flen);

    pthread_mutex_unlock(&dbs->mutex);

    return rc;
}

static inline int dbs_feature_update(const char *userid, void *feature, int flen){
    fdr_feature_db_t *dbs = getdbs();
    sqlite3_stmt *stmt;
    int   rc;

    rc = sqlite3_prepare_v2(dbs->dbs, feature_update, strlen(feature_update), &stmt, NULL);
    if(rc != SQLITE_OK){
        return -1;
    }

    sqlite3_bind_blob(stmt, 1, feature, flen, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, userid, strlen(userid), SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if(rc != SQLITE_DONE){
        log_warn("dbs_feature_update => update feature:%s fail!\n", userid);
        return -1;
    }

    return 0;
}

int fdr_dbs_feature_update(const char *userid, void *feature, int flen){
    fdr_feature_db_t *dbs = getdbs();
    int rc;

    CHECK_DBS_STATUS();

    pthread_mutex_lock(&dbs->mutex);

    rc = dbs_feature_update(userid, feature, flen);
    
    pthread_mutex_unlock(&dbs->mutex);

    return rc;
}

static inline int dbs_feature_lookup(const char *userid, void **feature, int *flen){
    fdr_feature_db_t *dbs = getdbs();
    sqlite3_stmt *stmt;
    void *temp, *ft;
    int len;
    int rc;

    rc = sqlite3_prepare_v2(dbs->dbs, feature_lookup, strlen(feature_lookup), &stmt, NULL);
    if(rc != SQLITE_OK){
        return -1;
    }

    sqlite3_bind_text(stmt, 1, userid, strlen(userid), SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_ROW){
        log_warn("dbs_feature_lookup => lookup feature:%s fail!\n", userid);
        sqlite3_finalize(stmt);
        return -1;
    }

    if(feature != NULL){
        len = sqlite3_column_bytes(stmt, 1);
        temp = (void *)sqlite3_column_blob(stmt, 1);
        if(len != (BISP_AIE_FDRVECTOR_MAX * sizeof(float))){
            log_warn("dbs_feature_lookup => feature len %d wrong\n", len);
            sqlite3_finalize(stmt);
            return -1;
        }

        ft = (void *)fdr_malloc(len);
        if(ft == NULL){
            log_warn("dbs_feature_lookup => no memory\n");
            sqlite3_finalize(stmt);
            return -1;
        }

        memcpy(ft, temp, len);

        *feature  = ft;
        *flen  = len;
    }

    rc = 0;

    sqlite3_finalize(stmt);

    return 0;
}

int fdr_dbs_feature_lookup(const char *userid, void **feature, int *flen){
    fdr_feature_db_t *dbs = getdbs();
    int rc;

    CHECK_DBS_STATUS();

    pthread_mutex_lock(&dbs->mutex);

    rc = dbs_feature_lookup(userid, feature, flen);
    
    pthread_mutex_unlock(&dbs->mutex);

    return rc;
}


static inline int dbs_feature_delete(const char *userid){
    fdr_feature_db_t *dbs = getdbs();
    sqlite3_stmt *stmt;
    int   rc;

    rc = sqlite3_prepare_v2(dbs->dbs, feature_delete, strlen(feature_delete), &stmt, NULL);
    if(rc != SQLITE_OK){
        return -1;
    }

    sqlite3_bind_text(stmt, 1, userid, strlen(userid), SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if(rc != SQLITE_DONE){
        log_warn("dbs_feature_delete => delete feature:%s fail!\n", userid);
        return -1;
    }

    return 0;
}

int fdr_dbs_feature_delete(const char *userid){
    fdr_feature_db_t *dbs = getdbs();
    int rc;

    CHECK_DBS_STATUS();

    pthread_mutex_lock(&dbs->mutex);

    rc = dbs_feature_delete(userid);
    
    pthread_mutex_unlock(&dbs->mutex);

    return rc;
}

