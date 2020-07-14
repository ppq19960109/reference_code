#include "sys_comm.h"
#include "xm_comm.h"
#include "hal_hw_ctl.h"
#include "hal_gpio.h"
#include "hal_key.h"
#include "hal_led.h"
#include "hal_log.h"
#include "hal_pir.h"
#include "hal_audio.h"
#include "hal_relay.h"
#include "hal_timer.h"
#include "hal_touch.h"



typedef struct {
    hal_timer_t timer;
    hal_timer_func_t func;
    void *handle;
    uint8_t run;
}hal_timer_hd_t;

typedef struct {
    pthread_t tid;
    pthread_mutex_t mutex;
    hal_timer_hd_t timer[HW_TIMER_MAX];
}hal_hw_t;

static hal_hw_t hw_handle;

void hal_key_task(void);
void hal_touch_task(void);


int32_t hal_hw_add_timer(uint8_t id, hal_timer_t *timer, hal_timer_func_t func, void *handle)
{
    if(id >= HW_TIMER_MAX || NULL == timer || NULL == func) {
        return -1;
    }

    pthread_mutex_lock(&hw_handle.mutex);
    hw_handle.timer[id].run = 1;
    hw_handle.timer[id].timer.time = timer->time;
    hw_handle.timer[id].func = func;
    hw_handle.timer[id].handle = handle;
    pthread_mutex_unlock(&hw_handle.mutex);

    return 0;
}

static void hal_hw_timer_task(void)
{
    uint32_t i;

    pthread_mutex_lock(&hw_handle.mutex);
    for(i = 0; i < HW_TIMER_MAX; i++) {
        if(!hw_handle.timer[i].run) {
            continue;
        }
        if(hal_time_is_expired(&hw_handle.timer[i].timer)) {
            hw_handle.timer[i].func(hw_handle.timer[i].handle);
            hw_handle.timer[i].run = 0;
        }
    }
    pthread_mutex_unlock(&hw_handle.mutex);
}

#ifdef TEMP_TEST
static void hal_led_task(void)
{
    static hal_timer_t tm = {0};
    static uint8_t on = 0;
    
    if(hal_time_is_expired(&tm)) {
        if(on) {
            hal_led_on(WHITE_LED_E);
            hal_led_on(IR_LED_E);
            hal_time_countdown_ms(&tm, 5000);
            on = 0;
        } else {
            hal_led_off(WHITE_LED_E);
            hal_led_off(IR_LED_E);
            hal_time_countdown_ms(&tm, 5000);
            on = 1;
        }
    }
}
#endif

static void *hal_hw_handle(void *arg)
{
    //static uint32_t count = 0;
    prctl(PR_SET_NAME,(unsigned long)"hw_ctl_handle");
    /*wait for all init*/
    sleep(4);
    
    while(1) {
        hal_key_task();
        hal_touch_task();
        hal_hw_timer_task();
#ifdef TEMP_TEST
        hal_led_task();
#endif
        usleep(10 * 1000);
    }

    pthread_exit(NULL);
}

int32_t hal_hw_init(void)
{
    hal_key_init();
    hal_pir_init();
    hal_relay_init();
    hal_led_init();
    hal_touch_init();
    
    pthread_mutex_init(&hw_handle.mutex, NULL);
    if(pthread_create(&hw_handle.tid, NULL, hal_hw_handle, NULL) != 0) {
        hal_error("pthread_create fail\n");
        return -1;
    }

    pthread_detach(hw_handle.tid);
    return 0;
}

