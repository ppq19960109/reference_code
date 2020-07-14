
#include "sys_comm.h"
#include "hal_wifi.h"
#include "hal_gpio.h"
#include "board.h"

int32_t hal_wifi_enable(void)
{
    hal_set_gpio_output(USB_WIFI_ENABLE_PIN, GPIO_HIGH);
    return 0;
}

int32_t hal_wifi_disable(void)
{
    hal_set_gpio_output(USB_WIFI_ENABLE_PIN, GPIO_LOW);
    return 0;
}


