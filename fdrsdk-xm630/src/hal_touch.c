
#include "sys_comm.h"
#include "hal_touch.h"
#include "hal_log.h"


#define TOUCHSCREEN_READ    0x22
#define TOUCHSCREEN_CLEAR   0x33
#define SCREEN_MAX_X	    1024
#define SCREEN_MAX_Y	    640

typedef struct touch_postion{
	int id;
	int x;
	int y;
}hal_touch_postion_t;

static int32_t g_touch_fd = -1;

void hal_touch_task(void)
{
    int32_t ret = 0;
    hal_touch_postion_t pos;
    if(g_touch_fd < 0) {
        return;
    }

    ret = ioctl(g_touch_fd, TOUCHSCREEN_READ, &pos);
    if(0 == ret) {
        hal_debug("touch pos: (%d, %d)\r\n", pos.x, pos.y);
        ioctl(g_touch_fd, TOUCHSCREEN_CLEAR, &pos);
    }
}

int32_t hal_touch_init(void)
{
    hal_touch_postion_t pos;
    
    g_touch_fd = open(HAL_TOUCH_DEV_NAME, O_RDWR);
    if(g_touch_fd < 0) {
		hal_error("%s open failed\r\n", HAL_TOUCH_DEV_NAME);
		return -1;
	}
    ioctl(g_touch_fd, TOUCHSCREEN_CLEAR, &pos);
    return 0;
}

int32_t hal_touch_deinit(void)
{
    if(g_touch_fd < 0) {
        return -1;
    }

    close(g_touch_fd);
    g_touch_fd = -1;
    return 0;
}

