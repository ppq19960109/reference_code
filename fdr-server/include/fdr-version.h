/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef _FDR_VERSION_H_
#define _FDR_VERSION_H_

#include "stdint.h"


#ifdef TEST_VERSION
#define FDR_VERSION_PREFIX  "T"
#define FDR_VERSION_TAG     "Test"
#else
#define FDR_VERSION_PREFIX  "V"
#define FDR_VERSION_TAG     "Release"
#endif

#define FDR_VERSION_MAJOR   3
#define FDR_VERSION_MINOR   4
#define FDR_VERSION_PATCH   0

#define FDR_PRODUCTID   "BFA1050"

int32_t fdr_productid_init(void);

int32_t fdr_version_init(void);


#endif
