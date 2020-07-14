

#ifndef _HAL_LED_H_
#define _HAL_LED_H_

#include "stdint.h"



typedef enum {
    LED_ON,
    LED_OFF,
}hal_led_state_t;

typedef enum {
    WHITE_LED_E,
    IR_LED_E,
    MAX_LED_E,
}hal_led_t;

int32_t hal_led_off(uint8_t led);

int32_t hal_led_on_duration(uint32_t ms);

int32_t hal_led_on(uint8_t led);

int32_t hal_get_led_state(uint8_t led);

int32_t hal_led_reverse(uint8_t led);

int32_t hal_led_init(void);

#endif


