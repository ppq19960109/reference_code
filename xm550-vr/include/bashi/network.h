/*
 * network.h
 *
 *  Created on: 2018/7/6
 *      Author: heyan
 */

#ifndef INCLUDE_NETWORK_H_
#define INCLUDE_NETWORK_H_

#include "stdint.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/inet.h>


#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
extern "C"
{
#endif

#define NETWORK_API(func_name)   func_name
#define NETWORK_API_ATTR  

#define NETWORK_LOG(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#define NETWORK_MSLEEP(ms)  usleep(ms * 1000)

#define NETWORK_LOG_DEBUG(fmt, ...)  \
    do {\
        NETWORK_LOG("[DEBUG]: %s(), (line: %d), " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    }while(0)
    
#define NETWORK_LOG_INFO(fmt, ...)  \
    do {\
        NETWORK_LOG("[INFO]: %s(), (line: %d), " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    }while(0)
    
#define NETWORK_LOG_ERROR(fmt, ...) \
    do {\
        NETWORK_LOG("[ERROR]: %s(), (line: %d), " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    }while(0)
    
#define NETWORK_LOG_FATAL(fmt, ...) \
    do {\
        NETWORK_LOG("[FATAL]: %s(), (line: %d), " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    }while(0)

#define NET_IP_ADDR_LEN  16
#define NET_TCP_SERVER_CLI_NUM  5
#define NETWORK_WRITE_LEN  1024

typedef enum {
    NET_TCP_CLIENT_T,
    NET_TCP_SERVER_T,
    NET_UDP_T,
}network_type_t;

typedef struct net_tcp_client_iface_tag{
    int32_t fd;
    uint8_t ip[NET_IP_ADDR_LEN];//IPv4 numbers-and-dots
    uint32_t addr;//network byte order
    uint16_t port;//host byte order
    uint8_t reconnect;//reconnect flag
    uint8_t connected;//connect to server flag
    uint32_t reconn_int;//reconnect interval, unit: ms
    uint8_t *read_buf; //buffer to read
    uint32_t buf_len;//buffer len
    
    void (*connect_event)(uint8_t state);//call back func
    void (*read_event)(uint8_t *buf, uint32_t len);//call back func
}net_tcp_client_iface_t;

typedef struct net_tcp_server_iface_tag{
    int32_t fd;
    uint16_t port;
    uint32_t cli_num;
    int32_t cfd[NET_TCP_SERVER_CLI_NUM];
    uint8_t *read_buf; //buffer to read
    uint32_t buf_len;//buffer len
    
    void (*connect_event)(int32_t fd, uint8_t state);//call back func
    void (*read_event)(int32_t fd, uint8_t *buf, uint32_t len);
}net_tcp_server_iface_t;


typedef struct net_udp_iface_tag{
    int32_t fd;
    uint16_t lport;
    
    uint8_t ip[NET_IP_ADDR_LEN];//remote ip
    uint32_t addr;//remote addr
    uint16_t rport;//remote port.
    
    uint8_t *read_buf; //buffer to read
    uint32_t buf_len;//buffer len
    
    struct sockaddr_in from;
    socklen_t fromlen;
    struct sockaddr_in to;
    socklen_t tolen;

    void (*read_event)(uint8_t *buf, uint32_t len);
}net_udp_iface_t;


typedef struct network_tag{
    uint32_t type; //network_type_t
    void *network_iface;
}network_t;


/*
* note:
* These api are not thread-safe.
*/

int32_t NETWORK_API(network_read)(network_t *network, int32_t fd, 
                    uint8_t *buf, uint32_t len, uint32_t tout_ms);

int32_t NETWORK_API(network_write)(network_t *network, int32_t fd,
                    uint8_t *buf, uint32_t len, uint32_t tout_ms);

int32_t NETWORK_API(network_close)(network_t *network, int32_t fd);

int32_t NETWORK_API(network_thread)(network_t *network);

int32_t NETWORK_API(network_init)(network_t *network);

#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
}
#endif

#endif /* INCLUDE_NETWORK_H_ */
