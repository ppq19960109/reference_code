
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "hal_types.h"
#include "cJSON.h"

uint8_t *config_get_value(const uint8_t *name);

int32_t config_json_set_value(cJSON *req);

int32_t config_set_value(const uint8_t *name, uint8_t *value);

int32_t config_commit(void);

int32_t config_init(void);

#endif
