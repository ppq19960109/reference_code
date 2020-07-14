
#include "fdr-version.h"
#include "fdr-dbs.h"
#include "logger.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

#define APP_VERSION_MAX_LEN     36

const static char app_version[] = "firmware-version";
const static char product_id[] = "product-id";

int32_t fdr_productid_init(void)
{
    int32_t rc = -1;
    char *pid = NULL;

    log_info("%s: %s\r\n", product_id, FDR_PRODUCTID);
    rc = fdr_dbs_config_get(product_id, &pid);
    if(rc){
        return fdr_dbs_config_set(product_id, FDR_PRODUCTID);
    }

    if(strcmp(pid, FDR_PRODUCTID)) {
        fdr_dbs_config_set(product_id, FDR_PRODUCTID);
    }
    free(pid);
    return 0;
}

int32_t fdr_version_init(void)
{
    int32_t rc = -1;
    char *val = NULL;
    char ver[APP_VERSION_MAX_LEN] = {0};
    sprintf(ver, FDR_VERSION_PREFIX"%d.%d.%d."FDR_VERSION_TAG, FDR_VERSION_MAJOR, FDR_VERSION_MINOR, FDR_VERSION_PATCH);

    log_info("%s: %s\r\n", app_version, ver);
    rc = fdr_dbs_config_get(app_version, &val);
    if(rc){
        return fdr_dbs_config_set(app_version, ver);
    }

    if(strcmp(ver, val)) {
        fdr_dbs_config_set(app_version, ver);
    }
    free(val);
    return 0;
}


