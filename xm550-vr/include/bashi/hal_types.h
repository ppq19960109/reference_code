
#ifndef _HAL_TYPES_H_
#define _HAL_TYPES_H_

#include "stdint.h"

typedef signed char  int8_t;
typedef unsigned char  uint8_t;

typedef signed short  int16_t;
typedef unsigned short  uint16_t;


typedef signed int  int32_t;
typedef unsigned int  uint32_t;

typedef long long int  int64_t;
typedef unsigned long long int  uint64_t;


#define HAL_ARRAY_SIZE(array)		(sizeof(array) / sizeof((array)[0]))

#endif

