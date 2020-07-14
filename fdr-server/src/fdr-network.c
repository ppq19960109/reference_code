
#include "fdr-network.h"
#include "fdr-dbs.h"
#include "logger.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>


#define NETWORK_COMMAND_MAX_LEN     128
#define IPADDR_LEN  16

char *fdr_get_ipaddr(const char * ifname)
{
    char *temp = NULL;
    int inet_sock = -1;
    struct ifreq ifr;
    static char ip[IPADDR_LEN] = "0.0.0.0";
    
    strcpy(ip, "0.0.0.0");

    if(NULL == ifname) {
        return ip;
    }
    inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(inet_sock < 0) {
        log_error("socket failed.\r\n");
        return ip;
    }
    memset(ifr.ifr_name, 0, sizeof(ifr.ifr_name));
    memcpy(ifr.ifr_name, ifname, strlen(ifname));

    if(0 != ioctl(inet_sock, SIOCGIFADDR, &ifr)) {
        perror("ioctl error");
        close(inet_sock);
        return ip;
    }
    temp = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
    strncpy(ip, temp, IPADDR_LEN);
    
    close(inet_sock);
    return ip;
}

static int32_t fdr_dns_server_config(void)
{
    char *dns = NULL;
    int32_t rc = -1;
    char command[NETWORK_COMMAND_MAX_LEN] = {0};
    
    fdr_dbs_config_get("ether-dns-server", &dns);
    if(NULL == dns) {
        goto end;
    }
    log_info("dns server: %s\r\n", dns);
    snprintf(command, NETWORK_COMMAND_MAX_LEN - 1, "echo \"servername %s\" > /var/resolv.conf", dns);
    rc = 0;
    rc = system(command);
end:
    free(dns);
    return rc;
}

static int32_t fdr_static_ifconfig(void)
{
    char *ip = NULL;
    char *mask = NULL;
    char *gw = NULL;
    int32_t rc = -1;
    char command[NETWORK_COMMAND_MAX_LEN] = {0};
    
    fdr_dbs_config_get("ether-ipv4-address", &ip);
    fdr_dbs_config_get("ether-ipv4-mask", &mask);
    fdr_dbs_config_get("ether-ipv4-gw", &gw);

    if(NULL == ip || NULL == mask || NULL == gw) {
        goto end;
    }
    log_info("network setting....\r\n ip: %s, mask: %s, gw: %s\r\n", ip, mask, gw);
    rc = 0;
    snprintf(command, NETWORK_COMMAND_MAX_LEN - 1, "ifconfig eth0 up %s netmask %s", ip, mask);
    rc += system(command);
    memset(command, 0, NETWORK_COMMAND_MAX_LEN);
    snprintf(command, NETWORK_COMMAND_MAX_LEN - 1, "route add default gw %s", gw);
    rc += system(command);
end:
    free(ip);
    free(mask);
    free(gw);
    return rc;
}

static int32_t fdr_dhcpc_start(void)
{  
    fdr_dns_server_config();
    int rc = system("ls -l; udhcpc -s /usr/sbin/udhcpc.script");
    log_info("dhcpc start rc = %d, errno %s\r\n", rc, strerror(errno));
    return 0;
}

int32_t fdr_network_init(void)
{
    char *val = NULL;
    int32_t rc = -1;
    
    if(fdr_dbs_config_get("ether-ipv4-address-mode", &val)) {
        rc = fdr_static_ifconfig();
    } else if(0 == strcasecmp("dynamic", val)) {
        rc = fdr_dhcpc_start();
    } else {
        rc = fdr_static_ifconfig();
    }

    free(val);
    return rc;
}

