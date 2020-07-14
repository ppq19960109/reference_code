
#ifndef _HAL_485_H_
#define _HAL_485_H_

#include "stdint.h"


int32_t hal_485_write(uint8_t *buf, uint32_t len);

int32_t hal_485_read(uint8_t *buf, uint32_t *len);

int32_t hal_485_init(void);


#endif

