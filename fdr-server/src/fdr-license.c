
#include "fdr-license.h"
#include "fdr-path.h"
#include "logger.h"

#include "base58.h"
#include "cJSON.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
device.json

{
    "device_id": "43VBeFm8t4DBDmtEqgayfP",
    "device_private_key": "CP7kZMmRbACH3SRTNLkyybMBH4eUSQfijYagM8VvWcVY",
    "device_public_key": "2BeaYQBn4yv76jwDwwyUDLnpTNw1eAnthj4xWH6xZtLi9",
    "mac": "8C:EC:4B:00:01:35",
    "server_private_key": "",
    "server_public_key": "z1TF7ZmVCwfjwbSQW941y7ZxQRwD9ksPxumWVEP9tkd1"
}
*/


static license_t license_info;

static int32_t fdr_license_parse_one(char *bs58_val, uint8_t *value, uint32_t val_len)
{
    if(NULL == bs58_val || NULL == value || 0 == val_len) {
        return -1;
    }

    if(val_len != base58_decode(bs58_val, strlen(bs58_val), value, val_len)) {
        return -1;
    }

    return 0;
}

static int32_t fdr_license_parse(uint8_t *license)
{
    int32_t rc = -1;
    char *devid = NULL;
    cJSON *root = cJSON_Parse((char *)license);

    if(NULL == root) {
        log_error("parse license failed\r\n");
        return -1;
    }

    devid = cJSON_GetObjectString(root, "device_id");
    if(NULL == devid) {
        log_error("parse device_id failed\r\n");
        goto fail;
    }
    strncpy((char *)license_info.device_id, devid, FDR_DEVICE_ID_MAX_SIZE);
    
    if(fdr_license_parse_one(cJSON_GetObjectString(root, "device_public_key"), license_info.dev_pubkey, ECC_BYTES+1)
        || fdr_license_parse_one(cJSON_GetObjectString(root, "device_private_key"), license_info.dev_prikey, ECC_BYTES)
        || fdr_license_parse_one(cJSON_GetObjectString(root, "server_public_key"), license_info.svr_pubkey, ECC_BYTES+1)) {
        log_error("parse license key failed\r\n");
        goto fail;
    }
    
    rc = 0;

fail:
    cJSON_Delete(root);
    return rc;
}

static uint8_t *fdr_license_read(void)
{
    FILE *f = NULL;
    uint32_t sz = 0;
    uint8_t *license = NULL;

    f = fopen(LICENSE_FILENAME, "r");
    if(NULL == f) {
        log_warn("no dev info\r\n");
        return NULL;
    }

    if(fseek(f, 0, SEEK_END)) {
        log_warn("fseek end fail\r\n");
        goto fail;
    }
    sz = ftell(f);
    license = (uint8_t *)malloc(sz + 1);
    if(NULL == license) {
        log_warn("malloc fail\r\n");
        goto fail;
    }
    memset(license, 0, sz + 1);
    if(fseek(f, 0, SEEK_SET)) {
        log_warn("fseek start fail\r\n");
        goto fail;
    }

    if(sz != fread(license, 1, sz, f)) {
        log_warn("read fail\r\n");
        goto fail;
    }

    fclose(f);
    return license;
    
fail:
    fclose(f);
    if(license) {
        free(license);
    }
    return NULL;
}

int32_t fdr_license_init(void)
{
    int32_t rc = -1;
    uint8_t *license = fdr_license_read();

    if(NULL == license) {
        return -1;
    }
    rc = fdr_license_parse(license);
    free(license);

    return rc;
}

uint8_t *fdr_get_device_id(void)
{
    return license_info.device_id;
}

uint8_t *fdr_get_dev_pubkey(void)
{
    return license_info.dev_pubkey;
}

uint8_t *fdr_get_dev_prikey(void)
{
    return license_info.dev_prikey;
}

uint8_t *fdr_get_svr_pubkey(void)
{
    return license_info.svr_pubkey;
}

