
#ifndef _ZXING_WRAPPER_H_
#define _ZXING_WRAPPER_H_

#ifdef __cplusplus
extern "C" {
#endif



int32_t qrcode_read(char *rt, uint32_t rlen, uint8_t *data, uint32_t x, uint32_t y, uint32_t w, uint32_t h);



#ifdef __cplusplus
}
#endif

#endif
