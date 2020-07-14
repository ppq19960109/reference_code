/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */
#include "fdr-test.h"

#include "fdr-dbs.h"
#include "fdr.h"

#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h> 
#include <sys/time.h>


static const char *test_db_sqlite = "./test_db.sqlite";
static const char *test_feature_sqlite = "./test_feature.sqlite";

#define TEST_RESULT(retcode,message)            \
    do{                                         \
        if(retcode == 0){                       \
            log_info("PASS => %s\n", message);  \
        }else{                                  \
            log_warn("FAIL => %s\n", message);  \
            return retcode;                     \
        }                                       \
    }while(0)

static char *const_names[] = {
    "张三",
    "张三.李四",
    "李四",
    "李四.张三",
    "王二麻子",
    "王二麻子.诺夫",
    "王二麻子.博格"
};

static char *const_descs[] = {
    "销售部",
    "市场部",
    "研发部",
    "国际贸易部",
    "新技术研究与开发部",
    "供应链"
};

static char *const_others[] = {
    "13457639976",
    "13663548853",
    "13763548853",
    "18563548853",
    "13263548853",
    "17763548853"
};

static int test_user_add(char *userid, int n, int d, int o){
    fdr_dbs_user_t  user;
    int rc;
    
    user.userid = userid;
    user.name   = const_names[n % (sizeof(const_names)/sizeof(char*))];
    user.desc   = const_descs[d % (sizeof(const_descs)/sizeof(char*))];
    user.others = const_others[o % (sizeof(const_others)/sizeof(char*))];
    user.perm   = 1;
    user.create = "2019-12-09 14:52:04";
    user.exptime = time(NULL) + 3600*24*365;

    rc = fdr_dbs_user_insert(&user);
    // TEST_RESULT(rc, "fdr_dbs_user_insert");

    return  rc;
}

static int test_user_del(const char *userid){
    int rc;

    rc = fdr_dbs_user_delete(userid);
    // TEST_RESULT(rc, "fdr_dbs_user_delete");

    return rc;
}

static int test_user_update(const char *userid, fdr_dbs_user_t *u){
    int rc;

    rc = fdr_dbs_user_update(userid, u);
    // TEST_RESULT(rc, "fdr_dbs_user_update");

    return  rc;
}

static int test_user_get(const char *userid, fdr_dbs_user_t **user){
    int rc;

    rc = fdr_dbs_user_lookup(userid, user);
    // TEST_RESULT(rc, "fdr_dbs_user_lookup");

    return rc;
}

static int test_user_compare(fdr_dbs_user_t *user0, fdr_dbs_user_t *user1){
    int  rc;

    if((user0->userid != NULL) && (user1->userid != NULL)){
        rc = strcmp(user0->userid, user1->userid);
        if(rc != 0)
            return -1;
    }else{
        return -1;
    }

    if((user0->name != NULL) && (user1->name != NULL)){
        rc = strcmp(user0->name, user1->name);
        if(rc != 0)
            return -2;
    }else{
        return -2;
    }

    if((user0->desc != NULL) && (user1->desc != NULL)){
        rc = strcmp(user0->desc, user1->desc);
        if(rc != 0)
            return -3;
    }else{
        return -3;
    }

    if((user0->others != NULL) && (user1->others != NULL)){
        rc = strcmp(user0->others, user1->others);
        if(rc != 0)
            return -4;
    }

    if(user0->perm != user1->perm)
        return -5;

    if(user0->exptime != user1->exptime)
        return -6;
    

    if((user0->create != NULL) && (user1->create != NULL)){
        rc = strcmp(user0->create, user1->create);
        if(rc != 0)
            return -7;
    }

    return 0;
}

static int test_user_list(int offset, int limit, char ***users){
    int rc;

    rc = fdr_dbs_user_list(offset, limit, users);
    if(rc == limit)
        rc = 0;

    //TEST_RESULT(rc, "fdr_dbs_user_list");

    return 0;
}

static int test_user_exist(const char *userid){
    int rc;

    rc = fdr_dbs_user_exist(userid);
    //TEST_RESULT(rc, "fdr_dbs_user_exist");

    return rc;
}


static int user_test_proc(fdr_dbs_user_t *user, void *handle){
    int *init = (int *)handle;

    // log_info("user_test_proc => index:%05d - userid %s\n", *init, user->userid);

    (*init) ++;

    return 0;
}

