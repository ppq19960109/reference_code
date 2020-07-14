/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */


#include "fdr-dbs.h"
#include "fdr.h"

#include "logger.h"
#include "sqlite3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>


// configuration table operations
/*
static const char *conf_tab_create = "CREATE TABLE IF NOT EXISTS fdr_config(key TEXT PRIMARY KEY,val TEXT);";
*/

static const char *conf_tab_lookup = "SELECT * FROM fdr_config WHERE key = ?1;";
static const char *conf_tab_insert = "INSERT INTO   fdr_config VALUES(?1, ?2);";
static const char *conf_tab_update = "REPLACE INTO  fdr_config VALUES(?1, ?2);";

static inline int check_dbs(){
    return fdr_dbs_get()->status;
}

#define CHECK_DBS_STATUS()      do{             \
    if(check_dbs() != 1){                       \
        log_warn("dbs not inited right!\n");    \
        return -1;                              \
    }                                           \
}while(0)


static inline int config_insert(const char *key, const char *val){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    int   rc;

    rc = sqlite3_prepare_v2(dbs->dbs, conf_tab_insert, strlen(conf_tab_insert), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("conf_insert => prepare conf:%s fail!\n", key);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, key, strlen(key), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, val, strlen(val), SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if(rc != SQLITE_DONE){
        log_warn("conf_insert => insert conf:%s fail!\n", key);
        return -1;
    }

    return 0;
}


static inline int config_lookup(const char *key, char **val){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    char *temp, *lval;
    int len;
    int rc = -1;

    rc = sqlite3_prepare_v2(dbs->dbs, conf_tab_lookup, strlen(conf_tab_lookup), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("config_lookup => prepare conf:%s fail!\n", key);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, key, strlen(key), SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_ROW){
        // log_warn("config_lookup => step conf:%s fail!\n", key);
        sqlite3_finalize(stmt);
        return -1;
    }

    temp = (char *)sqlite3_column_text(stmt, 1);
    len = sqlite3_column_bytes(stmt, 1);
    lval = fdr_malloc(len+1);
    if(lval == NULL){
        log_warn("config_lookup => no more memory!\n");
        sqlite3_finalize(stmt);
        return -1;
    }
    
    strncpy(lval, temp, len);
    lval[len] = '\0';

    sqlite3_finalize(stmt);

    *val = lval;
    
    return 0;
}

static inline int config_update(const char *key, const char *val){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    int   rc;

    rc = sqlite3_prepare_v2(dbs->dbs, conf_tab_update, strlen(conf_tab_update), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("conf_update => prepare conf:%s fail!\n", key);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, key, strlen(key), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, val, strlen(val), SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if(rc != SQLITE_DONE){
        log_warn("conf_update => update conf:%s fail!\n", key);
        return -1;
    }

    return 0;
}

int fdr_dbs_config_set(const char *key, const char *val){
    fdr_dbs_t *dbs = fdr_dbs_get();
    char *temp;
    int rc;

    CHECK_DBS_STATUS();

    pthread_mutex_lock(&dbs->mutex);

    rc = config_lookup(key, &temp);

    if(rc != 0){
        rc = config_insert(key, val);
    }else{
        fdr_free(temp);
        rc = config_update(key, val);
    }
    pthread_mutex_unlock(&dbs->mutex);

    return rc;
}

int fdr_dbs_config_get(const char *key, char **val){
    fdr_dbs_t *dbs = fdr_dbs_get();
    int rc = -1;

    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&dbs->mutex);

    rc = config_lookup(key, val);
    
    pthread_mutex_unlock(&dbs->mutex);

    return rc;
}
