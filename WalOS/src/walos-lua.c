/*
 * Copyright (c) 2020, Bashi Tech. All rights reserved.
 */

#include "sha256.h"
#include "ecc.h"

#include <stdlib.h>
#include <string.h> 
#include <limits.h>
#include <unistd.h>
#include <pthread.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


static const char *script_fmt = "%s/%s";
static const char *cpath_fmt  = "%s/lib/?.so;";
static const char *lpath_fmt  = "%s/modules/?.lua;%s/app/?.bin;";
static const char *ldpath_fmt  = "%s/lib";
static lua_State  *lvm;
static char basepath[PATH_MAX];
static char scriptfile[PATH_MAX];
static char envstr[PATH_MAX];

static int luavm_start(){
    char *bp = NULL, *sc = NULL;
    int errhandler;
    int ret;

    fprintf(stderr, "luavm_start ....\n");

    bp = ".";
    sc = "app/WalOS.bin";

    bp = realpath(bp, basepath);
    if(bp == NULL){
        fprintf(stderr, "FATAL : resolve path fail:%s\n", bp);
        return -1;
    }

    snprintf(scriptfile, PATH_MAX, script_fmt, basepath, sc);
    if(access(scriptfile, F_OK|R_OK) != 0){
        fprintf(stderr, "FATAL : scripts %s not exist or no execute permission\n", scriptfile);
        return -1;
    }

    snprintf(envstr, PATH_MAX, cpath_fmt, basepath);
    setenv("LUA_CPATH", envstr, 1);
    fprintf(stderr, "INFO : LUA_CPATH=%s\n", envstr);

    snprintf(envstr, PATH_MAX, lpath_fmt, basepath, basepath);
    setenv("LUA_PATH", envstr, 1);
    fprintf(stderr, "INFO : LUA_PATH=%s\n", envstr);

    snprintf(envstr, PATH_MAX, ldpath_fmt, basepath);
    setenv("LD_LIBRARY_PATH", envstr, 1);
    fprintf(stderr, "INFO : LD_LIBRARY_PATH=%s\n", envstr);


    lvm = luaL_newstate();
    if(lvm == NULL){
        fprintf(stderr, "FATAL : create lua state fail\n");
        return -1;
    }

    luaL_openlibs(lvm);

    ret = luaL_loadfile(lvm, scriptfile);
    if(ret != 0){
        fprintf(stderr, "FATAL : load lua script :%s fail:%d\n", scriptfile, ret);
        return -1;
    }

    ret = lua_pcall(lvm, 0, 0, 0);
    if(ret != 0){
        int top = lua_gettop(lvm);
        if(top > 0){
            if(lua_isstring(lvm, top) != 0){
                fprintf(stderr, "FATAL : call global code fail:%s\n", lua_tolstring(lvm, top, NULL));
            }
        }else{
            fprintf(stderr, "FATAL : call global code fail:%d\n", ret);
        }

        return -1;
    }

    lua_getglobal(lvm, "ErrorHandler");
    errhandler = lua_gettop(lvm);

    lua_getglobal(lvm, "Startup");
    lua_pushstring(lvm, basepath);
    ret = lua_pcall(lvm, 1, 1, errhandler);
    if(ret != 0){
        int top = lua_gettop(lvm);
        if(top > 0){
            if(lua_isstring(lvm, top) != 0){
                fprintf(stderr, "FATAL : call code fail:%s\n", lua_tolstring(lvm, top, NULL));
            }
        }else{
            fprintf(stderr, "FATAL : lua_pcall fail:%d\n", ret);
        }
    }

    lua_pop(lvm, 1);

    return ret;
}

static void *fdrmp_thread_proc(void *args){

    luavm_start();

    return NULL;
}

int fdrmp_init(){
    pthread_t thrd;
    int rc;

    fprintf(stderr, " INFO : fdrmp_init => ...\n");

    rc = pthread_create(&thrd, NULL, fdrmp_thread_proc, NULL);
    if(rc != 0){
        fprintf(stderr, "FATAL : fdrmp_init => create tick thread fail %d\n", rc);
        return -1;
    }

    return 0;
}