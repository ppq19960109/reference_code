/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "fdr-test.h"

#include "logger.h"
#include "base58.h"

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#if defined(_WIN32)
#include <windows.h>
#include <wincrypt.h>
#endif

static inline int generate_128bits_random(uint64_t *seed) {
  int res;

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
  FILE *fp = fopen("/dev/urandom", "rb");
  if(!fp) {
    return -1;
  }

  res = fread(seed, 1, sizeof(uint64_t) *2, fp);
  fclose(fp);

  if (res != (sizeof(uint64_t)*2)) {
    return -1;
  }

#elif defined(_WIN32)
  HCRYPTPROV hCryptProv;
  res = CryptAcquireContext(
    &hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
  if (!res) {
    return -1;
  }

  res = CryptGenRandom(hCryptProv, (DWORD) sizeof(uint64_t)*2, (PBYTE) seed);
  CryptReleaseContext(hCryptProv, 0);
  if (!res) {
    return -1;
  }

#else
  #error "unsupported platform"
#endif

  return 0;
}


int fdr_test_userid_generate(char *buf, size_t *buflen){
    uint64_t seed[2];
    int rc;

    rc = generate_128bits_random(seed);
    if(rc != 0){
        log_warn("fdr_test_userid_generate => get random number fail\n");
        return -1;
    }

    if(base58_encode(seed, sizeof(uint64_t)*2, buf, buflen) != NULL){
    //if(b58enc(buf, buflen, seed, sizeof(uint64_t)*2)){
        buf[*buflen] = 0;

        return *buflen;
    }

    return -1;
}

typedef struct fdr_mem_test{
    pthread_mutex_t lock;
    int count;
}fdr_mem_test_t;

static fdr_mem_test_t gtest;
static fdr_mem_test_t *get_mem(){
    return &gtest;
}

int fdr_mem_init(){
    
    fdr_mem_test_t *tm = get_mem();

    pthread_mutex_init(&tm->lock, NULL);
    tm->count = 0;

    return 0;
}

int fdr_mem_fini(){
    fdr_mem_test_t *tm = get_mem();

    if(tm->count > 0){
        log_warn("fdr_mem_fini => memory leak %d\n", tm->count);
    }else if(tm->count < 0){
        log_warn("fdr_mem_fini => memory free more than alloc %d\n", tm->count);
    }else{
        log_info("fdr_mem_fini => alloc == free\n");
    }

    return 0;
}

void *fdr_malloc(size_t size){
    void *p;

    fdr_mem_test_t *tm = get_mem();

    pthread_mutex_lock(&tm->lock);

    p = malloc(size);
    if(p != NULL)
        tm->count ++;

    pthread_mutex_unlock(&tm->lock);

    return p;
}

void fdr_free(void *ptr){
    fdr_mem_test_t *tm = get_mem();
    
    pthread_mutex_lock(&tm->lock);
    
    if(ptr != NULL)
        tm->count --;

    pthread_mutex_unlock(&tm->lock);

    return free(ptr);
}

