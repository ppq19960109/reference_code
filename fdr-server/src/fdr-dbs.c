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

// configuration table creation
static const char *conf_tab_create = "CREATE TABLE IF NOT EXISTS fdr_config(key TEXT PRIMARY KEY,val TEXT);";

// guest table creation
static const char *guest_tab_create = "CREATE TABLE IF NOT EXISTS fdr_guest(    \
        scodeid INTEGER PRIMARY KEY,                                            \
        userid  VARCHAR(32),                                                    \
        guest   VARCHAR(16),                                                    \
        expire_datetime INTEGER,                                                \
        create_datetime VARCHAR(32) DEFAULT CURRENT_TIMESTAMP NOT NULL);";

// user table operations
static const char *user_tab_create = "CREATE TABLE IF NOT EXISTS fdr_user( \
        userid  VARCHAR(32) PRIMARY KEY,                        \
        name    VARCHAR(16),                                    \
        desc    VARCHAR(32),                                    \
        others  VARCHAR(32),                                    \
        status  INTEGER,                                        \
        perm  INTEGER,                                          \
        expire_datetime INTEGER,                                \
        create_datetime VARCHAR(32),                            \
        modify_datetime VARCHAR(32) DEFAULT CURRENT_TIMESTAMP NOT NULL);";

// global variables
static fdr_dbs_t  g_fdr_dbs = {};
fdr_dbs_t* fdr_dbs_get(){
    return &g_fdr_dbs;
}

int fdr_dbs_init(const char *dbfile){
    fdr_dbs_t  *dbs = fdr_dbs_get();
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
    rc = sqlite3_prepare_v2(dbs->dbs, conf_tab_create, strlen(conf_tab_create), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("prepare config table create stmt fail:%d\n", rc);
        sqlite3_close(dbs->dbs);
        pthread_mutex_destroy(&dbs->mutex);
        return -EINVAL;
    }

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        log_warn("execute create config table fail:%d\n", rc);
        sqlite3_finalize(stmt);
        sqlite3_close(dbs->dbs);
        pthread_mutex_destroy(&dbs->mutex);
        
        return -EINVAL;
    }
    sqlite3_finalize(stmt);

    // init user table
    rc = sqlite3_prepare_v2(dbs->dbs, user_tab_create, strlen(user_tab_create), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("prepare user table create stmt fail:%d\n", rc);
        sqlite3_close(dbs->dbs);
        pthread_mutex_destroy(&dbs->mutex);
        return -EINVAL;
    }

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        log_warn("execute create user table fail:%d\n", rc);
        sqlite3_finalize(stmt);
        sqlite3_close(dbs->dbs);
        pthread_mutex_destroy(&dbs->mutex);
        return -EINVAL;
    }
    sqlite3_finalize(stmt);

    // init guest code table
    rc = sqlite3_prepare_v2(dbs->dbs, guest_tab_create, strlen(guest_tab_create), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("prepare guest code table create stmt fail:%d\n", rc);
        sqlite3_close(dbs->dbs);
        pthread_mutex_destroy(&dbs->mutex);
        return -EINVAL;
    }

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        log_warn("execute create guest code table fail:%d\n", rc);
        sqlite3_finalize(stmt);
        sqlite3_close(dbs->dbs);
        pthread_mutex_destroy(&dbs->mutex);
        
        return -EINVAL;
    }
    sqlite3_finalize(stmt);

    dbs->status = 1;

    return 0;
}


int fdr_dbs_fini(){
    fdr_dbs_t  *dbs = fdr_dbs_get();

    sqlite3_close(dbs->dbs);
    pthread_mutex_destroy(&dbs->mutex);
    dbs->status = 0;
    
    return 0;
}

static int exec_callback(void *count, int argc, char **argv, char **azColName) {
    int64_t *val = (int64_t *)count;
	const char *nptr = argv[0];
	char *endptr = NULL;

    if(nptr == NULL){
        *val = 0;
        return -1;
    }

	*val = strtoll(nptr, &endptr, 10);

    return 0;
}

int fdr_dbs_exec_result(void *db, const char *sqlstr, int64_t *rslt){
    sqlite3 *dbs = (sqlite3 *)db;
    int64_t count = 0;
    int rc;

    rc = sqlite3_exec(dbs, sqlstr, exec_callback, &count, NULL);
    if (rc != SQLITE_OK) {
        log_warn("row count fail:%d\n", rc);
        return -1;
    }

    *rslt = count;

    return 0;
}
