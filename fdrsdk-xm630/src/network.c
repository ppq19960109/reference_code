/*
 * network.c
 *
 *  Created on: 2018/7/6
 *      Author: heyan
 */

#if 1
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"
#endif
#include "network.h"

#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
extern "C"
{
#endif

int32_t NETWORK_API_ATTR
NETWORK_API(network_read)(network_t *network, int32_t fd, 
                    uint8_t *buf, uint32_t len, uint32_t tout_ms)
{
    fd_set rfds;
    struct timeval tv;
    int32_t retval;
    int32_t rlen = 0;
    
    if(!network || !network->network_iface || !buf || !len || fd < 0) {
        NETWORK_LOG_ERROR("network read err\r\n");
        return -1;
    }

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tv.tv_sec = tout_ms / 1000;
    tv.tv_usec = (tout_ms % 1000) * 1000;
    
    retval = select(fd + 1, &rfds, NULL, NULL, &tv);
    if(retval < 0) {
        NETWORK_LOG_ERROR("select error: read fd %d.\r\n", fd);
        return -1;
    } else if(0 == retval) {
        return 0;
    }
    if (!FD_ISSET(fd, &rfds)) {
        return 0;
    }
    
    switch(network->type) {
        case NET_TCP_CLIENT_T: {
            net_tcp_client_iface_t *iface = (net_tcp_client_iface_t *)network->network_iface;
            if(fd != iface->fd) {
                NETWORK_LOG_ERROR("fd check err.\r\n");
                return -1;
            }
            rlen = recv(fd, buf, len, 0);
            if(rlen <= 0) {
                NETWORK_LOG_ERROR("recv err: %d\r\n", rlen);
                NETWORK_API(network_close)(network, fd);
                return -1;
            }
        }
        break;
        
        case NET_TCP_SERVER_T: {
            net_tcp_server_iface_t *iface = (net_tcp_server_iface_t *)network->network_iface;
            uint32_t i;
            for(i = 0; i < iface->cli_num; i++) {
                if(fd == iface->cfd[i]) {
                    break;
                }
            }
            if(i >= iface->cli_num) {
                NETWORK_LOG_ERROR("fd check err.\r\n");
                return -1;
            }
            rlen = recv(fd, buf, len, 0);
            if(rlen <= 0) {
                NETWORK_LOG_ERROR("recv err: %d\r\n", rlen);
                NETWORK_API( network_close)(network, fd);
                return -1;
            }
        }
        break;
        
        case NET_UDP_T: {
            net_udp_iface_t *iface = (net_udp_iface_t *)network->network_iface;
            if(fd != iface->fd) {
                NETWORK_LOG_ERROR("fd check err.\r\n");
                return -1;
            }
            iface->fromlen = sizeof(iface->from);
            rlen = recvfrom(fd, buf, len, 0, (struct sockaddr *)&iface->from, &iface->fromlen);
            if(rlen <= 0) {
                NETWORK_LOG_ERROR("recvfrom err: %d\r\n", rlen);
                return -1;
            }
            // only recv the defined endpoint, iface->addr = 0xffffff, broadcast
            if(iface->rport && iface->addr
                && iface->from.sin_port != htons(iface->rport)
                && ((iface->from.sin_addr.s_addr & iface->addr) == iface->from.sin_addr.s_addr)) {
                rlen = 0;
                return 0;
            }
        }
        break;

        default:{
            NETWORK_LOG_ERROR("unkown network type, type = %d.\r\n", network->type);
            return -1;
        }
        break;
    }

    return rlen;
}

static int32_t NETWORK_API_ATTR
_network_write(network_t *network, int32_t fd, 
                    uint8_t *buf, uint32_t len, uint32_t tout_ms)
{
    fd_set wfds;
    struct timeval tv;
    int32_t retval;
    int32_t wlen;

    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);
    tv.tv_sec = tout_ms / 1000;
    tv.tv_usec = (tout_ms % 1000) * 1000;
    retval = select(fd + 1, NULL, &wfds, NULL, &tv);
    if(retval < 0) {
        NETWORK_LOG_ERROR("select err: write fd %d\r\n", fd);
        return -1;
    } else if(0 == retval) {
        return 0;
    }
    if (!FD_ISSET(fd, &wfds)) {
        return 0;
    }
    
    switch(network->type) {
        case NET_TCP_CLIENT_T: {
            net_tcp_client_iface_t *iface = (net_tcp_client_iface_t *)network->network_iface;
            if(fd != iface->fd) {
                NETWORK_LOG_ERROR("fd check err.\r\n");
                return -1;
            }
            wlen = send(fd, buf, len, 0);
            if(wlen != len) {
                NETWORK_LOG_ERROR("send err: %d\r\n", wlen);
                NETWORK_API(network_close)(network, fd);
                return -1;
            }
        }
        break;
        
        case NET_TCP_SERVER_T: {
            net_tcp_server_iface_t *iface = (net_tcp_server_iface_t *)network->network_iface;
            uint32_t i;
            for(i = 0; i < iface->cli_num; i++) {
                if(fd != iface->cfd[i]) {
                    break;
                }
            }
            if(i >= iface->cli_num) {
                NETWORK_LOG_ERROR("fd check err.\r\n");
                return -1;
            }
            wlen = send(fd, buf, len, 0);
            if(wlen != len) {
                NETWORK_LOG_ERROR("send err: %d\r\n", wlen);
                NETWORK_API(network_close)(network, fd);
                return -1;
            }
        }
        break;
        
        case NET_UDP_T: {
            net_udp_iface_t *iface = (net_udp_iface_t *)network->network_iface;
            if(fd != iface->fd) {
                NETWORK_LOG_ERROR("fd check err.\r\n");
                return -1;
            }
            
            wlen = sendto(fd, buf, len, 0, (struct sockaddr *)&iface->to, iface->tolen);
            if(wlen != len) {
                NETWORK_LOG_ERROR("sendto err: %d\r\n", wlen);
                return -1;
            }
        }
        break;

        default:{
            NETWORK_LOG_ERROR("unkown network type, type = %d.\r\n", network->type);
            return -1;
        }
        break;
    }
    return wlen;
}

