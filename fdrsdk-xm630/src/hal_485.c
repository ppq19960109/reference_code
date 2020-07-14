
#include "sys_comm.h"
#include "hal_uart.h"
#include "hal_log.h"
#include "hal_gpio.h"
#include "hal_485.h"
#include "board.h"


static int32_t g_485_fd = -1;


static void hal_485_tx_enable(void)
{
    hal_set_gpio_output(RS485_RTX_PIN, GPIO_HIGH);
}

static void hal_485_tx_disable(void)
{
    hal_set_gpio_output(RS485_RTX_PIN, GPIO_LOW);
}

int32_t hal_485_write(uint8_t *buf, uint32_t len)
{
    int32_t ret = 0;
    
    hal_485_tx_enable();
    usleep(1000);
    ret = uart_write(g_485_fd, buf, len);
    usleep(1000);
    hal_485_tx_disable();
    
    return ret;
}

int32_t hal_485_read(uint8_t *buf, uint32_t *len)
{
    int32_t bytes = 0;
    
    hal_485_tx_disable();
    usleep(1000);
    bytes = uart_read(g_485_fd, buf, *len);
    if(bytes > 0) {
        *len = bytes;
    }
    
    return bytes;
}

int32_t hal_485_init(void)
{
    g_485_fd = uart_open(HAL_UART2, UART_B9600, 8, 'N', 1);
    if(g_485_fd < 0) {
        hal_error("open FDR_UART2 fail\n");
        return -1;
    }
    
    hal_485_tx_disable();
    return 0;
}


