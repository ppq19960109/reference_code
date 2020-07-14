/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr-logger.h"
#include "fdr-dbs.h"
#include "fdr.h"

#include "fdr-path.h"

#include "logger.h"
#include "sqlite3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#define  FDR_LOGGER_PATH_MAX        256
#define  FDR_LOGGER_BATCH           128

#define CHECK_DBS_STATUS()      do{                 \
    if(get_logger()->status != 1){                        \
        log_warn("logger db not inited right!\n");  \
        return -1;                                  \
    }                                               \
}while(0)

// declarations
typedef struct fdr_logger{
    sqlite3     *dbs;

    pthread_mutex_t  mutex;
    int     status;
}fdr_logger_t;


// user table operations
static const char *logmatch_tab_create = "CREATE TABLE IF NOT EXISTS fdr_logmatch( \
        seqnum  INTEGER PRIMARY KEY AUTOINCREMENT,         \
        occtime INT8,                                  \
        face_x  INT2,                                  \
        face_y  INT2,                                  \
        face_w  INT2,                                  \
        face_h  INT2,                                  \
        sharp   FLOAT,                                 \
        score   FLOAT,                                 \
        userid  VARCHAR(16));";

static const char *logmatch_tab_lookup = "SELECT * FROM fdr_logmatch WHERE seqnum = ?1;";
static const char *logmatch_tab_insert = "INSERT INTO fdr_logmatch(occtime, face_x, face_y, face_w, face_h, sharp, score, userid) \
                                          VALUES(?1, ?2, ?3, ?4,?5, ?6, ?7, ?8);";

static const char *logmatch_tab_delete = "DELETE FROM fdr_logmatch WHERE seqnum = ?1;";

static const char  *logmatch_tab_count = "SELECT COUNT(*) FROM fdr_logmatch;";
static const char  *logmatch_tab_max   = "SELECT MAX(seqnum) FROM fdr_logmatch;";
static const char  *logmatch_tab_min   = "SELECT MIN(seqnum) FROM fdr_logmatch;";

static const char *logger_image_path       = CAPTURE_IMAGE_PATH;

// global variables
static fdr_logger_t  g_fdr_logger_instance;

static fdr_logger_t *get_logger(){
    return &g_fdr_logger_instance;
}

int fdr_logger_init(const char *dbfile){
    fdr_logger_t *logger = get_logger();
    sqlite3_stmt *stmt;
    int rc;

    pthread_mutex_init(&logger->mutex, NULL);
    logger->status = 0;

    // open sqlite db
    rc = sqlite3_open_v2(dbfile, &logger->dbs, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, NULL);
    if(rc != 0){
        log_warn("open logmatch db  fail:%d\n", rc);
        pthread_mutex_destroy(&logger->mutex);
        return -ENOMEM;
    }

    //init config table
    rc = sqlite3_prepare_v2(logger->dbs, logmatch_tab_create, strlen(logmatch_tab_create), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("prepare logmatch table create stmt fail:%d\n", rc);
        sqlite3_close(logger->dbs);
        pthread_mutex_destroy(&logger->mutex);
        return -EINVAL;
    }

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        log_warn("execute create logmatch table fail:%d\n", rc);
        sqlite3_finalize(stmt);
        sqlite3_close(logger->dbs);
        pthread_mutex_destroy(&logger->mutex);
        
        return -EINVAL;
    }

    sqlite3_finalize(stmt);

     logger->status = 1;

    return 0;
}


int fdr_logger_fini(){
    fdr_logger_t *logger = get_logger();

    if(logger->status > 0){
        logger->status = 0;
        sqlite3_close(logger->dbs);
        pthread_mutex_destroy(&logger->mutex);;
    }

    return 0;
}

