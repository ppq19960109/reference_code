/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#ifndef __BCP_H__
#define __BCP_H__

#include <inttypes.h>

typedef enum {
    BCP_PMT_INFO_REQ    = 0,            // 查询设备信息
    BCP_PMT_INFO_RSP    = 1,

    BCP_PMT_AUTH_REQ    = 2,            // 接入认证
    BCP_PMT_AUTH_RSP    = 3,

    BCP_PMT_USER_REQ    = 4,            // 帐户操作
    BCP_PMT_USER_RSP    = 5,

    BCP_PMT_CONF_REQ    = 6,            // 配置操作
    BCP_PMT_CONF_RSP    = 7,

    BCP_PMT_LOGGER_REQ  = 8,            // 日志操作
    BCP_PMT_LOGGER_RSP  = 9,

    BCP_PMT_GUEST_REQ   = 10,           // 访客操作
    BCP_PMT_GUEST_RSP   = 11,

    BCP_PMT_EVENT_REQ   = 12,           // 事件上报
    BCP_PMT_EVENT_RSP   = 13,

    BCP_PMT_PUSHDATA_REQ    = 14,       // 推送数据
    BCP_PMT_PUSHDATA_RSP    = 15,

    BCP_PMT_PULLDATA_REQ    = 16,       // 拉取数据
    BCP_PMT_PULLDATA_RSP    = 17,

    BCP_PMT_DATATRANS_REQ   = 18,       // 数据传输
    BCP_PMT_DATATRANS_RSP   = 19,

    BCP_PMT_DEVICE_REQ      = 20,       // 设备操作：重启/重置设备
    BCP_PMT_DEVICE_RSP      = 21,

    BCP_PMT_HEARTBEAT_REQ   = 22,       // 心跳
    BCP_PMT_HEARTBEAT_RSP   = 23,
}bcp_pushmessage_type_e;

#define BCP_PM_CHECKSUM_MAX     32

typedef struct bcp_pm_header{
    uint16_t    type;
    uint16_t    size;
    uint32_t    seqence;
    uint64_t    session;
}__attribute__((packed))bcp_pm_header_t;

typedef struct bcp_pm_transfer_data{
    uint16_t    fileid;   // transfer fileid,internal used
    uint16_t    datalen;
    uint32_t    offset;
    uint8_t     checksum[BCP_PM_CHECKSUM_MAX];    // sha256 checksum of data
}__attribute__((packed))bcp_pm_transfer_data_t;

#define BCP_PM_PAYLOAD_MAX          (64*1024)
#define BCP_PM_SLICE_MAX            (32*1024)

int bcp_pm_transfer_open(const char *fname, uint16_t *fileid);
int bcp_pm_transfer_close(uint16_t fileid);

int bcp_pm_transfer_write(uint16_t fileid, const void *data, uint32_t offset, uint16_t size);
int bcp_pm_transfer_read(uint16_t fileid, void *data, uint32_t offset, uint16_t *size);


#endif // __BCP_H__

