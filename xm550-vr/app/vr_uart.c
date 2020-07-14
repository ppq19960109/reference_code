
#include "vr_uart.h"
#include "hal_gpio.h"
#include "hal_sys.h"
#include "hal_log.h"

#define fr_error printf
#define fr_debug printf
#define fr_info printf
#define UART_PATH_LEN 36
#define UART_DEV_PATH "/dev/"

static char *g_uart_name[] = {"ttyAMA0", "ttyAMA1", "ttyAMA2"};

int32_t uart_read(int32_t fd, uint8_t *buf, uint32_t len,uint32_t timeout)
{
    fd_set rfds;
    struct timeval tv;
    int32_t retval;
    int32_t rlen;

    if (fd < 0)
    {
        return -1;
    }
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    //donot wait;
    tv.tv_sec = 0;
    tv.tv_usec = timeout;
    retval = select(fd + 1, &rfds, NULL, NULL, &tv);
    if (retval < 0)
    {
        fr_error("Error: select fd read"
                 "%d: %s\r\n",
                 fd, strerror(errno));
        return -1;
    }
    else if (0 == retval)
    {
        return 0;
    }

    rlen = read(fd, buf, len);
    if (rlen < 0)
    {
        fr_error("read err: %d\r\n", rlen);
    }
    return rlen;
}

int32_t uart_write(int32_t fd, uint8_t *buf, uint32_t len)
{
    fd_set wfds;
    struct timeval tv;
    int32_t retval;
    int32_t wlen;

    if (fd < 0)
    {
        return -1;
    }

    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);
    //donot wait;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    retval = select(fd + 1, NULL, &wfds, NULL, &tv);
    if (retval < 0)
    {
        fr_error("Error: select fd write"
                 "%d: %s\r\n",
                 fd, strerror(errno));
        return -1;
    }
    else if (0 == retval)
    {
        fr_error("select fd write not ready, fd = %d\r\n", fd);
        return 0;
    }

    wlen = write(fd, buf, len);
    if (wlen < 0)
    {
        fr_error("write err: %d\r\n", wlen);
    }
    return wlen;
}

static int32_t uart_set_opt(int32_t fd, uint32_t baudrate, uint8_t nbits, uint8_t parity, uint8_t stop)
{
    uint32_t bdrate;
    struct termios attr;

    if (fd < 0)
    {
        return -1;
    }
    if (tcgetattr(fd, &attr) < 0)
    {
        fr_error("failed to get uart attibute\r\n");
        return -1;
    }
    memset(&attr, 0, sizeof(attr));

    attr.c_cflag |= CLOCAL | CREAD; //Enable receiver.
    attr.c_cflag &= ~CSIZE;
    attr.c_cflag &= ~CRTSCTS; //Disable RTS/CTS (hardware) flow control
    //attr.c_lflag = 0;
    attr.c_lflag &= ~ICANON;

    if (7 == nbits)
    {
        attr.c_cflag |= CS7;
    }
    else
    {
        attr.c_cflag |= CS8;
    }

    switch (parity)
    {
    case 'O':
        attr.c_cflag |= PARENB;
        attr.c_cflag |= PARODD;
        attr.c_iflag |= (INPCK | ISTRIP); //Enable input parity checking,
        break;
    case 'E':
        attr.c_cflag |= PARENB;
        attr.c_cflag &= ~PARODD;
        attr.c_iflag |= (INPCK | ISTRIP);
        break;
    default:
        //'N'
        attr.c_cflag &= ~PARENB;
        break;
    }

    if (2 == stop)
    {
        attr.c_cflag |= CSTOPB;
    }
    else
    {
        attr.c_cflag &= ~CSTOPB;
    }

    if (UART_B9600 == baudrate)
    {
        bdrate = B9600;
    }
    else
    {
        bdrate = B115200;
    }
    cfsetispeed(&attr, bdrate);
    cfsetospeed(&attr, bdrate);

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &attr) < 0)
    {
        fr_error("Error: failed to set uart attibute.\r\n");
        return -1;
    }

    return 0;
}

int32_t _uart2_pinmux_tx_init(uint32_t gpio)
{
    uint32_t addr = XM_GPIO_BASE_ADDR + (gpio << 2);
    uint32_t val = 0;
    reg_read(addr, &val);
    fr_debug("val = 0x%x\r\n", val);
    val |= (1 << 3) | (1 << 10) | (1 << 11);
    fr_debug("val = 0x%x\r\n", val);
    return reg_write(addr, val);
}

int32_t _uart2_pinmux_rx_init(uint32_t gpio)
{
    uint32_t addr = XM_GPIO_BASE_ADDR + (gpio << 2);
    uint32_t val = 0;
    reg_read(addr, &val);
    fr_debug("val = 0x%x\r\n", val);
    val |= (1 << 3) | (1 << 12);
    fr_debug("val = 0x%x\r\n", val);
    return reg_write(addr, val);
}

int32_t _uart2_pinmux_init(uint32_t gpio)
{
    uint32_t addr = XM_GPIO_BASE_ADDR + (gpio << 2);
    uint32_t val = 0x88;

    return reg_write(addr, val);
}

static int32_t uart2_pinmux_init(void)
{
    return _uart2_pinmux_init(13) || _uart2_pinmux_init(14);
}

int32_t uart_open(uint8_t uartx, uint32_t baudrate, uint8_t nbits, uint8_t parity, uint8_t stop)
{
    int32_t fd = -1;
    char path[UART_PATH_LEN] = {0};
    uint32_t path_len = 0;

    if (FDR_UART_MAX <= uartx)
    {
        fr_error("unknow uart %d\r\n", uartx);
        return -1;
    }

    if (FDR_UART2 == uartx)
    {
        uart2_pinmux_init();
    }

    path_len = snprintf(path, UART_PATH_LEN - 1, UART_DEV_PATH);
    snprintf(path + path_len, UART_PATH_LEN - path_len - 1, g_uart_name[uartx]);
    fd = open((char *)path, O_RDWR | O_NOCTTY); //O_NONBLOCK
    if (fd < 0)
    {
        fr_error("Error: Could not open uart "
                 "'%s': %s\r\n",
                 path, strerror(errno));
        if (errno == EACCES)
        {
            fr_error("Run as root?\r\n");
        }
        goto end;
    }

    /* 8 bits, no parity, no stop bits */
    if (uart_set_opt(fd, baudrate, nbits, parity, stop) < 0)
    {
        fr_error("Error: set opt failed, "
                 "`%s': %s\r\n",
                 path, strerror(errno));
    }

    fr_info("open %s, fd = %d\r\n", path, fd);
    return fd;

end:
    uart_close(fd);
    return -1;
}

int32_t uart_close(int32_t fd)
{
    if (close(fd) < 0)
    {
        fr_error("close uart dev failed, fd = %d, %s\r\n", fd, strerror(errno));
        return -1;
    }

    return 0;
}
