
#ifndef _FR_UART_H_
#define _FR_UART_H_

// #define __USE_MISC
#include <stdio.h>  
#include <stdlib.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <termios.h> 
#include <errno.h>   
#include <string.h>
#include <sys/select.h>
 #include <pthread.h>
 #include <sys/time.h> 
 #include <signal.h>  
#include "stdint.h"



typedef enum {
    UART_B9600 = 9600,
    UART_B115200 = 115200,
}fr_uart_baudrate_t;

typedef enum {
    FDR_UART0,
    FDR_UART1,
    FDR_UART2,
    FDR_UART_MAX
}fdr_uart_nm_t;


int32_t uart_read(int32_t fd, uint8_t *buf, uint32_t len,uint32_t timeout);
int32_t uart_write(int32_t fd, uint8_t *buf, uint32_t len);

/*
* eg: uart_open(FR_UART1, 115200, 8, 'N', 1);
*/
int32_t uart_open(uint8_t uartx, uint32_t baudrate, uint8_t nbits, uint8_t parity, uint8_t stop);
int32_t uart_close(int32_t fd);


#endif

