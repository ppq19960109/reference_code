
#include "sys_comm.h"
#include "xm_comm.h"
#include "hal_gpio.h"
#include "hal_log.h"
#include "hal_pir.h"

uint8_t hal_pir_is_valid(void)
{
    if(FDR_PIR_DETECTED_STATE == hal_get_gpio_input(FDR_PIR_DETECTED_PIN)) {
        return 1;
    }

    return 0;
}

int32_t hal_pir_init(void)
{
    return hal_gpio_config(FDR_PIR_DETECTED_PIN, GPIO_INPUT);
}

