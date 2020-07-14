/*
 * Copyright (c) 2020, Bashi Tech. All rights reserved.
 */

#ifndef __FDRCP_H__
#define __FDRCP_H__

#include "list.h"

#include <inttypes.h>
#include <time.h>

#define FDRCP_FACE_MAX              8
#define FDRCP_FEATURE_MAX           512

#define FDRCP_UIWIDTH               128
#define FDRCP_UIHEIGHT              128

#define FDRCP_USRID_MAX             34
#define FDRCP_UNAME_MAX             16
#define FDRCP_UDESC_MAX             32
#define FDRCP_UOTHR_MAX             32

#define FDRCP_VCODE_MAX             8

enum {
    FDRCP_EVMSG_DATA = 0,
    FDRCP_EVMSG_KEY,
    FDRCP_EVMSG_QRCODE,
    FDRCP_EVMSG_THERM,
    FDRCP_EVMSG_USER_APPEND,
    FDRCP_EVMSG_USER_UPDATE,
    FDRCP_EVMSG_USER_DELETE,
    FDRCP_EVMSG_VCODE_INSERT,
    FDRCP_EVMSG_VCODE_DELETE,
    FDRCP_EVMSG_START,
    FDRCP_EVMSG_SNAPSHOT,
    FDRCP_EVMSG_MAX
};

typedef struct {
    int x;  // face left in image
    int y;  // face top in image
    int w;  // face width
    int h;  // face height
}fdrcp_rect_t;

typedef struct {
    int tfid;   // tracked face id
    int alive;

    fdrcp_rect_t rect;  // face rect in image

    float sharp;
    float score;

    float yaw;
    float roll;
    float pitch;
}fdrcp_fd_t;

typedef struct {
    int tfid;

    float feature[FDRCP_FEATURE_MAX];
}fdrcp_fr_t;


typedef struct {
    struct list_head node;

    int64_t timestamp;

    int fdnum;
    fdrcp_fd_t  fds[FDRCP_FACE_MAX];

    int frnum;
    fdrcp_fr_t  frs[FDRCP_FACE_MAX];

    void *frame;
}fdrcp_data_t;

// cache for face match
typedef struct {
    char usrid[FDRCP_USRID_MAX];
    char uname[FDRCP_UNAME_MAX];
    char udesc[FDRCP_UDESC_MAX];
    char uothr[FDRCP_UOTHR_MAX];

    int  rule;
    time_t expired;

    float feature[FDRCP_FEATURE_MAX];

    struct list_head node;
    int64_t timestamp;
    int tfid;       // for track & cache
}fdrcp_user_t;

typedef struct {
    struct list_head node;

    int  vcid;
    char vcode[FDRCP_VCODE_MAX];
    time_t start;
    time_t expire;
}fdrcp_vcode_t;

typedef struct {
    int rule;

    int mode;
    char days[32];
    short start;
    short end;
}fdrcp_rule_t;

int fdrcp_init();
int fdrcp_fini();

int fdrcp_start();
int fdrcp_snapshot(int64_t timestamp);

fdrcp_data_t *fdrcp_acquire();
void fdrcp_release(fdrcp_data_t *fdr);
int fdrcp_fdrdispatch(fdrcp_data_t *fdr);

int fdrcp_keydispatch(int status);
int fdrcp_qrcdispatch(const char *qrcode);
int fdrcp_thermdispatch(float therm);

fdrcp_vcode_t *fdrcp_acquire_vcode();
void fdrcp_release_vcode(fdrcp_vcode_t *vcode);
int fdrcp_vtcdispatch(int type, fdrcp_vcode_t *vcode);

fdrcp_user_t *fdrcp_acquire_user();
void fdrcp_release_user(fdrcp_user_t *user);
int fdrcp_usrdispatch(int type, fdrcp_user_t *user);

#endif // __FDRCP_H__
