
#ifndef _HAL_HW_CTL_H
#define _HAL_HW_CTL_H

#include "stdint.h"
#include "hal_timer.h"



typedef enum {
    IR_LED_TIMER_E,
    HW_TIMER_MAX
}hw_timer_id_t;



int32_t hal_hw_add_timer(uint8_t id, hal_timer_t *timer, hal_timer_func_t func, void *handle);

int32_t hal_hw_init(void);

#endif

