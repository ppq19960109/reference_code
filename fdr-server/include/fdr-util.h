/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __FDR_UTIL_H__
#define __FDR_UTIL_H__

#include <inttypes.h>
#include <time.h>
#include <dirent.h>

time_t fdr_util_strptime(const char *buf);

int fdr_util_rm(const char *dir);

// ts -> year-month-day
int fdr_util_mkdir(char *buffer, int64_t ts);
int fdr_util_rmdir(char *buffer, int64_t ts);

int fdr_util_read_file(const char *filepath, uint8_t **buf, int *len);

typedef int (*fdr_util_dir_iterator)(void *handle, struct dirent *de, const char *path);
int fdr_util_foreach_dir(const char *path, fdr_util_dir_iterator iterator, void *handle);


int fdr_util_imagepath(char *buffer, int64_t ts);
int fdr_util_oplogpath(char *buffer, int64_t ts);

#endif // __FDR_UTIL_H__

