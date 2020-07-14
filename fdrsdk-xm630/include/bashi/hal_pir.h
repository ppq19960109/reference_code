
#ifndef _HAL_PIR_H_
#define _HAL_PIR_H_

#include "stdint.h"

/*Passive InfraRed*/

#define FDR_PIR_DETECTED_STATE   GPIO_HIGH
#define FDR_PIR_DETECTED_PIN     76

uint8_t hal_pir_is_valid(void);

int32_t hal_pir_init(void);

#endif
