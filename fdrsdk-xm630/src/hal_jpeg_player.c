
#include "hal_jpeg_player.h"
#include "sys_comm.h"
#include "xm_comm.h"
#include "hal_jpeg.h"
#include "network.h"
#include "hal_log.h"

#define FRAME_MAX_SIZE 512
#define FRAME_MAGIC 0x5a5aa5a5
#define IMAGE_MAX_SIZE 1024*1024
#define UDPSERVER_PORT  30000

typedef struct {
    pthread_t tid;
    int sock;
    int exit;
    network_t udp;
}jpeg_player_handle_t;

typedef struct {
    uint32_t magic; //0x5a5aa5a5
    uint32_t seq;  //frame num
    uint32_t total; //total size
    uint32_t start; //start offset of the jpg image;
    uint32_t end;  //end offset of the jpg image
}jpg_frame_head_t;


static jpeg_player_handle_t  jpeg_player;

void jpeg_player_image(uint32_t idx, uint8_t *image, uint32_t len)
{
    static uint32_t seq = 0;
    uint32_t start = 0;
    uint32_t end = 0;
    jpg_frame_head_t frame;
    net_udp_iface_t *iface = jpeg_player.udp.network_iface;
    uint8_t buf[FRAME_MAX_SIZE + sizeof(jpg_frame_head_t)];
    
    if(NULL == image) {
        return;
    }
    if(len > IMAGE_MAX_SIZE) {
        hal_warn("too big image size = %u, ignore\r\n", len);
        return;
    }

    frame.magic = FRAME_MAGIC;
    frame.seq = seq;
    frame.total = len;
    iface->to.sin_port = htons(UDPSERVER_PORT + idx);
    while(1) {
        end = start + FRAME_MAX_SIZE;
        if(end > len) {
            end = len;
        }
        frame.start = start;
        frame.end = end;
        memcpy(buf, &frame, sizeof(jpg_frame_head_t));
        memcpy(buf + sizeof(jpg_frame_head_t), image + start, end - start);
        NETWORK_API(network_write)(&jpeg_player.udp, iface->fd, buf, sizeof(jpg_frame_head_t) + end - start, 0);
        start = end;
        if(start >= len) {
            //over;
            seq++;
            break;
        }
    }
}

static void *jpeg_player_proc(void *args)
{
    jpeg_player_handle_t *jph = args;  
    net_udp_iface_t iface;
    
    iface.fd = -1;
    iface.lport = 0;
    memset(iface.ip, 0, NET_IP_ADDR_LEN);
    strcpy((char *)iface.ip, "255.255.255.255");
    iface.addr = inet_addr((char *)iface.ip);
    iface.rport = UDPSERVER_PORT;
    iface.buf_len = 1024;
    iface.read_buf = malloc(iface.buf_len);
    iface.read_event = NULL;
    memset(&(iface.from), 0, sizeof(iface.from));
    iface.fromlen = 0;
    memset(&(iface.to), 0, sizeof(iface.to));
    iface.tolen = 0;
    jph->udp.type = NET_UDP_T;
    jph->udp.network_iface = &iface;
    
    NETWORK_API(network_init)(&jph->udp);
    while(0 == jph->exit) {
        NETWORK_API(network_thread)(&jph->udp);
        usleep(50);
    }
    free(iface.read_buf);
    pthread_exit(NULL);
}

int jpeg_player_init(void)
{
    jpeg_player_handle_t *jph = &jpeg_player;
    memset(jph, 0, sizeof(jpeg_player_handle_t));
    if(pthread_create(&jph->tid, NULL, jpeg_player_proc, jph) != 0) {
		hal_error("pthread_create fail\n");
		return -1;
	}

    return 0;
}

int jpeg_player_fini(void)
{
    jpeg_player_handle_t *jph = &jpeg_player;
    jph->exit = 1;
    return 0;
}

