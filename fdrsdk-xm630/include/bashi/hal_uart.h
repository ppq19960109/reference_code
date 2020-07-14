
#ifndef _HAL_UART_H_
#define _HAL_UART_H_

#include "stdint.h"



typedef enum {
    UART_B9600 = 9600,
    UART_B115200 = 115200,
}hal_uart_baudrate_t;

typedef enum {
    HAL_UART0,
    HAL_UART1,
    HAL_UART2,
    HAL_UART_MAX
}hal_uart_nm_t;


int32_t uart_read(int32_t fd, uint8_t *buf, uint32_t len);
int32_t uart_write(int32_t fd, uint8_t *buf, uint32_t len);

/*
* eg: uart_open(FR_UART1, 115200, 8, 'N', 1);
*/
int32_t uart_open(uint8_t uartx, uint32_t baudrate, uint8_t nbits, uint8_t parity, uint8_t stop);
int32_t uart_close(int32_t fd);


#endif

