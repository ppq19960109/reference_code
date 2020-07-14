
#include "fdr-discover.h"
#include "fdr-dbs.h"

#include <pthread.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <evdns.h>
#include <assert.h>

#include "logger.h"
#include "cJSON.h"

#define FDR_DISCOVER_UDP_POART  6000
#define FDR_DISCOVER_BUF_LEN    128

struct fdr_discover {
    pthread_t pid;   // thread for main event loop
    int sock;
    int exit;
};

#ifdef TEST_VERSION
#include "hal_image.h"
#include "fdr-path.h"

#define FDR_TEMP_PATH   FDR_HOME_PATH"/tmp"
static int32_t fdr_capture_handle(const char *data, uint32_t len, char **resp)
{
    cJSON *root = cJSON_Parse(data);
    cJSON *resp_json = NULL;
    char *action = NULL;
    char *cn = NULL;
    char pathname[128] = {0};
    int32_t rc = -1;
    
    if(NULL == root) {
        log_warn("invalid data format\r\n");
        return -1;
    }

    action = cJSON_GetObjectString(root, "action");
    if(NULL == action || strcmp("capture", action)) {
        goto end;
    }
    cn = cJSON_GetObjectString(root, "name");
    if(NULL == action) {
        goto end;
    }
    snprintf(pathname, 128 - 1, "%s/%s.jpg", FDR_TEMP_PATH, cn);
    while(hal_image_capture(pathname, 0)){
        usleep(1000);
    }
    snprintf(pathname, 128 - 1, "%s/%s-ir.jpg", FDR_TEMP_PATH, cn);
    while(hal_image_capture(pathname, 1)) {
        usleep(1000);
    }

    resp_json = cJSON_CreateObject();
    if(!cJSON_IsObject(resp_json)) {
        goto end;
    }
    cJSON_AddItemToObject(resp_json, "retcode", cJSON_CreateNumber(0));

    *resp = cJSON_PrintUnformatted(resp_json);
    if(NULL == *resp) {
        goto end;
    }
    rc = 0;
end:
    cJSON_Delete(root);
    cJSON_Delete(resp_json);
    return rc;
}

#endif
static int32_t fdr_discover_handle(const char *data, uint32_t len, char **resp)
{
    cJSON *root = cJSON_Parse(data);
    cJSON *param_json = NULL;
    cJSON *resp_json = NULL;
    char *action = NULL;
    int32_t rc = -1;
    char *val = NULL;
    
    if(NULL == root) {
        log_warn("invalid data format\r\n");
        return -1;
    }

    action = cJSON_GetObjectString(root, "action");
    if(NULL == action || strcmp("scan", action)) {
        goto end;
    }

    resp_json = cJSON_CreateObject();
    if(!cJSON_IsObject(resp_json)) {
        goto end;
    }

    param_json = cJSON_CreateObject();
    if(!cJSON_IsObject(param_json)) {
        goto end;
    }
    cJSON_AddItemToObject(resp_json, "params", param_json);

    if(fdr_dbs_config_get("device-id", &val)) {
        goto end;
    }
    cJSON_AddStringToObject(param_json, "device-id", val);
    free(val);
    *resp = cJSON_PrintUnformatted(resp_json);
    if(NULL == *resp) {
        goto end;
    }
    rc = 0;
end:
    cJSON_Delete(root);
    cJSON_Delete(resp_json);
    return rc;
}

static void *fdr_discover_proc(void *args)
{
    int rc;
    fdr_discover_t *fd = args;
    struct sockaddr_in from;
    char buf[FDR_DISCOVER_BUF_LEN] = {0};
    uint32_t len = sizeof(from);
    char *resp = NULL;
    
    bzero(&from, sizeof(struct sockaddr_in));
    
    while (0 == fd->exit) {
        len = sizeof(from);
        rc = recvfrom(fd->sock, buf, FDR_DISCOVER_BUF_LEN, 0, (struct sockaddr *)&from, (socklen_t *)&len);
        if (rc <= 0) {
            usleep(100 * 1000);
            continue;
        }
        //log_info("recv from 0x%08x:%d\r\n", ntohl(from.sin_addr.s_addr),ntohs(from.sin_port));
        from.sin_port = htons(FDR_DISCOVER_UDP_POART);
        if(!fdr_discover_handle(buf, rc, &resp)) {
            sendto(fd->sock, resp, strlen(resp), 0, (struct sockaddr *)&from, len);
            free(resp);
        }
#ifdef TEST_VERSION
        else if(!fdr_capture_handle(buf, rc, &resp)){
            sendto(fd->sock, resp, strlen(resp), 0, (struct sockaddr *)&from, len);
            free(resp);
        }
#endif
        usleep(50 * 1000);
    }
    
    close(fd->sock);
    fd->sock = -1;
    pthread_exit(NULL);
}

int fdr_discover_init(fdr_discover_t **fd) {
    int sock = -1;
    struct sockaddr_in addr;
    const int opt = 1;
    int rc = -1;

    *fd = (fdr_discover_t *) malloc(sizeof(fdr_discover_t));
    if (*fd == NULL) {
        log_warn("fdr-server alloc memory fail!\n");
        return -1;
    }
    memset(*fd, 0, sizeof(fdr_discover_t));
    
    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(FDR_DISCOVER_UDP_POART);
    
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        log_error("socket error");
        goto fail;
    }

    rc = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *) &opt, sizeof(opt));
    if (rc == -1) {
        log_error("set socket error...");
        close(sock);
        goto fail;
    }

    if (bind(sock, (struct sockaddr *) &(addr), sizeof(struct sockaddr_in)) == -1) {
        log_error("bind error...");
        close(sock);
        goto fail;
    }

    (*fd)->sock = sock;
    rc = pthread_create(&(*fd)->pid, NULL, fdr_discover_proc, (*fd));
    if(rc != 0){
        log_error("create discover thread fail:%d\n", rc);
        close((*fd )->sock);
        goto fail;
    }

    return 0;

fail:
    free(*fd);
    *fd = NULL;
    return -1;
}
