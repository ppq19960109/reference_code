

#ifndef _HAL_MLX90640_H_
#define _HAL_MLX90640_H_

#include "stdint.h"

#define MLX90640_I2C_DEV_NAME  "/dev/xm_i2c1"
#define MLX90640_SLAVE_ADDR     0x33

int32_t mlx90640_i2c_init(void);

int32_t mlx90640_i2c_deinit(void);

void mlx90640_i2c_test(void);

#endif

