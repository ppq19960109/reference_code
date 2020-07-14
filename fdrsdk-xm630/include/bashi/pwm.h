

#ifndef _PWM_H_
#define _PWM_H_

#include "stdint.h"


typedef struct {
	unsigned int level;         /*0~100*/
	unsigned int freq;          /*12000000/65536 ~ 12000000/100*/
}PWM_PARAM_S;


int SetPwm(int gpio, PWM_PARAM_S *pstParam);


#endif

