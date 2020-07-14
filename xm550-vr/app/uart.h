
#ifndef _UARTSERVER_H_
#define _UARTSERVER_H_

#include "vr_uart.h"
#include "hal_types.h"
#include "light_interface.h"

typedef enum {
    SET_TRACK=0,
    QUERY_TRACK,
    REPONSE_SET_TRACK=0x80,
    REPONSE_QUERY_TRACK,
    RADIO_COMMAND=0xFF,
}uart_command_t;

typedef enum {
    CLOSE=0,
    ENCRYPTION,
    NO_ENCRYPTION,
}uart_mode_t;

int32_t uart_init(void);
int32_t send_light_position(IA_LIGHT_RSLT_S *rt);

#endif