static int test_user(int total){
    fdr_dbs_user_t **users;
    fdr_dbs_user_t **users_list;
    char **list_users;
    char userid[64];
    size_t ulen;
    int i, j, find;
    int rc = -1;
      
    users = malloc(sizeof(fdr_dbs_user_t *) * total);
    if(users == NULL){
        log_warn("test_user => no more memory!\n");
        return -1;
    }
    memset(users, 0, sizeof(fdr_dbs_user_t *) * total);

    users_list = malloc(sizeof(fdr_dbs_user_t *) * total);
    if(users_list == NULL){
        log_warn("test_user => no more memory!\n");
        return -1;
    }
    memset(users_list, 0, sizeof(fdr_dbs_user_t *) * total);

    log_info("PASS => init user test\n");

    // add & exist & get & del & exist
    for(i = 0; i < total; i ++){
        ulen = 64;
        rc = fdr_test_userid_generate(userid, &ulen);
        if(rc <= 0){
            log_warn("test_user => fdr_test_userid_generate fail!\n");
            return -1;
        }

        rc = test_user_add(userid, i, i, i);
        if(rc != 0){
            log_warn("test_user => test_user_add fail!\n");
            return -1;
        }

        rc  = test_user_exist(userid);
        if(!rc){
            log_warn("test_user => test_user_exist user %s should exist %d!\n", userid, rc);
            return -1;
        }

        rc = test_user_get(userid, &users[i]);
        if(rc != 0){
            log_warn("test_user => test_user_add fail!\n");
            return -1;
        }

        rc = test_user_del(userid);
        if(rc != 0){
            log_warn("test_user => test_user_del fail!\n");
            return -1;
        }

        rc  = test_user_exist(userid);
        if(rc){
            log_warn("test_user => test_user_exist fail user %s have deleted!\n", userid);
            return -1;
        }
    }

    log_info("PASS => add & exist & get & del & exist\n");

    // fdr_dbs_user_free
    for(i = 0; i < total; i ++){
        rc = fdr_dbs_user_free(users[i]);
        if(rc != 0){
            log_warn("test_user => fdr_dbs_user_free fail!\n");
            return -1;
        }

        users[i] = NULL;
    }

    log_info("PASS => fdr_dbs_user_free\n");

    // add & update
    for(i = 0; i < total; i ++){
        ulen = 64;
        rc = fdr_test_userid_generate(userid, &ulen);
        if(rc <= 0){
            log_warn("test_user => fdr_test_userid_generate fail!\n");
            return -1;
        }


        rc = test_user_add(userid, i, i, i);
        if(rc != 0){
            log_warn("test_user => test_user_add fail!\n");
            return -1;
        }

        rc = test_user_get(userid, &users[i]);
        if(rc != 0){
            log_warn("test_user => test_user_add fail!\n");
            return -1;
        }

        users[i]->name[0] = 'a';
        users[i]->name[1] = 'b';

        users[i]->desc[0] = 'c';
        users[i]->desc[1] = 'd';

        users[i]->others[0] = 'e';
        users[i]->others[1] = 'f';

        users[i]->perm = 2;
        users[i]->exptime = time(NULL) + 12*24*3600;

        rc = test_user_update(userid, users[i]);
        if(rc != 0){
            log_warn("test_user => test_user_update fail!\n");
            return -1;
        }

        rc = test_user_get(userid, &users_list[i]);
        if(rc != 0){
            log_warn("test_user => test_user_get fail!\n");
            return -1;
        }

        rc  = test_user_compare(users[i], users_list[i]);
        if(rc != 0){
            log_warn("test_user => test_user_compare fail!\n");
            return -1;
        }

        rc = fdr_dbs_user_free(users_list[i]);
        if(rc != 0){
            log_warn("test_user => fdr_dbs_user_free fail!\n");
            return -1;
        }

        users_list[i] = NULL;
    }

    log_info("PASS => user  add  & update\n");

    // list
    rc = test_user_list(20, 10, &list_users);
    if(rc != 0){
        log_warn("test_user => test_user_list fail!\n");
        return -1;
    }


    for(i = 0; i < 10; i++){
        find = 0;
        for(j = 0; j < total; j++){
            rc  = strcmp(users[j]->userid, list_users[i]);
            if(rc == 0){
                find = 1;
                break;
            }
        }

        fdr_free(list_users[i]);
        list_users[i] = NULL;

        if(find){
            log_info("PASS => %05d -> id:%s\n", j, users[j]->userid);
            continue;
        }

        log_warn("FAIL => list id not exist\n");
    }

    fdr_free(list_users);

    log_info("PASS => user list\n");

    for(i = 0; i < total; i++){
        rc = fdr_dbs_user_free(users[i]);
        if(rc != 0){
            log_warn("test_user => fdr_dbs_user_free fail!\n");
            return -1;
        }

        users[i] = NULL;
    }

    find = 0;
    rc = fdr_dbs_user_foreach(user_test_proc, &find);

    log_info("PASS => find : %d\n", find);

    fdr_dbs_user_clean();

    log_info("PASS => user functions\n");

    return 0;
}

 
static int test_config(){
    const char *key0 = "test-config0";
    //const char *key1 = "test-config-settings1";
    const char *val0 = "test-config-value0";
    const char *val1 = "test-config-settings-value0";

    int rc;
    char *val;
    
    rc = fdr_dbs_config_set(key0, val0);
    TEST_RESULT(rc, "fdr_dbs_config_set val0");

    rc = fdr_dbs_config_get(key0, &val);
    TEST_RESULT(rc, "fdr_dbs_config_get val0");

    rc = strcmp(val0, val);
    TEST_RESULT(rc, "compare result val == val0");

    fdr_free(val);


    rc = fdr_dbs_config_set(key0, val1);
    TEST_RESULT(rc, "fdr_dbs_config_set val1");

    rc = fdr_dbs_config_get(key0, &val);
    TEST_RESULT(rc, "fdr_dbs_config_get val1");

    rc = strcmp(val1, val);
    TEST_RESULT(rc, "compare result val == val1");

    fdr_free(val);

    return 0;
}

