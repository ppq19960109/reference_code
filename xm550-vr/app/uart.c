
#include "hal_log.h"
#include "uart.h"
#include "vr.h"
#include "udp_server.h"

#define BUFLEN 128
#define SYNC_HEAD 0xA5
#define TIMEOUT 100
#define INIT_SEND_DELAY 500000

typedef struct
{
    pthread_t tid;
    int32_t fd;
    uint8_t delay_flag;
    uint8_t mode;
    uint8_t read_buf[BUFLEN];
    uint8_t write_buf[BUFLEN];
    uint8_t encode_buf[BUFLEN];
} uart_handle_t;

static uart_handle_t uart_handle;
struct itimerval tick;

int32_t vr_encode(const uint8_t *in, uint8_t *out, uint32_t len, uint32_t shift)
{
    uint32_t i;

    shift %= len;

    for (i = 0; i < shift; i++)
    {
        out[len + i - shift] = in[i];
    }
    for (i = shift; i < len; i++)
    {
        out[i - shift] = in[i];
    }

    return 0;
}

int32_t vr_decode(const uint8_t *in, uint8_t *out, uint32_t len, uint32_t shift)
{
    shift %= len;
    return vr_encode(in, out, len, len - shift);
}

uint8_t sum_check(uint8_t *in, uint8_t in_len)
{
    uint8_t i, sum = 0;
    for (i = 0; i < in_len; ++i)
    {
        sum += in[i];
    }
    return sum;
}

void append_int(int in, uint8_t *out, uint8_t out_len)
{
    uint8_t i;
    for (i = 0; i < out_len; ++i)
    {
        out[i] = in >> ((out_len - 1 - i) * 8);
    }
}

int32_t byte_to_int(uint8_t *in)
{
    return (in[0] << 24) + (in[1] << 16) + (in[2] << 8) + (in[3] << 0);
}

void fill_data(uint8_t *buf, uint8_t pos, IA_LIGHT_DET_S *astDetRslt)
{
    append_int((*astDetRslt).iCenterX, &buf[pos], 2);
    pos += 2;
    append_int((*astDetRslt).iCenterY, &buf[pos], 2);
    pos += 2;
    append_int((*astDetRslt).iWidth, &buf[pos], 2);
    pos += 2;
    append_int((*astDetRslt).iHeight, &buf[pos], 2);
}

int32_t send_light_position(IA_LIGHT_RSLT_S *rt)
{
    uint8_t i, pos;
    uint8_t *buf = uart_handle.write_buf;

    if (uart_handle.mode != ENCRYPTION && uart_handle.mode != NO_ENCRYPTION)
    {
        return -1;
    }
    if (uart_handle.delay_flag == 0)
    {
        return -1;
    }
    memset(buf, 0, BUFLEN);

    // rt->iCount=2;
    // rt->astDetRslt[0].iCenterX=500;
    // rt->astDetRslt[0].iCenterY=600;
    // rt->astDetRslt[0].iWidth=15;
    // rt->astDetRslt[0].iHeight=16;

    // rt->astDetRslt[1].iCenterX=500;
    // rt->astDetRslt[1].iCenterY=600;
    // rt->astDetRslt[1].iWidth=15;
    // rt->astDetRslt[1].iHeight=16;

    buf[0] = SYNC_HEAD;
    buf[1] = RADIO_COMMAND;

    if (uart_handle.mode == ENCRYPTION)
    {
        buf[2] = 0x20;
        if (rt->iCount > 4)
        {
            rt->iCount = 4;
        }
    }
    else
    {
        if (rt->iCount > 8)
        {
            rt->iCount = 8;
        }
        buf[2] = rt->iCount * 8;
    }

    pos = 3;
    for (i = 0; i < rt->iCount; ++i)
    {
        fill_data(buf, pos, &(rt->astDetRslt[i]));
        pos += 8;
    }

    if (uart_handle.mode == ENCRYPTION)
    {
        vr_encode(&buf[3], &uart_handle.encode_buf[3], 32, 15);
        memcpy(&buf[3], &uart_handle.encode_buf[3], 32);
    }

    buf[buf[2] + 4 - 1] = sum_check(&buf[1], buf[2] + 4 - 2);
    uart_write(uart_handle.fd, buf, buf[2] + 4);
    udp_report_log(buf, buf[2] + 4);
    return 0;
}

