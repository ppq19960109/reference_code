/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__

#include <inttypes.h>
#include <pthread.h>

#define HTTPSERVER_ERRCODE_NOMEM            100
#define HTTPSERVER_ERRCODE_INVALID          101
#define HTTPSERVER_ERRCODE_NOEXIT           102
#define HTTPSERVER_ERRCODE_CHECKSUM         103

#define HTTPSERVER_ADDR_MAX                 64
#define HTTPSERVER_PATH_MAX                 128

#define HTTPSERVER_UPGRADE_SLICE_MAX        (1024*128)

typedef struct httpserver{
    pthread_t thrd;   // thread for main event loop

    char image_path[HTTPSERVER_PATH_MAX];

    struct evhttp   *httpserver;
    struct event_base* base;

    int exitloop;
}httpserver_t;


int httpserver_init(httpserver_t *mp);
int httpserver_fini(httpserver_t *mp);

#endif // __HTTPSERVER_H__