int32_t NETWORK_API_ATTR
NETWORK_API(network_write)(network_t *network, int32_t fd, 
                    uint8_t *buf, uint32_t len, uint32_t tout_ms)
{
    uint32_t wlen = 0;
    uint32_t offset = 0;
    uint32_t tlen = len;
    int32_t ret = 0;

    if(!network || !network->network_iface || !buf || !len || fd < 0) {
        NETWORK_LOG_ERROR("network_write err\r\n");
        return -1;
    }
    
    while(tlen) {
        if(tlen > NETWORK_WRITE_LEN) {
            tlen -= NETWORK_WRITE_LEN;
            wlen = NETWORK_WRITE_LEN;
        } else {
            wlen = tlen;
            tlen = 0;
        }
        ret = _network_write(network, fd, buf + offset, wlen, tout_ms);
        if(ret != wlen) {
            break;
        }

        offset += wlen;
    }

    return offset;
}

int32_t NETWORK_API_ATTR
NETWORK_API(network_close)(network_t *network, int32_t fd)
{
    int32_t ret = 0;
    
    if(!network || !network->network_iface || fd < 0) {
        NETWORK_LOG_ERROR("network_close err\r\n");
        return -1;
    }
    
    switch(network->type) {
        case NET_TCP_CLIENT_T: {
            net_tcp_client_iface_t *iface = (net_tcp_client_iface_t *)network->network_iface;
            if(fd != iface->fd) {
                NETWORK_LOG_ERROR("fd check err.\r\n");
                return -1;
            }
            ret = close(fd);
            if(ret < 0) {
                NETWORK_LOG_ERROR("close err: %d\r\n", fd);
                return -1;
            }
            iface->fd = -1;
            iface->connected = 0;
            if(iface->connect_event) {
                iface->connect_event(iface->connected);
            }
        }
        break;
        
        case NET_TCP_SERVER_T: {
            net_tcp_server_iface_t *iface = (net_tcp_server_iface_t *)network->network_iface;
            uint32_t i;
        
            //server
            if(fd == iface->fd) {
                for(i = 0; i < iface->cli_num; i++) {
                    NETWORK_API(network_close)(network, iface->cfd[i]);
                }
                ret = close(fd);
                if(ret < 0) {
                    NETWORK_LOG_ERROR("close err: %d\r\n", fd);
                    return -1;
                }
                iface->fd = -1;
                break;
            }

            //client
            for(i = 0; i < iface->cli_num; i++) {
                if(fd == iface->cfd[i]) {
                    break;
                }
            }
            if(i >= iface->cli_num) {
                NETWORK_LOG_ERROR("fd check err.\r\n");
                return -1;
            }
            ret = close(fd);
            if(ret < 0) {
                NETWORK_LOG_ERROR("close err: %d\r\n", fd);
                return -1;
            }
            iface->cfd[i] = -1;
            if(iface->connect_event){
                iface->connect_event(fd, 0);
            }
        }
        break;
        
        case NET_UDP_T: {
            net_udp_iface_t *iface = (net_udp_iface_t *)network->network_iface;
            if(fd != iface->fd) {
                NETWORK_LOG_ERROR("fd check err.\r\n");
                return -1;
            }
            ret = close(fd);
            if(ret < 0) {
                NETWORK_LOG_ERROR("close err: %d\r\n", fd);
                return -1;
            }
            iface->fd = -1;
        }
        break;

        default:{
            NETWORK_LOG_ERROR("unkown network type, type = %d.\r\n", network->type);
            return -1;
        }
        break;
    }

    return 0;
}

