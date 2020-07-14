/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */


#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <unistd.h>
#ifndef XM6XX
#define SDL_MAIN_HANDLED        /*To fix SDL's "undefined reference to WinMain" issue*/
#include <SDL2/SDL.h>
#include "lv_drivers/display/monitor.h"
#else
#include "lv_drivers/display/xm6xx.h"
#endif 
#include "lvgl/lvgl.h"
#include "lv_drivers/indev/keyboard.h"

/*On OSX SDL needs different handling*/
#if defined(__APPLE__) && defined(TARGET_OS_MAC)
# if __APPLE__ && TARGET_OS_MAC
#define SDL_APPLE
# endif
#endif

static void hal_init(void);
static int tick_thread(void * data);
static void memory_monitor(lv_task_t * param);

const char *freetype_fontsfile = "../test.ttf";

#define CANVAS_WIDTH  480
#define CANVAS_HEIGHT 800

int main(int argc, char ** argv)
{

    (void) argc;    /*Unused*/
    (void) argv;    /*Unused*/


    lv_font_freestyle_init(freetype_fontsfile);

    /*Initialize LittlevGL*/
    lv_init();

    /*Initialize the HAL (display, input devices, tick) for LittlevGL*/
    hal_init();

    static lv_style_t style;
    lv_style_copy(&style, &lv_style_plain);
    style.body.main_color = LV_COLOR_BLUE;
    style.body.border.width = 2;
    style.body.border.color = LV_COLOR_WHITE;
    style.body.shadow.color = LV_COLOR_GRAY;
    style.body.shadow.width = 4;
    style.line.width = 2;
    style.line.color = LV_COLOR_BLACK;
    
    static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(CANVAS_WIDTH, CANVAS_HEIGHT)];

    lv_obj_t * canvas = lv_canvas_create(lv_scr_act(), NULL);
    lv_canvas_set_buffer(canvas, cbuf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(canvas, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_canvas_fill_bg(canvas,  LV_COLOR_RED);
    lv_canvas_draw_rect(canvas, (CANVAS_WIDTH - 100)/2, CANVAS_HEIGHT/2, 100, 70, &style);



    while(1) {
        /* Periodically call the lv_task handler.
         * It could be done in a timer interrupt or an OS task too.*/
        lv_task_handler();
        usleep(5 * 1000);

#ifndef XM6XX
        SDL_Event event;

        while(SDL_PollEvent(&event))
        { 
        }
#else 
        lv_tick_inc(5); /*Tell LittelvGL that 5 milliseconds were elapsed*/
#endif
    }

    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the Littlev graphics library
 */
static void hal_init(void)
{
    /* Use the 'monitor' driver which creates window on PC's monitor to simulate a display*/
#ifndef XM6XX
    monitor_init();
#else 
    xm6xx_init();
#endif

    /*Create a display buffer*/
    static lv_disp_buf_t disp_buf1;
    static lv_color_t buf1_1[800*480];
    lv_disp_buf_init(&disp_buf1, buf1_1, NULL, 800*480);

    /*Create a display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
    disp_drv.buffer = &disp_buf1;

#ifndef XM6XX
    disp_drv.flush_cb = monitor_flush;    /*Used when `LV_VDB_SIZE != 0` in lv_conf.h (buffered drawing)*/
#else 
    disp_drv.flush_cb = xm6xx_flush;    /*Used when `LV_VDB_SIZE != 0` in lv_conf.h (buffered drawing)*/
#endif

    lv_disp_drv_register(&disp_drv);
    
#ifndef XM6XX
    /* Tick init.
     * You have to call 'lv_tick_inc()' in periodically to inform LittelvGL about how much time were elapsed
     * Create an SDL thread to do this*/
    SDL_CreateThread(tick_thread, "tick", NULL);
#endif
    /* Optional:
     * Create a memory monitor task which prints the memory usage in periodically.*/
    lv_task_create(memory_monitor, 3000, LV_TASK_PRIO_MID, NULL);
}

/**
 * A task to measure the elapsed time for LittlevGL
 * @param data unused
 * @return never return
 */
static int tick_thread(void * data)
{
    (void)data;

    while(1) {

#ifndef XM6XX
        SDL_Delay(5);   /*Sleep for 5 millisecond*/
#else 
        usleep(5000);
#endif
        lv_tick_inc(5); /*Tell LittelvGL that 5 milliseconds were elapsed*/
    }

    return 0;
}

/**
 * Print the memory usage periodically
 * @param param
 */
static void memory_monitor(lv_task_t * param)
{
    (void) param; /*Unused*/

    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    printf("used: %6d (%3d %%), frag: %3d %%, biggest free: %6d\n", (int)mon.total_size - mon.free_size,
            mon.used_pct,
            mon.frag_pct,
            (int)mon.free_biggest_size);

}