int64_t fdr_logger_append(int64_t octime, bisp_aie_rect_t *rect, float sharp, float score, const char *userid){
    sqlite3_stmt *stmt;
    sqlite3_int64 rc;
    fdr_logger_t *logger = get_logger();
    char strangerid[FDR_DBS_USERID_SIZE+1];

    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&logger->mutex);

    rc = sqlite3_prepare_v2(logger->dbs, logmatch_tab_insert, strlen(logmatch_tab_insert), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("logmatch insert prepare fail:%d\n", rc);
        pthread_mutex_unlock(&logger->mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, octime);
    sqlite3_bind_int(stmt, 2, rect->x0);
    sqlite3_bind_int(stmt, 3, rect->y0);
    sqlite3_bind_int(stmt, 4, rect->x1 - rect->x0);
    sqlite3_bind_int(stmt, 5, rect->y1 - rect->y0);
    
    sqlite3_bind_double(stmt, 6, sharp);
    sqlite3_bind_double(stmt, 7, score);

    if(userid != NULL){
        sqlite3_bind_text(stmt, 8, userid, strlen(userid), SQLITE_STATIC);
    }else{
        snprintf(strangerid, FDR_DBS_USERID_SIZE+1,  "0000%012llX", octime);
        sqlite3_bind_text(stmt, 8, strangerid, strlen(strangerid), SQLITE_STATIC);
    }

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if(rc != SQLITE_DONE){
        log_warn("logmatch insert new user fail:%d\n", rc);
        rc = -1;
    }else{
        rc = sqlite3_last_insert_rowid(logger->dbs);
    }
    
    pthread_mutex_unlock(&logger->mutex);

    //log_info("append item ok : %llu - %f - %f\n", octime, sharp, score);

    return rc;
}

int fdr_logger_get(int64_t seqnum, fdr_logger_record_t *lr){
    sqlite3_stmt *stmt;
    fdr_logger_t *logger = get_logger();
    char *temp;
    int rc = -1;

    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&logger->mutex);

    rc = sqlite3_prepare_v2(logger->dbs, logmatch_tab_lookup, strlen(logmatch_tab_lookup), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("logmatch lookup prepare fail:%d\n", rc);
        pthread_mutex_unlock(&logger->mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, seqnum);

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_ROW){
        //log_warn("logmatch lookup record fail:%d\n", rc);
        rc = -1;
    }else{
        rc = 0;
    }
    
    lr->seqnum = sqlite3_column_int64(stmt, 0);
    lr->occtime = sqlite3_column_int64(stmt, 1);
    lr->face_x = sqlite3_column_int(stmt, 2);
    lr->face_y = sqlite3_column_int(stmt, 3);
    lr->face_w = sqlite3_column_int(stmt, 4);
    lr->face_h = sqlite3_column_int(stmt, 5);
    lr->sharp = sqlite3_column_double(stmt, 6);
    lr->score = sqlite3_column_double(stmt, 7);

    temp = (char *)sqlite3_column_text(stmt, 8);
    if(temp != NULL){
        strncpy(lr->userid, temp, FDR_DBS_USERID_SIZE+1);
    }else{
        // for stranger
        strncpy(lr->userid, "0000000000000000", FDR_DBS_USERID_SIZE+1);
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&logger->mutex);

    return rc;
}

static int fdr_logger_delete_item(int64_t seqnum){
    sqlite3_stmt *stmt;
    fdr_logger_t *logger = get_logger();
    int rc = -1;

    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&logger->mutex);

    rc = sqlite3_prepare_v2(logger->dbs, logmatch_tab_delete, strlen(logmatch_tab_delete), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("logmatch delete prepare fail:%d\n", rc);
        pthread_mutex_unlock(&logger->mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, seqnum);

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        log_warn("logmatch delete execute fail:%d\n", rc);
        rc = -1;
    }else{
        rc = 0;
    }
    
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&logger->mutex);

    return rc;

}

static int fdr_logger_count(int64_t *count){
    fdr_logger_t *logger = get_logger();
    int64_t rslt;
    int rc;

    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&logger->mutex);

    rc = fdr_dbs_exec_result(logger->dbs, logmatch_tab_count, &rslt);
    if(rc == 0){
        *count = rslt;
    }

    pthread_mutex_unlock(&logger->mutex);

    return rc;
}

