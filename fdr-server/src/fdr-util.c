/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr-util.h"
#include "fdr.h"

#include "fdr-path.h"

#include "logger.h"

//#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <time.h>

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

time_t fdr_util_strptime(const char *buf){
    struct tm timestamp;

    if(buf == NULL){
        return -1;
    }

    if(strptime(buf, "%Y-%m-%d %H:%M:%S", &timestamp) == NULL){
        return -1;
    }

    return mktime(&timestamp);
}

int fdr_util_mkdir(char *buffer, int64_t ts){
    time_t tm = (time_t)(ts / 1000);
    struct tm t;

    //setenv("TZ", "GMT-8", 1);
    if(localtime_r(&tm, &t) == NULL){
        log_warn("fdr_util_mkdir => get gmtime fail\n");
        return -1;
    }

    sprintf(buffer, "%s/%04d-%02d-%02d", CAPTURE_IMAGE_PATH, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

    if(access(buffer, F_OK) != 0){
        mkdir(buffer, S_IRWXU|S_IRWXG|S_IRWXO);
    }

    return 0;
}
#if 1
int fdr_util_imagepath(char *buffer, int64_t ts){

    time_t tm = (time_t)(ts / 1000);
    struct tm t;

    //setenv("TZ", "GMT-8", 1);
    if(localtime_r(&tm, &t) == NULL){
        log_warn("get gmtime fail\n");
        return -1;
    }

    sprintf(buffer, "%s/%04d-%02d-%02d/%02d-%02d-%02d.%03lu.jpg",
                            CAPTURE_IMAGE_PATH, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                            t.tm_hour, t.tm_min, t.tm_sec, (unsigned long)(ts % 1000));

    return 0;
}

int fdr_util_oplogpath(char *buffer, int64_t ts){
    return -1;
}

#endif

int fdr_util_foreach_dir(const char *path, fdr_util_dir_iterator iterator, void *handle){
    DIR  *rdir;
    struct dirent *next;
    int rc = -1;

    rdir = opendir(path);
    if(rdir == NULL){
        return rc;
    }

    while((next = readdir(rdir)) != NULL){
        if((strcmp(next->d_name, ".") == 0) || (strcmp(next->d_name, "..") == 0)){
            continue;
        }

        rc = iterator(handle, next, path);
        if(rc != 0){
            break;
        }
    }

    closedir(rdir);

    return rc;
}

int fdr_util_read_file(const char *filepath, uint8_t **buf, int *len){
    FILE *fj;
    long size;
    size_t rlen;
    uint8_t *fbuf;

    fj = fopen(filepath, "r");
    if(fj == NULL){
        log_warn("fdr_util_read_file => open %s fail!\n", filepath);
        return -1;
    }

	fseek(fj, 0L, SEEK_END);
	size = ftell(fj);
	fseek(fj, 0L, SEEK_SET);

    fbuf = fdr_malloc(size);
    if(fbuf == NULL){
        log_warn("fdr_util_read_file => no memory for load %s\n", filepath);
        fclose(fj);
        return -1;
    }

    rlen = fread(fbuf, 1, size, fj);
    if(rlen != (size_t)size){
        log_warn("fdr_util_read_file => load fail %lu, %ld!\n", size, rlen);
        fdr_free(fbuf);
        fclose(fj);
        return -1;
    }
    fclose(fj);

    *buf = fbuf;
    *len = (int)size;

    return 0;
}
