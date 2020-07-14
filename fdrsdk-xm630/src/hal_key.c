
#include "sys_comm.h"
#include "hal_gpio.h"
#include "hal_key.h"
#include "hal_log.h"
#include "board.h"


typedef struct {
    hal_key_event_cb cb;
    void *handle;
}hal_key_ctx_t;

static hal_key_ctx_t key_ctx;


static uint8_t hal_key_state(void)
{
    if(KEY_PRESS_UP_STATE == hal_get_gpio_input(HAL_KEY_GPIO_NM)) {
        return PRESS_UP;
    }
    return PRESS_DOWN;
}


void hal_key_task(void)
{
    static uint8_t lst = PRESS_UP;
    uint8_t cst;

    cst = hal_key_state(); 
    if(lst == cst) {
        return;
    }

    lst = cst;
    if(key_ctx.cb) {
        key_ctx.cb(key_ctx.handle, cst);
    }
    hal_info("key state: %d\r\n", lst);
}

int32_t hal_key_init(void)
{
    hal_info("key gpio:%d\r\n", HAL_KEY_GPIO_NM);
    return hal_gpio_config(HAL_KEY_GPIO_NM, GPIO_INPUT);
}

int32_t hal_key_regist_event(hal_key_event_cb func, void *handle)
{
    key_ctx.cb = func;
    key_ctx.handle = handle;

    return 0;
}


