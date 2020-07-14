
#ifndef _HAL_TOUCH_H_
#define _HAL_TOUCH_H_

#include "stdint.h"

#define HAL_TOUCH_DEV_NAME  "/dev/touch"

int32_t hal_touch_init(void);

int32_t hal_touch_deinit(void);

#endif
