
#ifndef _HAL_RELAY_H_
#define _HAL_RELAY_H_

#include "stdint.h"

#define HAL_RELAY_GPIO_NM    69

typedef enum {
    RELAY_OFF,
    RELAY_ON
}hal_relay_state_t;

int32_t hal_relay_on(void);

int32_t hal_relay_off(void);

uint8_t hal_get_relay_state(void);

int32_t hal_relay_reverse(void);

int32_t hal_relay_init(void);

#endif

