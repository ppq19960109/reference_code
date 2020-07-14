/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __FDR_QRCODE_H__
#define __FDR_QRCODE_H__

#include <stdio.h>

#define FDR_USERID_MAX      16
#define FDR_DEVID_MAX       16
#define FDR_CODE_MAX        6
#define FDR_PASSWD_MAX      12     // only use 8 characters
#define FDR_CHECKSUM_MAX    8      // sha256, prefix 8 characters

// offline
typedef struct fdr_qrcode_visitor{
    unsigned char magic[4];  // "BFAV"
    unsigned int duration;        // seconds from 1970.1.1 00:00, in net endian
    unsigned char userid[FDR_USERID_MAX];
    unsigned char checksum[FDR_CHECKSUM_MAX];
}__attribute__((packed)) fdr_qrcode_visitor_t;

// online
typedef struct fdr_qrcode_simple{
    unsigned char magic[4];             // "BFAS"
    unsigned char code[FDR_CODE_MAX];   // only use 6 characters
}__attribute__((packed)) fdr_qrcode_simple_t;

typedef struct fdr_qrcode_config{
    unsigned char magic[4];  // "BFAC"
    unsigned int duration;        // seconds from 1970.1.1 00:00, in net endian
    unsigned char passwd[FDR_PASSWD_MAX];
    unsigned char checksum[FDR_CHECKSUM_MAX];
}__attribute__((packed)) fdr_qrcode_config_t;

typedef struct fdr_qrcode_active{
    unsigned char magic[4];  // "BFAA"
    unsigned int duration;        // seconds from 1970.1.1 00:00, in net endian
    unsigned char devid[FDR_DEVID_MAX];
    unsigned char checksum[FDR_CHECKSUM_MAX];
}__attribute__((packed)) fdr_qrcode_active_t;

int b58_to_binary(const char *b58, size_t len, void *bin, size_t *outlen);
int binary_to_b58(const void *bin, size_t len, char **b58, size_t *outlen);

int fdr_qrcode_sign(void *bin, size_t len, void *checksum);
int fdr_qrcode_verify(void *bin, size_t len, void *checksum);

int fdr_qrcode_init();


#endif // __FDR_QRCODE_H__
