/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */


#ifndef __FDR_DBS_H__
#define __FDR_DBS_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "sqlite3.h"

#include <pthread.h>
#include <inttypes.h>
#include <time.h>

#define FDR_DBS_USERID_SIZE                48
#define FDR_DBS_USERNAME_SIZE              24
#define FDR_DBS_USERDESC_SIZE              48
#define FDR_DBS_USEROTHERS_SIZE            48

// declarations
typedef struct fdr_dbs{
    sqlite3         *dbs;       // sqlite3 db instance

    pthread_mutex_t  mutex;     // lock
    int     status;             // sync in startup
}fdr_dbs_t;

// 
int fdr_dbs_init(const char *dbfile);
int fdr_dbs_fini();
int fdr_dbs_exec_result(void *db, const char *sqlstr, int64_t *rslt);
fdr_dbs_t *fdr_dbs_get();


// configuration interface
int fdr_dbs_config_set(const char *key, const char *val);
int fdr_dbs_config_get(const char *key, char **val);

//  guest code interface
int fdr_dbs_guest_insert(int scodeid, const char *userid, const char *gcode, time_t expired);
int fdr_dbs_guest_delete(int scodeid);
int fdr_dbs_guest_exist(const char *gcode, time_t *expired);
void fdr_dbs_guest_clean();

//  user interface
#define FDR_DBS_USER_STATUS_FEATURE     0x00000001
#define FDR_DBS_USER_STATUS_UIIMAGE     0x00000002
#define FDR_DBS_USER_STATUS_NORMAL      (FDR_DBS_USER_STATUS_FEATURE|FDR_DBS_USER_STATUS_UIIMAGE)

typedef struct fdr_dbs_user{
    char *userid;       // 用户ID
    char *name;         // 用户名
    char *desc;         // 用户描述信息（公司部门）
    char *others;       // 自定义数据
    int  status;        // 0, 未申效用户， 0x01, 有特征值用户， 0x02， 有UI用户
    int  perm;          // 权限, 0-无权, 1-有权
    int  feature_len;   // 脸部特征值大小
    void *feature;      // 脸部特征
    char *create;       // 创建日期
    time_t exptime;     // 过期时间，自1970.1.1 00:00:00开始的秒数
    char *modify;       // 创建/修改用户时间
}fdr_dbs_user_t;

int fdr_dbs_user_free(fdr_dbs_user_t *user);

int fdr_dbs_user_insert(fdr_dbs_user_t *user);
int fdr_dbs_user_delete(const char *userid);
int fdr_dbs_user_update(const char *userid, fdr_dbs_user_t *user);
int fdr_dbs_user_lookup(const char *userid, fdr_dbs_user_t **user);
int fdr_dbs_user_list(int offset, int limit, char ***users);
int fdr_dbs_user_exist(const char *userid);
int fdr_dbs_user_setstatus(const char *userid, int bitmap);
void fdr_dbs_user_clean();

typedef int (*fdr_dbs_user_proc)(fdr_dbs_user_t *user, void *handle);
int fdr_dbs_user_foreach(fdr_dbs_user_proc proc, void *handle);

// feature interface
int fdr_dbs_feature_init(const char *dbfile);
int fdr_dbs_feature_fini();

int fdr_dbs_feature_insert(const char *userid, void *feature, int flen);
int fdr_dbs_feature_update(const char *userid, void *feature, int flen);
int fdr_dbs_feature_lookup(const char *userid, void **feature, int *flen);
int fdr_dbs_feature_delete(const char *userid);

#ifdef __cplusplus
}
#endif

#endif // __FDR_DBS_H__
