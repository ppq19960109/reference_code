/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "ecc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ecc_make_key(uint8_t p_publicKey[ECC_BYTES+1], uint8_t p_privateKey[ECC_BYTES]);

int write_to_file(const char *file, uint8_t *buf, int len){
    FILE    *f;
    size_t sz;

    f = fopen(file, "w");
    if(f != NULL){
        sz = fwrite(buf, 1, len, f);
        fclose(f);

        if(sz != len)
            return  -1;
        else
            return 0;
    }

    return -1;
}

int gen_ecc_keys(){
    uint8_t pubkey[ECC_BYTES + 1];
    uint8_t prikey[ECC_BYTES];
    int rc;

    rc = ecc_make_key(pubkey, prikey);
    if(rc != 1){
        printf("make key fail\n");
        return -1;
    }

    rc = write_to_file("pubkey.ecc", pubkey, ECC_BYTES + 1);
    if(rc != 0){
        printf("write pubkey.ecc fail\n");
        return -1;
    }


    rc = write_to_file("prikey.ecc", prikey, ECC_BYTES);
    if(rc != 0){
        printf("write prikey.ecc fail\n");
        return -1;
    }
    
    printf("genkey have write key to pubkey.ecc, prikey.ecc\n");
    return 0;
}

int main(){
    printf("genkey will write to prikey.ecc, pubkey.ecc\n");

    gen_ecc_keys();
    
    return 0;
}