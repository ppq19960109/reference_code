
#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_

#include "hal_types.h"


void udp_report_log(uint8_t *log, uint32_t len);

int32_t udp_server_init(void);

#endif
