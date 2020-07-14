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

/*
static const char *guest_tab_create = "CREATE TABLE IF NOT EXISTS fdr_guest(    \
        scodeid INTEGER PRIMARY KEY,                                            \
        userid  VARCHAR(32),                                                    \
        guest   VARCHAR(16),                                                    \
        expire_datetime INTEGER,                                                \
        create_datetime VARCHAR(32) DEFAULT CURRENT_TIMESTAMP NOT NULL);";
*/
static const char *guest_tab_insert = "INSERT INTO fdr_guest(scodeid, userid, guest, expire_datetime) VALUES(?1, ?2, ?3, ?4);";
static const char *guest_tab_delete = "DELETE FROM fdr_guest WHERE scodeid = ?1;";
static const char *guest_tab_lookup = "SELECT * FROM fdr_guest WHERE  guest = ?1;";
static const char *guest_tab_clean  = "DELETE FROM fdr_guest WHERE expire_datetime < ?1;";

static inline int check_dbs(){
    return fdr_dbs_get()->status;
}

#define CHECK_DBS_STATUS()      do{             \
    if(check_dbs() != 1){                       \
        log_warn("CHECK_DBS_STATUS => dbs not inited!\n");    \
        return -1;                              \
    }                                           \
}while(0)

static inline int guest_insert(int scodeid, const char *userid, const char *gcode, time_t expired){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    int rc = -1;

    rc = sqlite3_prepare_v2(dbs->dbs, guest_tab_insert, strlen(guest_tab_insert), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("guest_insert => prepare fail:%d\n", rc);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, scodeid);
    sqlite3_bind_text(stmt, 2, userid, strlen(userid), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, gcode,  strlen(gcode),  SQLITE_STATIC);
    sqlite3_bind_int(stmt,  4, expired);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if(rc != SQLITE_DONE){
        log_warn("guest_insert => insert fail:%d\n", rc);
        return -1;
    }

    return 0;
}


int fdr_dbs_guest_insert(int scodeid, const char *userid, const char *gcode, time_t expired){
    fdr_dbs_t *dbs = fdr_dbs_get();
    int rc = -1;

    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&dbs->mutex);

    rc = guest_insert(scodeid, userid, gcode, expired);

    pthread_mutex_unlock(&dbs->mutex);

    return rc;
}

static inline int guest_delete(int scodeid){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    int rc = -1;

    rc = sqlite3_prepare_v2(dbs->dbs, guest_tab_delete, strlen(guest_tab_delete), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("guest_delete => prepare fail:%d\n", rc);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, scodeid);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if(rc != SQLITE_DONE){
        log_warn("guest_delete => delete user fail:%d\n", rc);
        return -1;
    }

    return 0;
}

int fdr_dbs_guest_delete(int scodeid){
    fdr_dbs_t *dbs = fdr_dbs_get();
    int rc = -1;

    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&dbs->mutex);

    rc = guest_delete(scodeid);

    pthread_mutex_unlock(&dbs->mutex);

    return rc;
}

static inline int guest_exist(const char *gcode, time_t *expired){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    int rc = -1;

    rc = sqlite3_prepare_v2(dbs->dbs, guest_tab_lookup, strlen(guest_tab_lookup), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("guest_insert => prepare fail:%d\n", rc);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, gcode,  strlen(gcode),  SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if(rc != SQLITE_ROW){
        log_warn("guest_insert => insert fail:%d\n", rc);
        sqlite3_finalize(stmt);
        return -1;
    }

    *expired = sqlite3_column_int(stmt, 3);

    sqlite3_finalize(stmt);

    return 0;
}

int fdr_dbs_guest_exist(const char *gcode, time_t *expired){
    fdr_dbs_t *dbs = fdr_dbs_get();
    int rc = -1;

    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&dbs->mutex);

    rc = guest_exist(gcode, expired);

    pthread_mutex_unlock(&dbs->mutex);

    return rc;
}

static inline int guest_clean(){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    time_t cur = time(NULL);
    int rc = -1;

    rc = sqlite3_prepare_v2(dbs->dbs, guest_tab_clean, strlen(guest_tab_clean), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("guest_clean => prepare fail:%d\n", rc);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, cur);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if(rc != SQLITE_DONE){
        log_warn("guest_clean => clean fail:%d\n", rc);
        return -1;
    }

    return 0;
}

void fdr_dbs_guest_clean(){
    fdr_dbs_t *dbs = fdr_dbs_get();

    if(check_dbs() != 1){ 
        log_warn("fdr_dbs_guest_clean => dbs not inited!\n");
        return ;
    }
    
    pthread_mutex_lock(&dbs->mutex);
    guest_clean();
    pthread_mutex_unlock(&dbs->mutex);
}


