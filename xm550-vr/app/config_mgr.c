
#define _GNU_SOURCE

#include "config_mgr.h"
#include "hal_sdk.h"
#include "hal_log.h"

#define CONFIG_FILE_NAME "config.conf"

typedef struct {
    config_t config[CONFIG_MAX_NUM];
    pthread_mutex_t mutex;
}config_handle_t;


static config_handle_t config_handle;

int32_t config_mgr_regist_one(config_t *conf)
{
    uint32_t i;
    config_t *c = config_handle.config;
    
    if(!conf || !conf->name || !conf->value
        || !conf->size || conf->type >= CONF_INVALID) {
        hal_error("Invalid config param\r\n");
        return -1;
    }
    
    for(i = 0; i < CONFIG_MAX_NUM; i++) {
        if(c[i].name) {
            continue;
        }
        c[i].name = conf->name;
        c[i].value = conf->value;
        c[i].size = conf->size;
        c[i].type = conf->type;
        hal_debug("regist conf %s\r\n", conf->name);
        return 0;
    }

    hal_error("config num > %d.\r\n", CONFIG_MAX_NUM);
    return -1;
}

int32_t config_mgr_regist(config_t *config, uint32_t size)
{
    uint32_t i;
    for(i = 0; i < size; i++) {
        if(config_mgr_regist_one(&config[i])) {
            return -1;
        }
    }

    return 0;
}

static int32_t config_mgr_write(void)
{
    uint32_t i;
    config_t *c = config_handle.config;
    FILE *f = NULL;
    uint8_t eq = CONFIG_EQUAL_CHAR;

    f = fopen(CONFIG_FILE_NAME, "w");
    if(NULL == f) {
        hal_error("fopen %s for write failed.\r\n", CONFIG_FILE_NAME);
        return -1;
    }
    for(i = 0; i < CONFIG_MAX_NUM; i++) {
        if(NULL == c[i].name || NULL == c[i].value) {
            continue;
        }
        hal_printf("<<< %s=%s\r\n", c[i].name, c[i].value);
        fwrite(c[i].name, 1, strlen((const char *)c[i].name), f);
        fwrite(&eq, 1, sizeof(eq), f);
        fwrite(c[i].value, 1, strlen((char *)c[i].value), f);
        fwrite(CONFIG_NEWLINE, 1, strlen(CONFIG_NEWLINE), f);
    }
    fflush(f);
    fclose(f);
    return 0;
}

int32_t config_mgr_commit(void)
{
    int32_t rc = -1;
    
    pthread_mutex_lock(&config_handle.mutex);
    rc = config_mgr_write();
    pthread_mutex_unlock(&config_handle.mutex);

    return rc;
}

static int32_t config_mgr_parse_line(uint8_t *line, uint32_t sz)
{
    uint8_t *name = NULL;
    uint8_t *value = NULL;
    uint32_t i = 0;
    config_t *c = config_handle.config;

    if(NULL == line) {
        return -1;
    }
    name = line;
    if(sz < 4) {
        //name, value, '=', '\n'
        return -1;
    }
    if('\n' == line[sz - 1]) {
        line[sz - 1] = 0;
        sz--;
        if('\r' == line[sz - 1]) {
            line[sz - 1] = 0;
            sz--;
        }
    }
    
    value = (uint8_t *)strchr((char *)line, CONFIG_EQUAL_CHAR);
    if(NULL == value) {
        return -1;
    }
    value[0] = 0;
    value++;
    hal_printf(">>> %s=%s\r\n", name, value);
    for(i = 0; i < CONFIG_MAX_NUM; i++) {
        if(c[i].name && !strcmp((const char *)c[i].name, (char *)name)) {
            strncpy((char *)c[i].value, (char *)value, c[i].size);
            return 0;
        }
    }
    return -1;
}

static int32_t config_mgr_read(void)
{
    int32_t rc = -1;
    uint8_t *line = NULL;
    uint32_t sz;
    FILE *f = NULL;
    

    f = fopen(CONFIG_FILE_NAME, "r");
    if(NULL == f) {
        hal_warn("no saved config, set to default.\r\n");
        return 0;
    }

    while(1) {
        rc = (int32_t)getline((char **)&line, &sz, f);
        if(rc < 0) {
            break;
        }
        if( 0 == rc) {
            continue;
        }
        config_mgr_parse_line(line, rc);
    }
    rc = 0;
    if(line) {
        free(line);
    }
    fclose(f);
    return rc;
}

int32_t config_mgr_init(void)
{
    pthread_mutex_init(&config_handle.mutex, NULL);
    return config_mgr_read();
}

