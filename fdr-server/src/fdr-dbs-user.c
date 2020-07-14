/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */


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

#define USER_LIMIT_MAX          10

// user table operations
/*
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
*/
static const char *user_tab_lookup = "SELECT * FROM fdr_user WHERE userid = ?1;";
static const char *user_tab_insert = "INSERT INTO fdr_user VALUES(?1, ?2, ?3, ?4,?5, ?6, ?7, datetime(CURRENT_TIMESTAMP, 'localtime'), datetime(CURRENT_TIMESTAMP, 'localtime'));";
static const char *user_tab_delete = "DELETE FROM fdr_user WHERE userid = ?1;";
static const char *user_tab_update = "UPDATE fdr_user SET name = ?1, desc = ?2, others = ?3, status = ?4, perm = ?5, expire_datetime = ?6,  \
                                        modify_datetime = datetime(CURRENT_TIMESTAMP, 'localtime') where userid = ?7;";

static const char *user_tab_list  = "SELECT userid FROM fdr_user LIMIT ?1 OFFSET ?2;";
static const char *user_tab_clean = "SELECT userid FROM fdr_user WHERE expire_datetime < ?1;";


static const char *user_tab_getstatus = "SELECT status FROM fdr_user WHERE userid = ?1;";
static const char *user_tab_setstatus = "UPDATE fdr_user SET status = ?1 where userid = ?2;";

static const char *user_tab_foreach = "SELECT * FROM fdr_user;";

static inline int check_dbs(){
    return fdr_dbs_get()->status;
}

#define CHECK_DBS_STATUS()      do{             \
    if(check_dbs() != 1){                       \
        log_warn("CHECK_DBS_STATUS => dbs not inited!\n");    \
        return -1;                              \
    }                                           \
}while(0)


static inline int user_insert(fdr_dbs_user_t *user){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    int rc = -1;
    
    rc = sqlite3_prepare_v2(dbs->dbs, user_tab_insert, strlen(user_tab_insert), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("user_insert => insert prepare fail:%d\n", rc);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user->userid,    strlen(user->userid),   SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user->name,      strlen(user->name),     SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user->desc,      strlen(user->desc),     SQLITE_STATIC);

    if(user->others == NULL){
        sqlite3_bind_null(stmt, 4);
    }else{
        sqlite3_bind_text(stmt, 4, user->others,     strlen(user->others),    SQLITE_STATIC);
    }

    sqlite3_bind_int(stmt, 5, user->status);
    sqlite3_bind_int(stmt, 6, user->perm);
    sqlite3_bind_int(stmt, 7, user->exptime);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if(rc != SQLITE_DONE){
        log_warn("user_insert => insert new user fail:%d\n", rc);
        return -1;
    }
    log_debug("add user %s ok\r\n", user->userid);
    return 0;
}

int fdr_dbs_user_insert(fdr_dbs_user_t *user){
    fdr_dbs_t *dbs = fdr_dbs_get();
    int rc = -1;

    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&dbs->mutex);

    rc = user_insert(user);

    pthread_mutex_unlock(&dbs->mutex);

    return rc;
}

static inline int user_delete(const char *userid){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    int rc = -1;

    rc = sqlite3_prepare_v2(dbs->dbs, user_tab_delete, strlen(user_tab_delete), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("user_delete => delete prepare fail:%d\n", rc);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, userid, strlen(userid), SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if(rc != SQLITE_DONE){
        log_warn("user_delete =>  delete user fail:%d\n", rc);
        return -1;
    }

    return 0;
}

int fdr_dbs_user_delete(const char *userid){
    fdr_dbs_t *dbs = fdr_dbs_get();
    int rc = -1;


    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&dbs->mutex);

    rc = user_delete(userid);

    pthread_mutex_unlock(&dbs->mutex);
    
    return rc;
}

