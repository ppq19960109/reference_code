
#ifndef _HAL_KEY_H_
#define _HAL_KEY_H_

#include "stdint.h"
#include "hal_gpio.h"



typedef enum {
    PRESS_UP = 0,
    PRESS_DOWN = 1,
}hal_key_state_t;

typedef int (*hal_key_event_cb)(void *handle, int status);


int32_t hal_key_init(void);

int32_t hal_key_regist_event(hal_key_event_cb func, void *handle);

#endif

