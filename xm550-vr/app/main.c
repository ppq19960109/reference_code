
#include "hal_vio.h"
#include "hal_image.h"
#include "hal_jpeg.h"
#include "hal_log.h"
#include "hal_sdk.h"
#include "hal_layer.h"

#include "vr.h"
#include "udp_server.h"
#include "httpserver.h"
#include "config.h"
#include "uart.h"

//0 - 100
int camera_get_refrence_level();
int camera_set_refrence_level(int level);

int32_t dhcpc_start(void)
{
    char command[128] = {0};
    char *dns = "114.114.114.114";
    
    snprintf(command, 128 - 1, "echo \"servername %s\" > /var/resolv.conf", dns);
    system(command);
    //system("udhcpc -s /usr/sbin/udhcpc.script &");
    return 0;
}

static int32_t load_camera_attr(camera_attr_t *attr)
{
    if(NULL == attr) {
        return -1;
    }
    attr->fps = atoi((char *)config_get_value((const uint8_t *)"CFG_CAMERA_FPS"));
    attr->shut = atoi((char *)config_get_value((const uint8_t *)"CFG_CAMERA_SHUT"));
    attr->sagain = atoi((char *)config_get_value((const uint8_t *)"CFG_SENSOR_AGAIN"));
    attr->sdgain = atoi((char *)config_get_value((const uint8_t *)"CFG_SENSOR_DGAIN"));
    attr->idgain = atoi((char *)config_get_value((const uint8_t *)"CFG_ISP_DGAIN"));
    return 0;
}

int32_t main(int32_t argc, char *argv[])
{
    httpserver_t httpserver;
    camera_attr_t attr;
    
    config_init();
    
    //dhcpc_start();
    
    httpserver_init(&httpserver);

    load_camera_attr(&attr);
    hal_vio_init(&attr);

    hal_jpeg_init();

    hal_layer_init();

    udp_server_init();
    
    vr_init();

    uart_init();

    while(1) {
        sleep(2);
    }
    return 0;
}


