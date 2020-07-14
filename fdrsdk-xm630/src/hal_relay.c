
#include "sys_comm.h"
#include "xm_comm.h"
#include "hal_gpio.h"
#include "hal_log.h"
#include "hal_relay.h"

static uint8_t g_relay_state = RELAY_OFF;

int32_t hal_relay_off(void)
{
    g_relay_state = RELAY_OFF;
    return hal_set_gpio_output(HAL_RELAY_GPIO_NM, GPIO_LOW);
}

int32_t hal_relay_on(void)
{
    g_relay_state = RELAY_ON;
    return hal_set_gpio_output(HAL_RELAY_GPIO_NM, GPIO_HIGH);
}

uint8_t hal_get_relay_state(void)
{
    return g_relay_state;
}

int32_t hal_relay_reverse(void)
{
    if(RELAY_OFF == hal_get_relay_state()) {
        return hal_relay_on();
    }
    return hal_relay_off();
}

int32_t hal_relay_init(void)
{
    return hal_relay_off();
}

