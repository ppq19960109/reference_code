/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __FDR_LOGGER_H__
#define __FDR_LOGGER_H__

#ifdef __cplusplus
extern "C"{
#endif
#include "fdr-dbs.h"
#include "bisp_aie.h"

#include <inttypes.h>


typedef struct fdr_logger_record{
    int64_t seqnum;     // 记录序号
    int64_t occtime;    // 时间戳, 毫秒数，自1970.01.01 00:00:00, 隐含：图片ID, 目录：年月日，文件名：时间戳
    int face_x;     // 人脸位置， x position
    int face_y;     // 人脸位置， y position
    int face_w;     // 人脸宽度
    int face_h;     // 人脸高度
    float sharp;    // 检测质量
    float score;    // 识别比率
    char userid[FDR_DBS_USERID_SIZE+1];  // 识别出的人脸ID， NULL 为陌生人, +1 for '\0'
} fdr_logger_record_t;

int fdr_logger_init(const char *dbfile);
int fdr_logger_fini();

int64_t fdr_logger_append(int64_t octime, bisp_aie_rect_t *rect, float sharp, float score, const char *userid);

int fdr_logger_get(int64_t seqnum, fdr_logger_record_t *lr);
int fdr_logger_saveimage(int64_t octime, void *frame, int width, int heigh);

int fdr_logger_max(int64_t *max);

void fdr_logger_clean();

#ifdef __cplusplus
}
#endif

#endif // __FDR_LOGGER_H__