int fdr_logger_max(int64_t *max){
    fdr_logger_t *logger = get_logger();
    int64_t rslt;
    int rc;

    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&logger->mutex);

    rc = fdr_dbs_exec_result(logger->dbs, logmatch_tab_count, &rslt);
    if(rc == 0){
        *max = rslt;
    }
    if(*max != 0){
        rc = fdr_dbs_exec_result(logger->dbs, logmatch_tab_max, &rslt);
         if(rc == 0){
            *max = rslt;
        }
    }

    pthread_mutex_unlock(&logger->mutex);

    return rc;
}

static int fdr_logger_min(int64_t *min){
    fdr_logger_t *logger = get_logger();
    int64_t rslt;
    int rc;

    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&logger->mutex);

    rc = fdr_dbs_exec_result(logger->dbs, logmatch_tab_count, &rslt);
    if(rc == 0){
        *min = rslt;
    }
    if(*min != 0){
        rc = fdr_dbs_exec_result(logger->dbs, logmatch_tab_min, &rslt);
        if(rc == 0){
            *min = rslt;
        }
    }
    pthread_mutex_unlock(&logger->mutex);

    return rc;
}

static void logger_mkdir(const char *path){
    if(access(path, F_OK) != 0){
        mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);
    }
}

int fdr_logger_saveimage(int64_t octime, void *frame, int width, int heigh){
    char image_path[FDR_LOGGER_PATH_MAX];
    time_t tm = (time_t)(octime / 1000);
    struct tm t;

    //setenv("TZ", "GMT-8", 1);
    if(localtime_r(&tm, &t) == NULL){
        log_warn("fdr_logger_saveimage => get gmtime fail\n");
        return -1;
    }

    sprintf(image_path, "%s/%04d-%02d-%02d", logger_image_path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    logger_mkdir(image_path);

    sprintf(image_path, "%s/%04d-%02d-%02d/%02d-%02d-%02d.%03lu.jpg",
                            logger_image_path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                            t.tm_hour, t.tm_min, t.tm_sec, (unsigned long)(octime % 1000));

    //log_info("fdr_logger_saveimage => save to %s, timestamp : %llu\n", image_path, octime);

    return bisp_aie_writetojpeg(image_path, frame);
}

static void fdr_logger_delete(int64_t seqnum){
    char image_path[FDR_LOGGER_PATH_MAX];
    fdr_logger_record_t lr;
    time_t tm;
    struct tm t;
    int rc;

    rc = fdr_logger_get(seqnum, &lr);
    if(rc != 0){
        log_warn("fdr_logger_delete => get min record fail:%d\n", rc);
        return ;
    }

    fdr_logger_delete_item(seqnum);

    tm = (time_t)(lr.occtime / 1000);

    //setenv("TZ", "GMT-8", 1);
    if(localtime_r(&tm, &t) == NULL){
        log_warn("fdr_logger_delete => get gmtime fail\n");
        return ;
    }

    sprintf(image_path, "%s/%04d-%02d-%02d/%02d-%02d-%02d.%03lu.jpg",
                            logger_image_path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                            t.tm_hour, t.tm_min, t.tm_sec, (unsigned long)(lr.occtime % 1000));
    remove(image_path);

    sprintf(image_path, "%s/%04d-%02d-%02d", logger_image_path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    remove(image_path);
}

void fdr_logger_clean(){
    fdr_config_t *cfg = fdr_config();
    int64_t count = 0, max = 0, min = 0, i;
    int rc;

    rc = fdr_logger_count(&count);
    if(rc != 0){
        log_warn("fdr_logger_clean => get logger count fail:%d\n", rc);
        return ;
    }

    // logger count less than logr_item_max, just return;
    // 
    if(count <= (cfg->logr_item_max + FDR_LOGGER_BATCH))
        return ;

    rc = fdr_logger_max(&max);
    if(rc != 0){
        log_warn("fdr_logger_clean => get logger max seqnum fail:%d\n", rc);
        return ;
    }

    rc = fdr_logger_min(&min);
    if(rc != 0){
        log_warn("fdr_logger_clean => get logger min seqnum fail:%d\n", rc);
        return ;
    }

    if((max - min) <= count){
        log_warn("fdr_logger_clean => count != max-min:%llu %llu %llu\n", count, max, min);
        return ;
    }

    max = max - count;
    
    for(i = min; i < max; i++){
        fdr_logger_delete(i);
    }
}