static int32_t net_tcp_client_init(net_tcp_client_iface_t *iface);
static int32_t NETWORK_API_ATTR
net_tcp_client_thread(network_t *network)
{
    net_tcp_client_iface_t *iface = network->network_iface;
    int32_t ret = 0;
    
    if(iface->fd < 0 && iface->reconnect) {
        net_tcp_client_init(iface);
        return -1;
    }

    if(0 == iface->connected) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = iface->addr;
        addr.sin_port = htons(iface->port);
        if(connect(iface->fd, (struct sockaddr *)&addr, sizeof(addr))) {
            NETWORK_LOG_INFO("connect to server 0x%08x failed.\r\n", iface->addr);
            NETWORK_MSLEEP(iface->reconn_int);
            return -1;
        }
        iface->connected = 1;
        if(iface->connect_event) {
            iface->connect_event(iface->connected);
        }
    }

    ret = NETWORK_API(network_read)(network, iface->fd, iface->read_buf, iface->buf_len, 0);
    if(ret > 0){
        if(iface->read_event) {
            iface->read_event(iface->read_buf, ret);
        }
    }
    
    return 0;
}

static int32_t NETWORK_API_ATTR
net_tcp_client_init(net_tcp_client_iface_t *iface)
{
    iface->connected = 0;
    if(0 == iface->buf_len || NULL == iface->read_buf) {
        NETWORK_LOG_ERROR("neet to define the read_buf\r\n");
        return -1;
    }
    
    iface->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(iface->fd < 0) {
        NETWORK_LOG_ERROR("socket failed\r\n");
        return -1;
    }
    if(0 == iface->addr) {
        iface->addr = inet_addr((char *)iface->ip);
    }

    return 0;
}


