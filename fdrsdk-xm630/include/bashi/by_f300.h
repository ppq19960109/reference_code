
#ifndef _BY_F300_H_
#define _BY_F300_H_

#include "stdint.h"

#define BY_START    0x7e
#define BY_END      0xef

enum {
    BY_PLAY_OPCODE = 0x01,
    BY_PAUSE_OPCODE = 0x02,
    BY_STOP_OPCODE = 0x0e,
    BY_SELECT_OPCODE = 0x41,
    BY_QUERY_VOLUME_OPCODE = 0x11,
};
typedef uint32_t by_opcode_t;

int32_t by_f300_play(uint16_t num);

int32_t by_f300_init(void);

int32_t by_f300_exit(void);

#endif