static int test_guest(int total){
    char userid[64];
    char gcode[32];
    size_t ulen;
    time_t exp;
    int i;
    int rc;

    for(i = 0; i < total; i ++){
        fdr_test_userid_generate(userid, &ulen);

        snprintf(gcode, 32, "%06d", i+214560);
        rc = fdr_dbs_guest_insert(i, userid, gcode, time(NULL) - 3600*24*12);
        if(rc != 0){
            log_warn("test_guest => fdr_dbs_guest_insert fail\n");
            return -1;
        }

        rc = fdr_dbs_guest_exist(gcode, &exp);
        if(rc != 0){
            log_warn("test_guest => fdr_dbs_guest_exist fail\n");
            return -1;
        }

        if(i%3 == 0){ 
            fdr_dbs_guest_delete(i);
            if(rc != 0){
                log_warn("test_guest => fdr_dbs_guest_delete fail\n");
                return -1;
            }
        }
    }
    
    fdr_dbs_guest_clean();

    log_info("test_guest => fdr_dbs_guest_clean \n");

    return 0;
}

#define FEATURE_MAX         512
static float gfeature0[FEATURE_MAX] = {};
static float gfeature1[FEATURE_MAX] = {};

static int test_feature(int total){
    char userid[64];
    size_t ulen;
    struct timeval tv;
    float *feature;
    int flen;
    int i;
    int rc;

    for(i = 0; i < FEATURE_MAX; i++){
        gettimeofday(&tv, NULL);
        gfeature0[i] = (float)tv.tv_usec;
        gfeature1[i] = (float)i;
    }

    for(i = 0; i < total; i++){
        fdr_test_userid_generate(userid, &ulen);

        rc = fdr_dbs_feature_insert(userid, gfeature0, sizeof(float)*FEATURE_MAX);
        if(rc != 0){
            log_warn("test_feature => fdr_dbs_feature_insert fail\n");
            return -1;
        }

        
        rc = fdr_dbs_feature_lookup(userid, (void*)&feature, &flen);
        if(rc != 0){
            log_warn("test_feature => fdr_dbs_feature_lookup fail\n");
            return -1;
        }

        rc = memcmp(feature, gfeature0, flen);
        if(rc != 0){
            log_warn("test_feature => memcmp not same\n");
            return -1;
        }
        fdr_free(feature);

        rc = fdr_dbs_feature_update(userid, gfeature1, sizeof(float)*FEATURE_MAX);
        if(rc != 0){
            log_warn("test_feature => fdr_dbs_feature_update fail\n");
            return -1;
        }
        
        rc = fdr_dbs_feature_lookup(userid, (void*)&feature, &flen);
        if(rc != 0){
            log_warn("test_feature => fdr_dbs_feature_lookup fail\n");
            return -1;
        }

        rc = memcmp(feature, gfeature1, flen);
        if(rc != 0){
            log_warn("test_feature => memcmp not same\n");
            return -1;
        }
        fdr_free(feature);

        rc = fdr_dbs_feature_delete(userid);
        if(rc != 0){
            log_warn("test_feature => fdr_dbs_feature_delete fail\n");
            return -1;
        }
    }

    return 0;
}

int main(){
    int rc;
    
    // test dbs, so we got clean dbs
    remove(test_db_sqlite);
    remove(test_feature_sqlite);

    logger_init();
    fdr_mem_init();

    rc = fdr_dbs_init(test_db_sqlite);
    if(rc != 0){
        log_warn("main =>fdr_dbs_init fail!\n");
        return -1;
    }

    rc = fdr_dbs_feature_init(test_feature_sqlite);
    if(rc != 0){
        log_warn("main => fdr_dbs_feature_init fail!\n");
        return -1;
    }

    rc = test_config();
    TEST_RESULT(rc, "test_config");

    rc = test_user(200);
    TEST_RESULT(rc, "test_user");

    rc = test_guest(1200);
    TEST_RESULT(rc, "test_user");
    
    rc = test_feature(1200);
    TEST_RESULT(rc, "test_feature");


    rc =fdr_dbs_fini();
    if(rc != 0){
        log_warn("main => fdr_dbs_fini fail!\n");
        return -1;
    }
    
    rc = fdr_dbs_feature_fini();
    if(rc != 0){
        log_warn("main => fdr_dbs_feature_fini fail!\n");
        return -1;
    }

    fdr_mem_fini();
    
    return 0;
}