static int32_t NETWORK_API_ATTR
net_tcp_server_thread(network_t *network)
{
    net_tcp_server_iface_t *iface = network->network_iface;
    fd_set rfds;
    struct timeval tv;
    int32_t retval;
    uint32_t i;
    
    if(iface->fd < 0) {
        return -1;
    }
    
    // accept new client
    FD_ZERO(&rfds);
    FD_SET(iface->fd, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    retval = select(iface->fd + 1, &rfds, NULL, NULL, &tv);
    if(retval < 0) {
        NETWORK_LOG_ERROR("select error: server fd %d.\r\n", iface->fd);
        return -1;
    } else if(0 < retval && FD_ISSET(iface->fd, &rfds)) {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int32_t fd = accept(iface->fd, (struct sockaddr *)&addr, &len);
        if(fd >= 0) {
            for(i = 0; i < iface->cli_num; i++) {
                if(iface->cfd[i] < 0) {
                    break;
                }
            }
            if(i >= iface->cli_num) {
                close(fd);
            } else {
                iface->cfd[i] = fd;
                if(iface->connect_event){
                    iface->connect_event(fd, 1);
                }
            }
        }
    }

    //recv
    for(i = 0; i < iface->cli_num; i++) {
        if(iface->cfd[i] < 0) {
            continue;
        }

        retval = NETWORK_API(network_read)(network, iface->cfd[i], iface->read_buf, iface->buf_len, 0);
        if(retval > 0){
            if(iface->read_event) {
                iface->read_event(iface->cfd[i], iface->read_buf, retval);
            }
        }
    }
    
    return 0;
}

static int32_t NETWORK_API_ATTR
net_tcp_server_init(net_tcp_server_iface_t *iface)
{
    uint32_t i;
    if(0 == iface->buf_len || NULL == iface->read_buf) {
        NETWORK_LOG_ERROR("neet to define the read_buf\r\n");
        return -1;
    }

    if(0 == iface->cli_num || iface->cli_num > NET_TCP_SERVER_CLI_NUM) {
        iface->cli_num = NET_TCP_SERVER_CLI_NUM;
    }

    for(i = 0; i < NET_TCP_SERVER_CLI_NUM; i++) {
        iface->cfd[i] = -1;
    }
    
    iface->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(iface->fd < 0) {
        NETWORK_LOG_ERROR("socket failed\r\n");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(iface->port);
    if(bind(iface->fd, (struct sockaddr *)&addr, sizeof(addr))) {
        NETWORK_LOG_ERROR("bind failed. port = %d\r\n", iface->port);
        return -1;
    }

    if(listen(iface->fd, iface->cli_num)) {
        NETWORK_LOG_ERROR("listen failed, fd = %d.\r\n", iface->fd);
        return -1;
    }
    
    return 0;
}

static int32_t NETWORK_API_ATTR
net_udp_thread(network_t *network)
{
    net_udp_iface_t *iface = network->network_iface;
    int32_t ret = 0;
    
    if(iface->fd < 0) {
        return -1;
    }

    ret = NETWORK_API(network_read)(network, iface->fd, iface->read_buf, iface->buf_len, 0);
    if(ret > 0){
        if(iface->read_event) {
            iface->read_event(iface->read_buf, ret);
        }
    }
    
    return 0;
}

static int32_t NETWORK_API_ATTR
net_udp_init(net_udp_iface_t *iface)
{
    if(0 == iface->buf_len || NULL == iface->read_buf) {
        NETWORK_LOG_ERROR("neet to define the read_buf\r\n");
        return -1;
    }
    
    iface->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(iface->fd < 0) {
        NETWORK_LOG_ERROR("socket failed\r\n");
        return -1;
    }

    if(0 == iface->addr && iface->rport) {
        iface->addr = inet_addr((char *)iface->ip);
        NETWORK_LOG_INFO("udp remote addr: 0x%08x, port: %d.\r\n", iface->addr, iface->rport);
    }
    if(iface->rport && iface->addr) {
        memset(&(iface->to), 0, sizeof(iface->to));
        iface->to.sin_family = AF_INET;
        iface->to.sin_addr.s_addr = iface->addr;
        iface->to.sin_port = htons(iface->rport);
        iface->tolen = sizeof(struct sockaddr_in);
    }

    {
        //enable broadcast;
        uint32_t opt = 1;
        setsockopt(iface->fd, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));
    }
    
    if(iface->lport) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(iface->lport);
        if(bind(iface->fd, (struct sockaddr *)&addr, sizeof(addr))) {
            NETWORK_LOG_ERROR("bind failed. port = %d\r\n", iface->lport);
            return -1;
        }
    }
    
    return 0;
}

int32_t NETWORK_API_ATTR
NETWORK_API(network_thread)(network_t *network)
{
    int32_t ret = 0;
    if(!network || !network->network_iface) {
        NETWORK_LOG_ERROR("network_init err\r\n");
        return -1;
    }
    switch(network->type) {
    case NET_TCP_CLIENT_T:
        ret = net_tcp_client_thread(network);
        break;
    case NET_TCP_SERVER_T:
        ret = net_tcp_server_thread(network);
        break;

    case NET_UDP_T:
        ret = net_udp_thread(network);
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

int32_t NETWORK_API_ATTR
NETWORK_API(network_init)(network_t *network)
{
    int32_t ret = 0;
    
    if(!network || !network->network_iface) {
        NETWORK_LOG_ERROR("network_init err\r\n");
        return -1;
    }
    switch(network->type) {
    case NET_TCP_CLIENT_T:
        ret = net_tcp_client_init(network->network_iface);
        break;
    case NET_TCP_SERVER_T:
        ret = net_tcp_server_init(network->network_iface);
        break;

    case NET_UDP_T:
        ret = net_udp_init(network->network_iface);
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
}
#endif
