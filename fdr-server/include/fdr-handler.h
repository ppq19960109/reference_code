/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __FDR_HANDLER_H__
#define __FDR_HANDLER_H__

#include "cJSON.h"

// function declarations
extern int handler_info     (cJSON *req, cJSON *rsp);
extern int handler_config   (cJSON *req, cJSON *rsp);
extern int handler_user     (cJSON *req, cJSON *rsp);
extern int handler_logger   (cJSON *req, cJSON *rsp);
extern int handler_restart  (cJSON *req, cJSON *rsp);
extern int handler_guest    (cJSON *req, cJSON *rsp);

#endif //__FDR_HANDLER_H__
