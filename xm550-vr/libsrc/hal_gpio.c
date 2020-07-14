

#include "hal_sdk.h"
#include "hal_gpio.h"
#include "hal_sys.h"


int32_t hal_gpio_config(uint32_t gpio, uint8_t dir)
{
    uint32_t addr = XM_GPIO_BASE_ADDR + (gpio << 2);
    uint32_t val = 0;
    if(GPIO_INPUT == dir) {
        val = 1 << 12;
    } else {
        val = 1 << 10;
    }

    return reg_write(addr, val);
}

int32_t hal_set_gpio_output(uint32_t gpio, uint8_t state)
{
    uint32_t addr = XM_GPIO_BASE_ADDR + (gpio << 2);
    uint32_t val = 0;

    val = 1 << 10;
    if(GPIO_HIGH == state) {
        val |= (1 << 11);
    }
    return reg_write(addr, val);
}

int32_t hal_get_gpio_input(uint32_t gpio)
{
    uint32_t addr = XM_GPIO_BASE_ADDR + (gpio << 2);
    uint32_t val = 0;
    int32_t ret = 0;
    
    ret = reg_read(addr, &val);
    if(0 != ret) {
        return -1;
    }

    if(val & (1 << 13)) {
        return GPIO_HIGH;
    }
    return GPIO_LOW;
}

