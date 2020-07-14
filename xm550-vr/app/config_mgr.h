
#ifndef _CONFIG_MGR_H_
#define _CONFIG_MGR_H_

#include "hal_types.h"

#define CONFIG_MAX_NUM              128
#define CONFIG_MAX_NAME_VALUE_SIZE  512
#define CONFIG_EQUAL_CHAR           '='
#define CONFIG_NEWLINE              "\r\n"

typedef struct {
    const uint8_t *name;
    uint8_t *value;
    uint32_t size;
    uint8_t type;
    uint8_t readonly;
}config_t;

typedef enum {
    CONF_STRING,
    CONF_HEX_STR,
    CONF_UINT32,
    CONF_UINT16,
    CONF_UINT8,
    CONF_INVALID,
}config_type_t;


int32_t config_mgr_regist_one(config_t *conf);

int32_t config_mgr_regist(config_t *config, uint32_t size);

int32_t config_mgr_commit(void);

/*
* @brief: config regist before init.
*/
int32_t config_mgr_init(void);

#endif


