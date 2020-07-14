#include "base58.h"

#include <stdlib.h>
#include <stdio.h>


const char *feature = "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";

uint8_t binbuf[512];

int main(){
    ssize_t olen;
    size_t  blen = 512;
    float *f;

    olen = base58_decode(feature, strlen(feature), binbuf, blen);
    if(olen > 0){

        printf("base58 to bin ok :%d  %d\n", strlen(feature), olen);

        f = (float *)binbuf;
        for(int i = 0; i < 10; i++){
            printf("bin %d -> %f\n", i, f[i]);
        }
    
        return 0;

    }else{
        printf("base58 to bin fail %d %d\n", strlen(feature), olen);
        return -1;
    }
}