static inline int user_update(const char *userid, fdr_dbs_user_t *user){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    int rc = -1;
    
    rc = sqlite3_prepare_v2(dbs->dbs, user_tab_update, strlen(user_tab_update), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("user_update => update user info prepare fail:%d\n", rc);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user->name,      strlen(user->name), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user->desc,      strlen(user->desc), SQLITE_STATIC);

    if(user->others == NULL){
        sqlite3_bind_null(stmt, 3);
    }else{
        sqlite3_bind_text(stmt, 3, user->others,    strlen(user->others), SQLITE_STATIC);
    }

    sqlite3_bind_int (stmt, 4, user->status);
    sqlite3_bind_int (stmt, 5, user->perm);
    sqlite3_bind_int (stmt, 6, user->exptime);
    sqlite3_bind_text(stmt, 7, userid, strlen(userid), SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if(rc != SQLITE_DONE){
        log_warn("user_update =>  update info fail:%d\n", rc);
        return -1;
    }
    
    return 0;
}


int fdr_dbs_user_update(const char *userid, fdr_dbs_user_t *user){
    fdr_dbs_t *dbs = fdr_dbs_get();
    fdr_dbs_user_t *fu;
    int rc = -1;

    CHECK_DBS_STATUS();

    rc = fdr_dbs_user_lookup(userid, &fu);
    if(rc != 0){
        log_warn("fdr_dbs_user_update => lookup user %s fail:%d\n", userid, rc);
        return -1;
    }
    
    if(user->name == NULL){
        user->name = fu->name;
    }

    if(user->desc == NULL){
        user->desc = fu->desc;
    }

    if(user->others == NULL){
        user->others = fu->others;
    }

    if(user->status < 0)
        user->perm = fu->status;

    if(user->perm < 0)
        user->perm = fu->perm;

    if(user->exptime < 0)
        user->exptime = fu->exptime;

    pthread_mutex_lock(&dbs->mutex);

    rc = user_update(userid, user);

    pthread_mutex_unlock(&dbs->mutex);

    fdr_dbs_user_free(fu);

    return rc;
}

static char *user_get_item(sqlite3_stmt *stmt, int idx){
    char *item;
    char *data;
    int len;

    item = (char *)sqlite3_column_text(stmt, idx);
    len = sqlite3_column_bytes(stmt, idx);
    if( (len == 0) || (item == NULL)){
        return NULL;
    }

    data = fdr_malloc(len+1);
    if(data == NULL){
        return NULL;
    }
    
    strncpy(data, item, len);
    data[len] = '\0';

    return data;
}


static inline int user_lookup(const char *userid, fdr_dbs_user_t **user){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    fdr_dbs_user_t *fu;
    int rc = -1;

    rc = sqlite3_prepare_v2(dbs->dbs, user_tab_lookup, strlen(user_tab_lookup), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("user_lookup => prepare fail:%d\n", rc);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, userid, strlen(userid), SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_ROW){
        log_warn("user_lookup => lookup user %s return %d\n", userid, rc);
        rc = -1;
        goto fail_exit;
    }

    fu = fdr_malloc(sizeof(fdr_dbs_user_t));
    if(fu == NULL){
        log_warn("user_lookup => no memory\n");
        rc = -1;
        goto fail_exit;
    }
    memset(fu, 0, sizeof(fdr_dbs_user_t));

    fu->userid = user_get_item(stmt, 0);
    if(fu->userid == NULL){
        log_warn("user_lookup => fu->userid no memory\n");
        goto fail_user;
    }
    
    fu->name = user_get_item(stmt, 1);
    if(fu->name == NULL){
        log_warn("user_lookup => fu->name no memory\n");
        goto fail_user;
    }

    fu->desc = user_get_item(stmt, 2);
    if(fu->desc == NULL){
        log_warn("user_lookup => fu->desc no memory\n");
        goto fail_user;
    }

    fu->others = user_get_item(stmt, 3);

    fu->status = sqlite3_column_int(stmt, 4);
    fu->perm = sqlite3_column_int(stmt, 5);

    fu->exptime = sqlite3_column_int(stmt, 6);

    fu->create = user_get_item(stmt, 7);
    if(fu->create == NULL){
        log_warn("user_lookup => fu->create no memory\n");
        goto fail_user;
    }
    
    fu->modify = user_get_item(stmt, 8);
    if(fu->modify == NULL){
        log_warn("user_lookup => fu->modify no memory\n");
        goto fail_user;
    }

    *user = fu;

    sqlite3_finalize(stmt);

    return 0;

fail_user:
    fdr_dbs_user_free(fu);

fail_exit:
    sqlite3_finalize(stmt);
    return -1;
}


int fdr_dbs_user_lookup(const char *userid, fdr_dbs_user_t **user){
    fdr_dbs_t *dbs = fdr_dbs_get();
    int rc = -1;

    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&dbs->mutex);

    rc = user_lookup(userid, user);

    pthread_mutex_unlock(&dbs->mutex);

    return rc;
}