void timer_task(int signo)
{
    uart_handle.delay_flag = 1;
    memset(&tick, 0, sizeof(tick));
    // hal_info("setitimer\n");
}
uint8_t command_handle(uint8_t *buf, uint8_t len)
{
    switch (buf[1])
    {
    case SET_TRACK:
    {
        buf[1] = REPONSE_SET_TRACK;
        uart_handle.mode = byte_to_int(&buf[3]);
        // hal_info("uart_handle.mode: %d\n", uart_handle.mode);

        if (uart_handle.mode == ENCRYPTION || uart_handle.mode == NO_ENCRYPTION)
        {
            if (uart_handle.delay_flag == 1)
            {
                break;
            }
            signal(SIGALRM, timer_task);
            memset(&tick, 0, sizeof(tick));
            tick.it_value.tv_usec = INIT_SEND_DELAY;
            setitimer(ITIMER_REAL, &tick, NULL);
        }
        else
        {
            uart_handle.delay_flag = 0;
        }
    }
    break;
    case QUERY_TRACK:
    {
        buf[1] = REPONSE_QUERY_TRACK;
        append_int(uart_handle.mode, &buf[3], 4);
    }
    break;
    default:
        return -1;
    }
    buf[len - 1] = sum_check(&buf[1], len - 2);
    return 0;
}

int32_t vr_uart_read(uint8_t *buf, uint32_t len)
{
    int32_t read_count = 0, read_len;
    for (;;)
    {
        read_len = uart_read(uart_handle.fd, &buf[read_count], len - read_count, TIMEOUT);
        read_count += read_len;
        if (read_len == 0 || len <= read_count)
        {
            break;
        }
    }
    return read_count;
}

static void *uart_pro(void *parm)
{
    uint8_t read_len, data_len;
    uint8_t *read_buf = uart_handle.read_buf;

    uart_handle.fd = uart_open(FDR_UART0, UART_B115200, 8, 'N', 1);
    if (uart_handle.fd < 0)
    {
        hal_error("uart_open fail\n");
        return NULL;
    }

    while (1)
    {

        if (vr_uart_read(&read_buf[0], 1) != 1)
        {
            continue;
        }

        if (read_buf[0] != SYNC_HEAD)
        {
            continue;
        }

        if (vr_uart_read(&read_buf[1], 2) != 2)
        {
            hal_error("read command error\n");
            continue;
        }
        data_len = read_buf[2] + 1;

        read_len = vr_uart_read(&read_buf[3], data_len);
        if (read_len != data_len)
        {
            hal_error("read data error\n");
            continue;
        }

        read_len = data_len + 3;
        // hal_info("read_len:%d\n", read_len);
        // hal_info("len:%d read_buf: %x, %x, %x, %x, %x, %x, %x, %x\n", read_len, read_buf[0], read_buf[1], read_buf[2], read_buf[3], read_buf[4], read_buf[5], read_buf[6], read_buf[7]);

        if (read_buf[read_len - 1] != sum_check(&read_buf[1], read_len - 2))
        {
            hal_error("sum check error\n");
            continue;
        }

        if (command_handle(read_buf, read_len) != 0)
        {
            // hal_error("command error\n");
            continue;
        }
        uart_write(uart_handle.fd, read_buf, read_buf[2] + 4);
    }

    uart_close(uart_handle.fd);
    pthread_exit(NULL);
}

int32_t uart_init(void)
{

    if (pthread_create(&uart_handle.tid, NULL, uart_pro, NULL) != 0)
    {
        hal_error("pthread_create fail\n");
        return -1;
    }

    return 0;
}