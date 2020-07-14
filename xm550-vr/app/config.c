
#include "config.h"
#include "config_mgr.h"
#include "hal_sdk.h"
#include "cJSON.h"

#define IPADDR_MAX_LEN      16
#define VALUE_INT_MAX_SIZE  16

static uint8_t ethernet_ipaddr[IPADDR_MAX_LEN];
static uint8_t camera_fps[VALUE_INT_MAX_SIZE] = "25";
static uint8_t camera_shut[VALUE_INT_MAX_SIZE] = "100";
static uint8_t sensor_again[VALUE_INT_MAX_SIZE] = "6400";
static uint8_t sensor_dgain[VALUE_INT_MAX_SIZE] = "6400";
static uint8_t isp_dgain[VALUE_INT_MAX_SIZE] = "1024";
static uint8_t udp_server_port[VALUE_INT_MAX_SIZE] = "10500";
static uint8_t udp_server_radio[VALUE_INT_MAX_SIZE] = "255.255.255.255";


static config_t configs[] = {
    {(const uint8_t *)"CFG_ETHERNET_IPADDR", ethernet_ipaddr, sizeof(ethernet_ipaddr), CONF_STRING, 0},
    {(const uint8_t *)"CFG_CAMERA_FPS", camera_fps, sizeof(camera_fps), CONF_STRING, 0},
    {(const uint8_t *)"CFG_CAMERA_SHUT", camera_shut, sizeof(camera_shut), CONF_STRING, 0},
    {(const uint8_t *)"CFG_SENSOR_AGAIN", sensor_again, sizeof(sensor_again), CONF_STRING, 0},
    {(const uint8_t *)"CFG_SENSOR_DGAIN", sensor_dgain, sizeof(sensor_dgain), CONF_STRING, 0},
    {(const uint8_t *)"CFG_ISP_DGAIN", isp_dgain, sizeof(isp_dgain), CONF_STRING, 0},
    {(const uint8_t *)"CFG_UDP_SERVER_PORT", udp_server_port, sizeof(udp_server_port), CONF_STRING, 0},
    {(const uint8_t *)"CFG_UDP_SERVER_RADIO", udp_server_radio, sizeof(udp_server_radio), CONF_STRING, 0},
};

uint8_t *config_get_value(const uint8_t *name)
{
    uint32_t i;
    if(NULL == name) {
        return NULL;
    }
    for(i = 0; i < HAL_ARRAY_SIZE(configs); i++) {
        if(!strcmp((const char *)name, (const char *)configs[i].name)) {
            return configs[i].value;
        }
    }
    return NULL;
}

int32_t config_json_set_value(cJSON *req)
{
    char *item = NULL;
    uint32_t i = 0;
    
    if(NULL == req) {
        return -1;
    }

    for(i = 0; i < HAL_ARRAY_SIZE(configs); i++) {
        item = cJSON_GetObjectString(req, (const char *)configs[i].name);
        if(item) {
            strncpy((char *)configs[i].value, item, configs[i].size);
            return 0;
        }
    }
    return -1;
}

int32_t config_set_value(const uint8_t *name, uint8_t *value)
{
    uint32_t i;
    if(NULL == name || NULL == value) {
        return -1;
    }

    for(i = 0; i < HAL_ARRAY_SIZE(configs); i++) {
        if(!strcmp((const char *)name, (const char *)configs[i].name)) {
            strncpy((char *)configs[i].value, (char *)value, configs[i].size);
            return 0;
        }
    }

    return -1;
}

int32_t config_commit(void)
{
    return config_mgr_commit();
}


int32_t config_init(void)
{
    config_mgr_regist(configs, HAL_ARRAY_SIZE(configs));
    return config_mgr_init();
}

