
#include "udp_server.h"
#include "hal_sdk.h"
#include "hal_log.h"
#include "network.h"
#include "vr.h"
#include "config.h"


typedef struct {
    pthread_t tid;
    network_t udp;
}udp_handle_t;

const char *udp_cmd[VR_MAX_E] = {
    "stop",
    "log",
    "show",
    "debug"
};

static udp_handle_t udp_handle;

static void udp_read_callback(uint8_t *buf, uint32_t len)
{
    uint32_t i = 0;
    //net_udp_iface_t *iface = udp_handle.udp.network_iface;
    //memcpy(&iface->to, &iface->from, iface->fromlen);
    //iface->tolen = iface->fromlen;

    for(i = 0; i < VR_MAX_E; i++) {
        if(!strncasecmp(udp_cmd[i], (char *)buf, len)) {
            break;
        }
    }
    vr_set_state(i);
}

void udp_report_log(uint8_t *log, uint32_t len)
{
    net_udp_iface_t *iface = udp_handle.udp.network_iface;
    NETWORK_API(network_write)(&udp_handle.udp, iface->fd, log, len, 0);
}

static void *udp_server_pro(void *parm)
{
    net_udp_iface_t iface;
    uint16_t port = atoi((char *)config_get_value((const uint8_t *)"CFG_UDP_SERVER_PORT"));
    
    iface.fd = -1;
    iface.lport = port + 1;
    memset(iface.ip, 0, NET_IP_ADDR_LEN);
    // strcpy((char *)iface.ip, "255.255.255.255");
    strcpy((char *)iface.ip, (char *)config_get_value((const uint8_t *)"CFG_UDP_SERVER_RADIO"));
    iface.addr = 0;
    iface.rport = port;
    iface.buf_len = 1024;
    iface.read_buf = malloc(iface.buf_len);
    iface.read_event = udp_read_callback;
    memset(&(iface.from), 0, sizeof(iface.from));
    iface.fromlen = 0;
    memset(&(iface.to), 0, sizeof(iface.to));
    iface.tolen = 0;
    udp_handle.udp.type = NET_UDP_T;
    udp_handle.udp.network_iface = &iface;
    
    NETWORK_API(network_init)(&udp_handle.udp);
    while(1) {
        NETWORK_API(network_thread)(&udp_handle.udp);
        usleep(50);
    }
    free(iface.read_buf);
    pthread_exit(NULL);
}

int32_t udp_server_init(void)
{
    if(pthread_create(&udp_handle.tid, NULL, udp_server_pro, NULL) != 0) {
		hal_error("pthread_create fail\n");
		return -1;
	}

    return 0;
}


