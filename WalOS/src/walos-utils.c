/*
 * Copyright (c) 2020, Bashi Tech. All rights reserved.
 */

#include "sha256.h"
#include "ecc.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <arpa/inet.h>

typedef struct bcp_header{
    uint16_t  mtype;
    uint16_t  bsize;
    uint32_t  seqence;
    uint64_t  session;
}__attribute__((packed)) bcp_header_t;

void bcpheader_tonetwork(struct bcp_header *bh){
    bh->mtype    = htons(bh->mtype);
    bh->bsize    = htons(bh->bsize);
    bh->seqence     = htonl(bh->seqence);
    //
    // bh->session     = htonl(bh->session);
}

void bcpheader_tohost(struct bcp_header *bh){
    bh->mtype    = ntohs(bh->mtype);
    bh->bsize    = ntohs(bh->bsize);
    bh->seqence     = ntohl(bh->seqence);
    // bh->session     = ntohl(bh->session);
}

int ECDSA_Sign(const void *prikey, const char *devid, const char *ts, void *checksum){
    uint8_t shacheck[SHA256_HASH_BYTES];
    sha256_context ctx;
    int rc;

    if((prikey == NULL)||(devid == NULL)||(ts == NULL)||(checksum == NULL)){
        fprintf(stderr, "ECDSA_Sign => param wrong\n");
        return -1;
    }

    sha256_init(&ctx);
    sha256_hash(&ctx, (uint8_t*)devid, strlen(devid));
    sha256_hash(&ctx, (uint8_t*)ts, strlen(ts));
    sha256_done(&ctx, shacheck);
        
    rc = ecdsa_sign(prikey, shacheck, checksum);
    if(rc == 1){
        return 0;
    }else{
        fprintf(stderr, "ECDSA_Sign => Fail : %d\n", rc);
        return -1;
    }
}

int ECDSA_Verify(const void *pubkey, const char *devid, const char *ts, const char *checksum){
    uint8_t shacheck[SHA256_HASH_BYTES];
    sha256_context ctx;
    int rc;

    if((pubkey == NULL)||(devid == NULL)||(ts == NULL)||(checksum == NULL)){
        fprintf(stderr, "ECDSA_Verify => param wrong\n");
        return -1;
    }
    sha256_init(&ctx);
    sha256_hash(&ctx, (uint8_t*)devid, strlen(devid));
    sha256_hash(&ctx, (uint8_t*)ts, strlen(ts));
    sha256_done(&ctx, shacheck);

    rc = ecdsa_verify(pubkey, shacheck, (const uint8_t*)checksum);
    if(rc == 1){
        return 0;
    }else{
        fprintf(stderr, "ECDSA_Verify => Fail : %d\n", rc);
        return -1;
    }
}

double Timestamp(){
    double r = -1;
    struct timeval tv;

    if(gettimeofday(&tv,NULL) != 0){
        return r;
    }

    r = tv.tv_sec;
    r = r*1000 + tv.tv_usec/1000;

    return r;
}


#if 0

#define FDR_DEVICE_ID_MAX_SIZE  36

typedef struct {
    uint8_t device_id[FDR_DEVICE_ID_MAX_SIZE];
    uint8_t dev_pubkey[ECC_BYTES + 1];
    uint8_t dev_prikey[ECC_BYTES];
    uint8_t svr_pubkey[ECC_BYTES + 1];
}license_t;

/*
int32_t fdr_license_init(void);

uint8_t *fdr_get_device_id(void);

uint8_t *fdr_get_dev_pubkey(void);

uint8_t *fdr_get_dev_prikey(void);

uint8_t *fdr_get_svr_pubkey(void);
*/

#endif
