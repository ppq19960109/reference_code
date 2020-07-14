/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */
#include "fdr-gui.h"
#include "fdr.h"

#include "logger.h"

#include "lvgl/lvgl.h"
#include "lv_drivers/display/monitor.h"
#include "lv_drivers/indev/keyboard.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>


/*On OSX SDL needs different handling*/
#if defined(__APPLE__) && defined(TARGET_OS_MAC)
#if __APPLE__ && TARGET_OS_MAC
#define SDL_MAIN_HANDLED        /*To fix SDL's "undefined reference to WinMain" issue*/
#include <SDL2/SDL.h>

#define SDL_APPLE
#define SDL_DISPLAY         1

#endif
#endif

#define GUI_MAX_USERS           16

#define GUI_WIDTH_MAX           256
#define GUI_HEIGH_MAX           256

#define GUI_DEPTH_MAX           4
#define GUI_USER_IMAGE_BUFFER   (GUI_WIDTH_MAX * GUI_HEIGH_MAX * GUI_DEPTH_MAX)

#define GUI_SCREEN_WIDTH        480
#define GUI_SCREEN_HEIGH        800
#define GUI_SCREEN_DEPTH        2
#define GUI_SCREEN_BUFFER       (GUI_SCREEN_WIDTH * GUI_SCREEN_HEIGH * GUI_SCREEN_DEPTH)


#define GUI_USER_SHOW_LATANCY_MULTI     1000        // ms
#define GUI_USER_SHOW_LATANCY_SINGLE    2000

enum {
    GUI_USER_EXPIRED = -1,
    GUI_USER_UNKOWN = 0,
    GUI_USER_PERMIT,
};


enum {
    GUI_STATE_NONE = 0,
    GUI_STATE_START,
    GUI_STATE_FDR,
    GUI_STATE_UNKOWN,
    GUI_STATE_QRCODE,
    GUI_STATE_ADBOX,
    GUI_STATE_KEYBOARD,
    GUI_STATE_MAX
};

typedef struct win_start{
    lv_obj_t *screen;
    lv_obj_t *image;
}win_start_t;

typedef struct win_fdr{
    lv_obj_t *screen;

    lv_obj_t *container;

    lv_obj_t *background;
    lv_obj_t *state_success;
    lv_obj_t *state_failure;

}win_fdr_t;

typedef struct win_qrcode{
    lv_obj_t *screen;
    lv_obj_t *image;
}win_qrcode_t;

typedef struct win_unkown{
    lv_obj_t *screen;
    lv_obj_t *image;
    lv_obj_t *tips;
    lv_obj_t *arrow;
}win_unkown_t;

typedef struct win_adbox{
    lv_obj_t *screen;
    lv_obj_t *image;
}win_adbox_t;

typedef struct fdr_gui{
    pthread_t     thrd_tick;    // loop for tick
    
    // state for windows selection
    int state;
    int last_state;

    // windows used by gui
    lv_obj_t *wins[GUI_STATE_MAX];

    win_start_t     win_start;
    win_fdr_t       win_fdr;
    win_unkown_t    win_unkown;
    win_qrcode_t    win_qrcode;
    win_adbox_t     win_adbox;

    // lvgl related members
    size_t         bufsize;      // buffer for draw
    lv_color_t     buffer[GUI_SCREEN_BUFFER];

    lv_disp_buf_t  disp_buffer;     // related disp buffer
    lv_disp_drv_t  disp_driver;     // related disp driver

    // users in fdr
    pthread_mutex_t lock;
    int64_t last_fdr_ts;
    struct list_head show_users;
    struct list_head free_users;

    int exitloop;
}fdr_gui_t;

static const char *gui_ftfontsfile  = "./fonts.ttf";

static const char *start_image_file = "./a1050/wallpaper.png";

static const char *fdr_container    = "./a1050/fdr-container.png";
static const char *fdr_success      = "./a1050/fdr-success.png";
static const char *fdr_failure      = "./a1050/fdr-failure.png";
static const char *fdr_user         = "./a1050/fdr-user.png";

static const char *fdr_tips         = "./a1050/fdr-tips.png";

static const char *gui_user_permit_str  = "验证通过";
static const char *gui_user_expired_str = "帐号过期";
static const char *gui_user_unkonw_str  = "尚未授权";

