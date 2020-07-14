
#ifndef _HAL_SYS_H_
#define _HAL_SYS_H_

#include "stdint.h"

#define	REG_SPACE_SIZE	0x1000

int reg_write(unsigned long phyaddr, unsigned int val);

int reg_read(unsigned long phyaddr, unsigned int *val);


#endif
