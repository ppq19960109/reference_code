

#ifndef _HAL_TIMER_H_
#define _HAL_TIMER_H_

#include "stdint.h"

/* unit: ms*/
typedef struct {
    uint32_t time;
}hal_time_t;

typedef int32_t (*hal_timer_func_t)(void *handle);

uint32_t hal_time_get_ms(void);

void hal_time_init(hal_time_t *timer);

void hal_time_start(hal_time_t *timer);

uint32_t hal_time_spend(hal_time_t *start);

uint32_t hal_time_left(hal_time_t *end);

uint32_t hal_time_is_expired(hal_time_t *timer);

void hal_time_countdown_ms(hal_time_t *timer, uint32_t millisecond);

void hal_time_expire(hal_time_t *timer);

#endif


