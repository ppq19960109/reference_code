/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */
#include "fdr-qrcode.h"
#include "logger.h"
#include "fdr-path.h"
#include "fdr-license.h"

#include "base58.h"
#include "sha256.h"
#include "ecc.h"

#include <stdlib.h>
#include <string.h>

#define FDR_QRCODE_MAX_STRING       256
#define FDR_QRCODE_CHECKSUM_MAX     32

static uint8_t ecc_pubkey[ECC_BYTES+1];
static uint8_t ecc_prikey[ECC_BYTES];

int fdr_qrcode_sign(void *bin, size_t len, void *checksum){
    uint8_t shacheck[FDR_QRCODE_CHECKSUM_MAX];
    sha256_context ctx;

    sha256_init(&ctx);
    sha256_hash(&ctx, bin, len);
    sha256_hash(&ctx, ecc_prikey, ECC_BYTES);
    sha256_done(&ctx, shacheck);

    memcpy(checksum, shacheck, FDR_CHECKSUM_MAX);

    return 0;
}

int fdr_qrcode_verify(void *bin, size_t len, void *checksum){
    uint8_t shacheck[FDR_QRCODE_CHECKSUM_MAX];
    sha256_context ctx;

    sha256_init(&ctx);
    sha256_hash(&ctx, bin, len);
    sha256_hash(&ctx, ecc_prikey, ECC_BYTES);
    sha256_done(&ctx, shacheck);

    return memcmp (shacheck, checksum, FDR_CHECKSUM_MAX);
}

int fdr_ecc_sign(void *bin, size_t len, void *checksum){
    uint8_t shacheck[FDR_QRCODE_CHECKSUM_MAX];
    sha256_context ctx;
    int rc;

    sha256_init(&ctx);
    sha256_hash(&ctx, bin, len);
    sha256_done(&ctx, shacheck);

    rc = ecdsa_sign(ecc_prikey, shacheck, checksum);
    if(rc == 1){
        return 0;
    }else{
        log_warn("ecc sign fail : %d\n", rc);
        return -1;
    }
}

int fdr_ecc_verify(void *bin, size_t len, void *checksum){
    uint8_t shacheck[FDR_QRCODE_CHECKSUM_MAX];
    sha256_context ctx;

    sha256_init(&ctx);
    sha256_hash(&ctx, bin, len);
    sha256_done(&ctx, shacheck);
    int rc;

    rc = ecdsa_verify(ecc_pubkey, shacheck, checksum);
    if(rc == 1){
        return 0;
    }else{
        log_warn("ecc verify fail : %d\n", rc);
        return -1;
    }
}

int fdr_qrcode_init(){
    memcpy(ecc_pubkey, fdr_get_dev_pubkey(), ECC_BYTES+1);
    memcpy(ecc_prikey, fdr_get_dev_prikey(), ECC_BYTES);
    return 0;
}

