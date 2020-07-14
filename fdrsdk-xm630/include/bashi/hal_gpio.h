
#ifndef _HAL_GPIO_H_
#define _HAL_GPIO_H_

#include "stdint.h"


#define XM_GPIO_BASE_ADDR 0x10020000

typedef enum {
    GPIO_INPUT,
    GPIO_OUTPUT,
}hal_gpio_dir_t;

typedef enum{
    GPIO_LOW = 0,
    GPIO_HIGH = 1,
    GPIO_UNKNOWN = 255,
}hal_gpio_state_t;

int32_t hal_gpio_config(uint32_t gpio, uint8_t dir);

int32_t hal_set_gpio_output(uint32_t gpio, uint8_t state);

int32_t hal_get_gpio_input(uint32_t gpio);

#endif
