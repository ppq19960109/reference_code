
#ifndef _FDR_LICENSE_H_
#define _FDR_LICENSE_H_

#include "stdint.h"
#include "ecc.h"

#define FDR_DEVICE_ID_MAX_SIZE  36

typedef struct {
    uint8_t device_id[FDR_DEVICE_ID_MAX_SIZE];
    uint8_t dev_pubkey[ECC_BYTES + 1];
    uint8_t dev_prikey[ECC_BYTES];
    uint8_t svr_pubkey[ECC_BYTES + 1];
}license_t;


int32_t fdr_license_init(void);

uint8_t *fdr_get_device_id(void);

uint8_t *fdr_get_dev_pubkey(void);

uint8_t *fdr_get_dev_prikey(void);

uint8_t *fdr_get_svr_pubkey(void);

#endif