#define GUI_CHECK_NULL(obj_val)             \
        do{                                 \
            if(obj_val == NULL){            \
                log_warn("create %s fail\n", #obj_val);     \
                return NULL;                \
            }                               \
        }while(0)


static fdr_gui_t *gui_get(){
    static fdr_gui_t gui;
    return &gui;
}

static lv_obj_t *win_start_init(fdr_gui_t *gui){
    win_start_t *start = &gui->win_start;

    start->screen = lv_obj_create(NULL, NULL);
    GUI_CHECK_NULL(start->screen);
    
    start->image = lv_img_create(start->screen, NULL);
    GUI_CHECK_NULL(start->image);
    
    lv_img_set_src(start->image, start_image_file);

    return start->screen;
}

static void win_start_fini(fdr_gui_t *gui){
    win_start_t *start = &gui->win_start;

    lv_obj_del(start->screen);
}

static lv_obj_t *win_qrcode_init(fdr_gui_t *gui){
    win_qrcode_t *qrc = &gui->win_qrcode;

    qrc->screen = lv_obj_create(NULL, NULL);
    GUI_CHECK_NULL(qrc->screen);
    
    qrc->image = lv_img_create(qrc->screen, NULL);
    GUI_CHECK_NULL(qrc->image);
    
    lv_img_set_src(qrc->image, start_image_file);

    return qrc->screen;
}

static void win_qrcode_fini(fdr_gui_t *gui){
    win_qrcode_t *qrc = &gui->win_qrcode;

    lv_obj_del(qrc->screen);
}


static lv_obj_t *win_unkown_init(fdr_gui_t *gui){
    win_unkown_t *unkown = &gui->win_unkown;

    unkown->screen = lv_obj_create(NULL, NULL);
    GUI_CHECK_NULL(unkown->screen);
    
    unkown->image = lv_img_create(unkown->screen, NULL);
    GUI_CHECK_NULL(unkown->image);
    
    lv_img_set_src(unkown->image, start_image_file);

    return unkown->screen;
}

static void win_unkown_fini(fdr_gui_t *gui){
    win_unkown_t *unkown = &gui->win_unkown;

    lv_obj_del(unkown->screen);
}


static lv_obj_t *win_adbox_init(fdr_gui_t *gui){
    win_adbox_t *adbox = &gui->win_adbox;

    adbox->screen = lv_obj_create(NULL, NULL);
    GUI_CHECK_NULL(adbox->screen);
    
    adbox->image = lv_img_create(adbox->screen, NULL);
    GUI_CHECK_NULL(adbox->image);
    
    lv_img_set_src(adbox->image, start_image_file);

    return adbox->screen;
}

static void win_adbox_fini(fdr_gui_t *gui){
    win_adbox_t *adbox = &gui->win_adbox;

    lv_obj_del(adbox->screen);
}

static lv_obj_t  *win_fdr_init(fdr_gui_t *gui){
    win_fdr_t *mw = &gui->win_fdr;
    
    static char buffer[480*160*4];

    mw->screen = lv_obj_create(NULL, NULL);
    GUI_CHECK_NULL(mw->screen);
    
    mw->container = lv_canvas_create(mw->screen, NULL);
    GUI_CHECK_NULL(mw->container);
    lv_obj_set_size(mw->container, FDR_GUI_SCREEN_WIDTH, FDR_GUI_MAIN_WIN_HEIGHT);
    lv_obj_set_pos(mw->container, 0, FDR_GUI_SCREEN_HEIGH - FDR_GUI_MAIN_WIN_HEIGHT);

    lv_canvas_set_buffer(mw->container, buffer, 480, 160, LV_IMG_CF_TRUE_COLOR);

    mw->background = lv_img_create(mw->container, NULL);
    GUI_CHECK_NULL(mw->background);
    lv_obj_set_size(mw->background, FDR_GUI_SCREEN_WIDTH, FDR_GUI_MAIN_WIN_HEIGHT);
    lv_obj_set_pos(mw->background, 0, 0);
    lv_img_set_src(mw->background, fdr_container);

#if 0
    mw->state_success = lv_img_create(mw->screen, NULL);
    GUI_CHECK_NULL(mw->state_success);
    lv_img_set_src(mw->state_success, fdr_success);
    lv_obj_set_hidden(mw->state_success, true);

    mw->state_failure = lv_img_create(mw->screen, NULL);
    GUI_CHECK_NULL(mw->state_failure);
    lv_img_set_src(mw->state_failure, fdr_failure);
    lv_obj_set_hidden(mw->state_failure, true);
#endif
    return mw->screen;
}

static void win_fdr_fini(fdr_gui_t *gui){
    win_fdr_t *fdr = &gui->win_fdr;

    lv_obj_del(fdr->screen);
}

static int64_t get_timestamp(){
    struct timeval tv;
    int64_t ts;

    if(gettimeofday(&tv, NULL) == 0){
        ts = tv.tv_sec * 1000;
        ts += tv.tv_usec / 1000;
        return ts;
    }

    return 0;
}

static void gui_show_start(fdr_gui_t *gui){
    if(gui->last_state != gui->state){
        lv_scr_load(gui->win_start.screen);
        gui->last_state = gui->state;
    }
}

static void gui_show_qrcode(fdr_gui_t *gui){
    if(gui->last_state != gui->state){
        lv_scr_load(gui->win_qrcode.screen);
        gui->last_state = gui->state;
    }
}

static void gui_show_unkown(fdr_gui_t *gui){
    if(gui->last_state != gui->state){
        lv_scr_load(gui->win_unkown.screen);
        gui->last_state = gui->state;
    }
}

static void gui_show_adbox(fdr_gui_t *gui){
    if(gui->last_state != gui->state){
        lv_scr_load(gui->win_adbox.screen);
        gui->last_state = gui->state;
    }
}

static void gui_show_user(win_fdr_t *wf, fdr_gui_user_t *user){
    time_t ts;
    struct tm *t;
    char buffer[128];
    lv_obj_t *image;

    static lv_style_t st;

    lv_canvas_fill_bg(wf->container, LV_COLOR_BLUE);

    lv_obj_set_hidden(wf->background, true);

    lv_style_copy(&st, &lv_style_plain);

    lv_canvas_draw_img(wf->container, 0, 0, fdr_container, &st);
    lv_canvas_draw_text(wf->container, 176, 68, 300, &st, user->name, LV_LABEL_ALIGN_LEFT);

#if 0
    // draw background
    //lv_canvas_draw_img(wf->container, 0, 0, wf->background, NULL);

    // draw user image
    sprintf(buffer, "%s.bmp", user->userid);

    image = lv_img_create(wf->container, NULL);
    lv_img_set_src(image, fdr_user);
    //lv_canvas_draw_img(wf->container, 16, 16, fdr_user, NULL);

    lv_font_ft_set(24, 4);
    lv_canvas_draw_text(wf->container, 176, 68, 300, NULL, user->name, LV_LABEL_ALIGN_LEFT);

    lv_font_ft_set(12, 4);
    lv_canvas_draw_text(wf->container, 176, 106, 300, NULL, user->desc, LV_LABEL_ALIGN_LEFT);

    switch(user->state){
    case GUI_USER_PERMIT:
        lv_canvas_draw_img(wf->container, 368, 24, wf->state_success, NULL);

        sprintf(buffer, "%s", gui_user_permit_str);
        lv_font_ft_set(12, 4);
        lv_canvas_draw_text(wf->container, 320, 68, 160, NULL, buffer, LV_LABEL_ALIGN_CENTER);

        break;

    case GUI_USER_EXPIRED:
        lv_canvas_draw_img(wf->container, 368, 24, wf->state_failure, NULL);

        sprintf(buffer, "%s", gui_user_expired_str);
        lv_font_ft_set(12, 4);
        lv_canvas_draw_text(wf->container, 320, 68, 160, NULL, buffer, LV_LABEL_ALIGN_CENTER);
        break;

    default:
        lv_canvas_draw_img(wf->container, 368, 24, wf->state_failure, NULL);

        sprintf(buffer, "%s", gui_user_unkonw_str);
        lv_font_ft_set(12, 4);
        lv_canvas_draw_text(wf->container, 320, 68, 160, NULL, buffer, LV_LABEL_ALIGN_CENTER);
        break;
    }
    
    ts = time(NULL);
    t = localtime(ts);
    sprintf(buffer, "%02d:%02d", t->tm_hour, t->tm_min);
    lv_canvas_draw_text(wf->container, 320, 120, 160, NULL, buffer, LV_LABEL_ALIGN_CENTER);
#endif
}

static void gui_show_fdr(fdr_gui_t *gui){
    fdr_gui_user_t *user = NULL;
    int64_t ts;

    if(gui->last_state != gui->state){
        lv_scr_load(gui->win_fdr.screen);
        gui->last_state = gui->state;
    }

    log_warn("gui_show_fdr => ...\n");

    ts = get_timestamp();
    pthread_mutex_lock(&gui->lock);

    if(list_empty(&gui->show_users)){

        log_warn("gui_show_fdr => no user show...\n");
        #if 0
        if((ts - gui->last_fdr_ts) >= GUI_USER_SHOW_LATANCY_SINGLE){
            gui->last_state = gui->state;
            gui->state = GUI_STATE_NONE;
        }
        #endif
    }else{
        //if((ts - gui->last_fdr_ts) >= GUI_USER_SHOW_LATANCY_MULTI){
            user = list_first_entry(&gui->show_users, fdr_gui_user_t, node);
            if(user->timestamp == 0){
                user->timestamp = time(NULL);
                log_info("gui_show_fdr => %s\n", user->name);

                gui_show_user(&gui->win_fdr, user);

            }else if((user->timestamp + 2) < time(NULL)){
                log_info("gui_show_fdr => release %s\n", user->name);

                list_del(&user->node);

                memset(user, 0, sizeof(fdr_gui_user_t));

                list_add(&user->node, &gui->free_users);
            }
        //}
    }

    pthread_mutex_unlock(&gui->lock);
}

static void gui_update_screen(lv_task_t * param)
{
    fdr_gui_t *gui = (fdr_gui_t*)param->user_data;


    switch(gui->state){
    case GUI_STATE_NONE:
        gui->last_state = -1;
        // FIXME : close GUI 
        break;

    case GUI_STATE_START:
        gui_show_start(gui);
        break;

    case GUI_STATE_FDR:
        gui_show_fdr(gui);
        break;
    
    case GUI_STATE_UNKOWN:
        gui_show_unkown(gui);
        break;
    
    case GUI_STATE_ADBOX:
        gui_show_adbox(gui);
        break;

    case GUI_STATE_QRCODE:
        gui_show_qrcode(gui);
        break;
    
    default:
        log_warn("gui_update_screen => unkown state %d\n", gui->state);
        break;
    }
}

static int gui_lvgl_init(fdr_gui_t *gui)
{
    if(lv_font_freestyle_init(gui_ftfontsfile) != 0){
        log_warn("gui_lvgl_init => load font %s fail\n", gui_ftfontsfile);
        return -1;
    }

    lv_init();

    fdr_gui_decoder_png();
    fdr_gui_decoder_jpg();

    monitor_init();

    lv_disp_buf_init(&gui->disp_buffer, gui->buffer, NULL, GUI_SCREEN_BUFFER);
    lv_disp_drv_init(&gui->disp_driver);
    gui->disp_driver.buffer = &gui->disp_buffer;
    gui->disp_driver.flush_cb = monitor_flush;
    lv_disp_drv_register(&gui->disp_driver);

    return 0;
}

void fdr_gui_loop(){
    fdr_gui_t *gui = gui_get();
    fdr_gui_user_t *pos, *temp;

    while(!gui->exitloop){

        usleep(10);
        lv_task_handler();

#ifdef SDL_DISPLAY
        SDL_Event event;
        while(SDL_PollEvent(&event)){ 
        }
#endif
    }

    pthread_mutex_lock(&gui->lock);

    list_for_each_entry_safe(pos, temp, &gui->show_users, node){
        list_del(&pos->node);
        fdr_free(pos);
    }

    list_for_each_entry_safe(pos, temp, &gui->free_users, node){
        list_del(&pos->node);
        fdr_free(pos);
    }

    pthread_mutex_unlock(&gui->lock);

    pthread_mutex_destroy(&gui->lock);
}

static void *gui_tick_routine(void *args){
    fdr_gui_t *gui = (fdr_gui_t *)args;

    log_info("gui_tick_routine => ..... \n");

    while(!gui->exitloop) {

#ifdef SDL_DISPLAY
        SDL_Delay(5);   /*Sleep for 5 millisecond*/
#else
        usleep(5);
#endif
        lv_tick_inc(5); 
    }

    return NULL;
}

int fdr_gui_init(){
    fdr_gui_t *gui = gui_get();
    int rc;

    memset(gui, 0, sizeof(fdr_gui_t));

    gui->bufsize = GUI_SCREEN_BUFFER;

    pthread_mutex_init(&gui->lock, NULL);

    INIT_LIST_HEAD(&gui->show_users);
    INIT_LIST_HEAD(&gui->free_users);

    gui_lvgl_init(gui);

    gui->wins[GUI_STATE_START]  = win_start_init(gui);
    gui->wins[GUI_STATE_FDR]    = win_fdr_init(gui);
    gui->wins[GUI_STATE_UNKOWN] = win_unkown_init(gui);
    gui->wins[GUI_STATE_QRCODE] = win_qrcode_init(gui);
    gui->wins[GUI_STATE_ADBOX]  = win_adbox_init(gui);

    gui->last_state = -1;
    gui->state = GUI_STATE_FDR;

    lv_task_create(gui_update_screen, 200, LV_TASK_PRIO_MID, gui);

    rc = pthread_create(&gui->thrd_tick, NULL, gui_tick_routine, gui);
    if(rc != 0){
        gui->exitloop = 1;
        log_warn("fdr_gui_init => create tick thread fail %d\n", rc);
        return -1;
    }


    for(int i = 10; i < 20; i ++){
        fdr_gui_user_t *user;

        user = fdr_malloc(sizeof(fdr_gui_user_t));
        if(user != NULL){
            memset(user, 0, sizeof(fdr_gui_user_t));

            strcpy(user->name, "独孤美香");
            strcpy(user->desc, "国际关系与艺术指导部");
            user->state = GUI_USER_PERMIT;
            user->fid = i;

            fdr_gui_dispatch(user->fid, user);

            log_info("gui_tick_routine => add user to show \n");
        }
    }

    return 0;
}

int fdr_gui_fini(){
    fdr_gui_t *gui = gui_get();

    gui->exitloop = 1;

    return 0;
}

int fdr_gui_setmode(int mode){
    return -1;
}

int fdr_gui_acquire(fdr_gui_user_t **user){
    fdr_gui_t *gui = gui_get();

    fdr_gui_user_t *fr = NULL;

    pthread_mutex_lock(&gui->lock);

    if(!list_empty(&gui->free_users)){
        fr = list_first_entry(&gui->free_users, fdr_gui_user_t, node);
        list_del(&fr->node);
    }

    pthread_mutex_unlock(&gui->lock);

    *user = fr;

    return (fr == NULL) ? -1 :  0;
}

int fdr_gui_release(uint32_t fid){
    fdr_gui_t *gui = gui_get();
    fdr_gui_user_t *pos = NULL, *temp = NULL;
    int find = 0;

    pthread_mutex_lock(&gui->lock);

    list_for_each_entry_safe(pos, temp, &gui->show_users, node){

        if(pos->fid == fid){
            list_del(&pos->node);
            find = 1;
            break;
        }
    }
    
    if(find){
        memset(pos, 0, sizeof(fdr_gui_user_t));
        list_add_tail(&pos->node, &gui->free_users);
    }

    pthread_mutex_unlock(&gui->lock);

    return find ? 0 : -1;
}

int fdr_gui_dispatch(uint32_t fid, fdr_gui_user_t *user){
    fdr_gui_t *gui = gui_get();
    fdr_gui_user_t *pos = NULL, *temp = NULL;
    int find = 0;

    pthread_mutex_lock(&gui->lock);

    list_for_each_entry_safe(pos, temp, &gui->show_users, node){

        if(pos->fid == fid){
            find = 1;
            break;
        }
    }
    
    if(!find){
        log_info("fdr_gui_dispatch => add to show user\n");
        list_add_tail(&user->node, &gui->show_users);
    }

    pthread_mutex_unlock(&gui->lock);

    return find ? -1 : 0;
}