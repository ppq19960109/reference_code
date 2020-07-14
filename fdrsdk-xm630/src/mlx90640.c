

#include "sys_comm.h"
#include "mlx90640.h"
#include "hal_log.h"



#define CMD_I2C_WRITE      0x01
#define CMD_I2C_READ       0x03

typedef struct I2C_DATA{
        unsigned char   dev_addr; 
        unsigned int    reg_addr; 
        unsigned int    addr_byte_num; 
        unsigned int    data; 
        unsigned int    data_byte_num;
}I2C_DATA_S;

static int32_t i2c_fd = -1;

int32_t mlx90640_i2c_read(uint16_t reg, uint16_t *value)
{
    I2C_DATA_S data = {0};
    data.dev_addr = MLX90640_SLAVE_ADDR;
    data.reg_addr = reg;
    data.addr_byte_num = 2;
    data.data = *value;
    data.data_byte_num = 2;
    if(ioctl(i2c_fd, CMD_I2C_READ, &data)) {
        hal_warn("i2c read failed.\r\n");
        return -1;
    }
    *value = data.data;
    return 0;
}

int32_t mlx90640_i2c_write(uint16_t reg, uint16_t value)
{
    I2C_DATA_S data = {0};
    data.dev_addr = MLX90640_SLAVE_ADDR;
    data.reg_addr = reg;
    data.addr_byte_num = 2;
    data.data = value;
    data.data_byte_num = 2;
    if(ioctl(i2c_fd, CMD_I2C_WRITE, &data)) {
        hal_warn("i2c write failed.\r\n");
        return -1;
    }

    return 0;
}

int32_t mlx90640_i2c_init(void)
{
    i2c_fd = open(MLX90640_I2C_DEV_NAME, O_RDWR);
    if(i2c_fd < 0) {
		hal_error("%s open failed\r\n", MLX90640_I2C_DEV_NAME);
		return -1;
	}
    return 0;
}

int32_t mlx90640_i2c_deinit(void)
{
    if(i2c_fd < 0) {
        return -1;
    }

    close(i2c_fd);
    i2c_fd = -1;
    return 0;
}

void mlx90640_i2c_test(void)
{
    uint16_t cr = 0;
    
    if(mlx90640_i2c_init()) {
        return;
    }

    usleep(1000);
    //mlx90640_i2c_write(0x800D, 0x01);
    usleep(1000);
    mlx90640_i2c_read(0x800d, &cr);
    hal_info("read reg 0x800d value: %#x\r\n", cr);
    usleep(1000);

    mlx90640_i2c_deinit();
}

