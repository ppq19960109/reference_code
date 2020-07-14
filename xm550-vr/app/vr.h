
#ifndef _VR_H_
#define _VR_H_

#include "hal_types.h"
#include "light_interface.h"

typedef enum {
    VR_IDLE_E,
    VR_LOG_E,
    VR_SHOW_E,
    VR_DEBUG_E,
    VR_MAX_E,
}vr_state_t;

int32_t vr_set_state(uint32_t state);

int32_t vr_snapshot(const char *path, char *filename);

int32_t vr_init(void);

#endif

