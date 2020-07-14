#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "bitmap_rotate.h"

#define BITMAP_TEST 0

#define DEBUG 0

#if DEBUG
#define debug printf
#else
#define debug(...)
#endif

int32_t bitmap_show(const uint8_t *bitmap, uint32_t w, uint32_t h)
{
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t offset = 0;

    printf("bitmap: \r\n");
    for (y = 0; y < h; y++){
        for(x = 0; x < w; x++) {
            offset = y * w + x;
            printf("%d ", bitmap[offset]);
        }
        printf("\r\n");
    }
    
    return 0;
}

static int32_t bitmap_left_rotate(const uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h)
{
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t dx = 0;
    uint32_t sx = 0;
    
    for(y = 0; y < h; y++) {
        for(x = 0; x < w; x++) {
            dx = y + (w - 1 - x) * h;
            sx = x + y * w;
            dst[dx] = src[sx];
            debug("%d - %d - %d - %d\r\n", dx, sx, src[sx], dst[dx]);
        }
    }
    
    return 0;
}

static int32_t bitmap_right_rotate(const uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h)
{
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t dx = 0;
    uint32_t sx = 0;

    for(y = 0; y < h; y++) {
        for(x = 0; x < w; x++) {
            dx = (h - 1 - y) + x * h;
            sx = x + y * w;
            dst[dx] = src[sx];
            debug("%d - %d - %d - %d\r\n", dx, sx, src[sx], dst[dx]);
        }
    }
    
    return 0;
}

int32_t bitmap_rotate(const uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h, uint8_t mode)
{
    debug("\r\nrotate mode: %d - %d - %d\r\n", mode, w, h);
    if(LEFT_ROTATE == mode) {
        return bitmap_left_rotate(src, dst, w, h);
    } else if(RIGHT_ROTATE == mode) {
        return bitmap_right_rotate(src, dst, w, h);
    }
    
    return -1;
}

#if defined(BITMAP_TEST) && (0 != BITMAP_TEST)

#define BITMAP_W 5
#define BITMAP_H 6

int main(int argc, char *argv[])
{
    uint8_t a[BITMAP_W *BITMAP_H] = {0};
    uint8_t b[BITMAP_W *BITMAP_H] = {0};
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t w = BITMAP_W;
    uint32_t h = BITMAP_H;
    uint32_t n = 1;
    uint32_t offset = 0;
#if 0
    uint8_t mode = RIGHT_ROTATE;
#else
    uint8_t mode = LEFT_ROTATE;
#endif
    
    for(y = 0; y < h; y++) {
        for(x = 0; x < w; x++) {
            offset = y * w + x;
            a[offset] = n++;
        }
    }
    
    bitmap_show(a, w, h);
    //bitmap_show(a, w, h);
    
    bitmap_rotate(a, b, w, h, mode);
    bitmap_show(a, w, h);
    bitmap_show(b, h, w);

#if 1
    bitmap_rotate(b, a, h, w, mode);
    bitmap_show(b, h, w);
    bitmap_show(a, w, h);
    
    bitmap_rotate(a, b, w, h, mode);
    bitmap_show(a, w, h);
    bitmap_show(b, h, w);
    
    bitmap_rotate(b, a, h, w, mode);
    bitmap_show(b, h, w);
    bitmap_show(a, w, h);
#endif 
    return 0;
}

#endif