static inline int user_list(int offset, int limit, char ***users){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    char **rsltuser;
    int i = 0;
    int rc = -1;

    rsltuser = (char **)fdr_malloc(sizeof(char*)*limit);
    if(rsltuser == NULL){
        log_warn("user_list  => no memory %d\n", sizeof(char*)*limit);
        return -1;
    }
    memset(rsltuser, 0, sizeof(char*)*limit);

    rc = sqlite3_prepare_v2(dbs->dbs, user_tab_list, strlen(user_tab_list), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("user_list => prepare fail:%d\n", rc);
        fdr_free(rsltuser);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);

    for(i = 0; i < limit; i++){
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_ROW){
            // log_info("user_list => no more users\n");
            break;
        }
        
        rsltuser[i] = user_get_item(stmt, 0);
        if(rsltuser[i] == NULL){
            break;
        }
    }

    sqlite3_finalize(stmt);

    *users = rsltuser;
    
    return i;
}

int fdr_dbs_user_list(int offset, int limit, char ***users){
    fdr_dbs_t *dbs = fdr_dbs_get();
    int rc = -1;
    
    CHECK_DBS_STATUS();

    if((limit <= 0) || (limit > USER_LIMIT_MAX))
        limit = USER_LIMIT_MAX;
    
    pthread_mutex_lock(&dbs->mutex);

    rc = user_list(offset, limit, users);

    pthread_mutex_unlock(&dbs->mutex);

    
    return rc;
}

static inline int user_free(fdr_dbs_user_t *user){
    if(user == NULL){
        return 0;
    }

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

    if(user->create != NULL){
        fdr_free(user->create);
        user->create = NULL;
    }

    if(user->modify != NULL){
        fdr_free(user->modify);
        user->modify = NULL;
    }

    return 0;
}

int fdr_dbs_user_free(fdr_dbs_user_t *user){
    user_free(user);

    fdr_free(user);

    return 0;
}

int fdr_dbs_user_exist(const char *userid){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    int rc = -1;


    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&dbs->mutex);

    rc = sqlite3_prepare_v2(dbs->dbs, user_tab_lookup, strlen(user_tab_lookup), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("fdr_user_exist => prepare fail:%d\n", rc);
        pthread_mutex_unlock(&dbs->mutex);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, userid, strlen(userid), SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&dbs->mutex);

    if(rc != SQLITE_ROW){
        return 0;
    }else{
        return 1;
    }
}


int fdr_dbs_user_setstatus(const char *userid, int bitmap){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    int status;
    int rc = -1;

    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&dbs->mutex);

    rc = sqlite3_prepare_v2(dbs->dbs, user_tab_getstatus, strlen(user_tab_getstatus), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("fdr_user_set_status => prepare fail:%d\n", rc);
        pthread_mutex_unlock(&dbs->mutex);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, userid, strlen(userid), SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if(rc == SQLITE_ROW){
        status = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);

        status = status | bitmap;
    
        rc = sqlite3_prepare_v2(dbs->dbs, user_tab_setstatus, strlen(user_tab_setstatus), &stmt, NULL);
        if(rc != SQLITE_OK){
            log_warn("fdr_user_set_status => prepare fail:%d\n", rc);
            pthread_mutex_unlock(&dbs->mutex);
            return -1;
        }
        sqlite3_bind_int(stmt, 1, status);
        sqlite3_bind_text(stmt, 2, userid, strlen(userid), SQLITE_STATIC);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    
        if(rc != SQLITE_DONE){
            log_warn("fdr_user_set_status => user update info fail:%d\n", rc);
            rc = -1;
        }else{
            rc = 0;
        }
    }else{
        sqlite3_finalize(stmt);
        log_warn("fdr_user_set_status => user update info fail:%d\n", rc);
        rc = -1;
    }

    pthread_mutex_unlock(&dbs->mutex);

    return rc;
}

