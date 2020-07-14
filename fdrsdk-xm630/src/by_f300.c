
#include "sys_comm.h"
#include "hal_uart.h"
#include "hal_log.h"
#include "hal_gpio.h"
#include "hal_485.h"

#include "by_f300.h"

#define BY_F300_FRAME_LEN  16
typedef struct {
    uint8_t buf[BY_F300_FRAME_LEN];
    uint32_t exit;
    pthread_t tid;
    pthread_mutex_t mutex;
}by_f300_context_t;

static by_f300_context_t byf300_ctx;

static void frame_append(uint8_t *buf, uint32_t offset, uint8_t data)
{
    buf[offset] = data;
}

static uint8_t frame_chsum(uint8_t *buf, uint32_t start, uint32_t len)
{
    uint32_t i = 0;
    uint8_t sum = 0;
    
    for(i = 0; i < len; i++) {
        sum ^= buf[start + i];
    }
    return sum;
}

static void frame_debug(uint8_t *buf, uint32_t len)
{
    uint32_t i = 0;
    
    hal_printf("frame: ");
    for(i = 0; i < len; i++) {
        hal_printf("%02x ", buf[i]);
    }
    hal_printf("\r\n");
}

int32_t by_f300_play(uint16_t num)
{
    by_f300_context_t *ctx = &byf300_ctx;
    uint8_t len = 0x05;
    
    frame_append(ctx->buf, 0, BY_START);
    frame_append(ctx->buf, 1, len);
    frame_append(ctx->buf, 2, BY_SELECT_OPCODE);
    frame_append(ctx->buf, 3, num >> 8);
    frame_append(ctx->buf, 4, num & 0xff);
    frame_append(ctx->buf, 5, frame_chsum(ctx->buf, 1, len - 1));
    frame_append(ctx->buf, 6, BY_END);
    len += 2;
    
    pthread_mutex_lock(&ctx->mutex);
    hal_485_write(ctx->buf, len);
    pthread_mutex_unlock(&ctx->mutex);
    frame_debug(ctx->buf, len);
    return 0;
}

void *by_f300_thread(void *args)
{
    by_f300_context_t *ctx = &byf300_ctx;
    uint32_t len = BY_F300_FRAME_LEN;

    while(0 != ctx->exit) {
        len = BY_F300_FRAME_LEN;
        pthread_mutex_lock(&ctx->mutex);
        hal_485_read(ctx->buf, &len);//ignore the read data.
        pthread_mutex_unlock(&ctx->mutex);
        usleep(1000);
    }
    pthread_exit(NULL);
}

int32_t by_f300_init(void)
{
    by_f300_context_t *ctx = &byf300_ctx;
    int32_t rc = -1;
    
    rc = hal_485_init();
    if(rc) {
        return -1;
    }
    pthread_mutex_init(&ctx->mutex, NULL);
    if(pthread_create(&ctx->tid, NULL, by_f300_thread, NULL) != 0) {
        hal_warn("thread create failed\r\n");
        return -1;
    }
    return 0;
}

int32_t by_f300_exit(void)
{
    by_f300_context_t *ctx = &byf300_ctx;
    
    ctx->exit = 1;
    pthread_join(ctx->tid, NULL);
    pthread_mutex_destroy(&ctx->mutex);
    return 0;
}


