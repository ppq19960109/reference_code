
#ifndef _HAL_SYS_H_
#define _HAL_SYS_H_

#include "hal_types.h"


int reg_write(unsigned long phyaddr, unsigned int val);

int reg_read(unsigned long phyaddr, unsigned int *val);


#endif