int fdr_dbs_user_foreach(fdr_dbs_user_proc iterator, void *handle){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    fdr_dbs_user_t  user;
    int count = 0;
    int rc = -1;


    CHECK_DBS_STATUS();
    
    pthread_mutex_lock(&dbs->mutex);

    rc = sqlite3_prepare_v2(dbs->dbs, user_tab_foreach, strlen(user_tab_foreach), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("fdr_dbs_user_foreach => prepare fail:%d\n", rc);
        pthread_mutex_unlock(&dbs->mutex);
        return -1;
    }
    
    do{
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_ROW){
            log_info("fdr_dbs_user_foreach => retcode:%d, count:%d\n", rc, count);
            break;
        }

        user.userid = user_get_item(stmt, 0);
        if(user.userid == NULL){
            log_warn("fdr_dbs_user_foreach => no userid memory\n");
            continue;
        }
    
        user.name = user_get_item(stmt, 1);
        if(user.name == NULL){
            log_warn("fdr_dbs_user_foreach => no user name memory\n");
            continue;
        }

        user.desc = user_get_item(stmt, 2);
        if(user.desc == NULL){
            log_warn("fdr_dbs_user_foreach => no user desc memory\n");
            continue;
        }

        user.others = user_get_item(stmt, 3);

        user.status = sqlite3_column_int(stmt, 4);
        user.perm = sqlite3_column_int(stmt, 5);

        user.exptime = sqlite3_column_int(stmt, 6);

        user.create = NULL;
        user.modify = NULL;

        rc = fdr_dbs_feature_lookup(user.userid, &user.feature, &user.feature_len);
        if(rc != 0){
            user.feature = NULL;
            user.feature_len = 0;
            user_free(&user);
            continue;
        }

        rc = iterator(&user, handle);

        user_free(&user);

        if(rc != 0){
            log_warn("fdr_dbs_user_foreach => caller break %d\n", rc);
            break;
        }

        count++;
    }while(1);

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&dbs->mutex);

    return count;
}

static inline int user_clean(char *userid){
    fdr_dbs_t *dbs = fdr_dbs_get();
    sqlite3_stmt *stmt;
    char *uid;
    int  ulen;
    time_t cur = time(NULL);
    int rc = -1;

    rc = sqlite3_prepare_v2(dbs->dbs, user_tab_clean, strlen(user_tab_clean), &stmt, NULL);
    if(rc != SQLITE_OK){
        log_warn("user_clean => prepare fail:%d\n", rc);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, cur);
    rc = sqlite3_step(stmt);
    if(rc == SQLITE_ROW){
        uid = (char *)sqlite3_column_text(stmt, 0);
        ulen = sqlite3_column_bytes(stmt, 0);
        strncpy(userid, uid, ulen);
        userid[ulen] = '\0';
        rc = 0;
    }else{
        rc = -1;
    }

    sqlite3_finalize(stmt);
    
    return rc;
}

void fdr_dbs_user_clean(){
    fdr_dbs_t *dbs = fdr_dbs_get();
    char userid[FDR_DBS_USERID_SIZE*2];
    char userfile[FDR_PATH_MAX];
    int rc = -1;

    if(check_dbs() != 1){
        log_warn("CHECK_DBS_STATUS => dbs not inited!\n");
        return ;
    } 
    
    do{
        pthread_mutex_lock(&dbs->mutex);
        rc = user_clean(userid);
        pthread_mutex_unlock(&dbs->mutex);
    
        if(rc == 0){
            fdr_dbs_user_delete(userid);

            sprintf(userfile, "%s/%s.nv12", USER_IMAGE_PATH, userid);
            remove(userfile);

            sprintf(userfile, "%s/%s.bmp", USER_IMAGE_PATH, userid);
            remove(userfile);

            continue;
        }else{
            break;
        }
    }while(1);
}
