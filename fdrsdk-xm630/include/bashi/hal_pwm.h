
#ifndef _HAL_PWM_H_
#define _HAL_PWM_H_

#include "stdint.h"

typedef enum{
    HAL_PWM_WHITE,
    HAL_PWM_RED,
    HAL_PWM_GREEN,
    HAL_PWM_BLUE,
    HAL_PWM_MAX,
}hal_pwm_name_t;

#define HAL_PWM_FREQ_MIN    1000
#define HAL_PWM_FREQ_MAX    10000

/*
* @brief: PWM enable or disable.
* @parm[in] pwm: pwm name, white, red, green, blue.
* @parm[in] freq: unit: HZ, 1K ~ 10K
* @parm[in] duty: the duty of the pwm. 0 - 100
* @parm[in] enable: 0 - disable, 1 - enable.
*/
int32_t hal_pwm_config(uint8_t pwm, uint32_t duty, uint32_t freq, uint32_t enable);


#endif
