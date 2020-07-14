
#include "sys_comm.h"
#include "xm_comm.h"
#include "hal_gpio.h"
#include "hal_log.h"
#include "hal_led.h"
#include "hal_timer.h"
#include "hal_hw_ctl.h"
#include "board.h"

static uint8_t g_led_state[MAX_LED_E];
static uint8_t g_led_on_state[MAX_LED_E] = {GPIO_HIGH, GPIO_HIGH};
static uint32_t g_led_pin[MAX_LED_E] = {WHITE_LED_GPIO_PIN, IR_LED_GPIO_PIN};

int32_t hal_led_off(uint8_t led)
{
    if(led >= MAX_LED_E) {
        return -1;
    }
    g_led_state[led] = LED_OFF;
    return hal_set_gpio_output(g_led_pin[led], GPIO_HIGH == g_led_on_state[led] ? GPIO_LOW : GPIO_HIGH);
}

static int32_t hal_led_timer_cb(void *handle)
{
    hal_led_off(WHITE_LED_E);
    hal_led_off(IR_LED_E);
    return 0;
}

int32_t hal_led_on_duration(uint32_t ms)
{
    static uint8_t led = IR_LED_E;
    hal_timer_t timer;
    
    hal_time_countdown_ms(&timer, ms);
    hal_led_on(WHITE_LED_E);
    hal_led_on(IR_LED_E);
    return hal_hw_add_timer(IR_LED_TIMER_E, &timer, hal_led_timer_cb, &led);
}

int32_t hal_led_on(uint8_t led)
{
    if(led >= MAX_LED_E) {
        return -1;
    }
    g_led_state[led] = LED_ON;
    return hal_set_gpio_output(g_led_pin[led], g_led_on_state[led]);
}

int32_t hal_get_led_state(uint8_t led)
{
    if(led >= MAX_LED_E) {
        return -1;
    }
    return g_led_state[led];
}

int32_t hal_led_reverse(uint8_t led)
{
    if(led >= MAX_LED_E) {
        return -1;
    }
    if(LED_OFF == hal_get_led_state(led)) {
        return hal_led_on(led);
    }
    return hal_led_off(led);
}

int32_t hal_led_init(void)
{
    uint32_t i;
    for(i = 0; i < MAX_LED_E; i++) {
        hal_led_off(i);
    }
    
    return 0;
}

