
#include "hal_pwm.h"
#include "pwm.h"
#include "hal_log.h"

#include "stdio.h"
#include "stdlib.h"

#if (CONFIG_BSBOARD == BSM650LV100)
static const uint8_t pwm_gpio[HAL_PWM_MAX] = {29, 33, 37, 41};
#else
static const uint8_t pwm_gpio[HAL_PWM_MAX] = {0, 0, 0, 0};
#endif




static int32_t _hal_pwm_config(uint8_t pwm, uint32_t duty, uint32_t freq)
{
    PWM_PARAM_S param;
    param.level = duty;
    param.freq = freq;
    return SetPwm(pwm_gpio[pwm], &param);
}

int32_t hal_pwm_config(uint8_t pwm, uint32_t duty, uint32_t freq, uint32_t enable)
{
    if(pwm >= HAL_PWM_MAX) {
        hal_warn("unkown pwm name.\r\n");
        return -1;
    }

    if(0 == enable) {
        return _hal_pwm_config(pwm, 0, 1000);
    }
    
    if(duty > 100) {
        hal_warn("pwm duty max: 100\r\n");
        duty = 100;
    }

    if(freq < HAL_PWM_FREQ_MIN || freq > HAL_PWM_FREQ_MAX) {
        hal_warn("invalid pwm freq, use default.\r\n");
        freq = HAL_PWM_FREQ_MIN;
    }

    return _hal_pwm_config(pwm, duty, freq);
}

