/*
 * Copyright (c) 2020, Bashi Tech. All rights reserved.
 */

#include "lvgui.h"
#include "lvgui-yuv.h"

#include "logger.h"

#include "lvgl/lvgl.h"

#ifdef XM6XX
#include "lv_drivers/display/xm6xx.h"
#else
#include "lv_drivers/display/monitor.h"
#include <SDL2/SDL.h>
#endif

#include "fdrcp-utils.h"

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event_struct.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

/* On OSX SDL needs different handling */
#if defined(__APPLE__) && defined(TARGET_OS_MAC)
    #if __APPLE__ && TARGET_OS_MAC
        #define SDL_MAIN_HANDLED        /* To fix SDL's "undefined reference to WinMain" issue */
        #include <SDL2/SDL.h>

        #define SDL_APPLE
        #define SDL_DISPLAY             1
    #endif
#endif

#define LVGUI_SCREEN_WIDTH              480
#define LVGUI_SCREEN_HEIGHT             800
#define LVGUI_SCREEN_BUFFER             LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(LVGUI_SCREEN_WIDTH,LVGUI_SCREEN_HEIGHT)

#define LVGUI_NETSTATE_WIDTH            LVGUI_SCREEN_WIDTH
#define LVGUI_NETSTATE_HEIGHT           32

#define LVGUI_COLOR_PASS                LV_COLOR_MAKE(77,208,93)
#define LVGUI_COLOR_DENY                LV_COLOR_MAKE(210,56,52)
#define LVGUI_COLOR_EXPIRED             LV_COLOR_MAKE(41,111,255)
#define LVGUI_COLOR_UNKOWN              LV_COLOR_GRAY

#define LVGUI_COLOR_THERM_PERM            LV_COLOR_MAKE(77,208,93)
#define LVGUI_COLOR_THERM_DENY            LV_COLOR_MAKE(255,0,0)

#define LVGUI_COLOR_USER_BK             LV_COLOR_MAKE(41,111,255)

/*
 * 5 inch defines
 */

#define LVGUI_THERM_HEIGHT              32
#define LVGUI_UBAR_HEIGHT               160

#define LVGUI_UBAR_BUFFER               LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(LVGUI_SCREEN_WIDTH,LVGUI_UBAR_HEIGHT)

#define LVGUI_UIMG_AREA_W               160     // 1/3 of screen width
#define LVGUI_UIMG_AREA_H               160     // same as width

#define LVGUI_UIMG_X                    16      // = (160 - 128)/2
#define LVGUI_UIMG_Y                    16
#define LVGUI_UIMG_W                    128
#define LVGUI_UIMG_H                    128
#define LVGUI_UIMG_BUFFER               LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(LVGUI_UIMG_W,LVGUI_UIMG_H)


#define LVGUI_UIMG_BOX_W                4

#define LVGUI_USER_NAME_X               160
#define LVGUI_USER_NAME_Y               (LVGUI_SCREEN_HEIGHT - LVGUI_UBAR_HEIGHT/2 - 32)
#define LVGUI_USER_NAME_W               320
#define LVGUI_USER_NAME_H               40

#define LVGUI_USER_DESC_X               160
#define LVGUI_USER_DESC_Y               (LVGUI_SCREEN_HEIGHT - LVGUI_UBAR_HEIGHT/2 + 24)
#define LVGUI_USER_DESC_W               320
#define LVGUI_USER_DESC_H               30

#define LVGUI_TIPS_IMG_X                ((LVGUI_SCREEN_WIDTH - 34) / 2)
#define LVGUI_TIPS_IMG_Y                (LVGUI_UBAR_HEIGHT - 48)

#define LVGUI_EVMSG_USER                1
#define LVGUI_EVMSG_MODE                2
#define LVGUI_EVMSG_THERM               3


typedef struct lvgui{
    pthread_t   thrd;    // loop tick
    int         init;
    // state for windows selection
    int state;

    // lvgl related members
    size_t              bufsize;      // buffer for draw
    lv_color_t          buffer[LVGUI_SCREEN_BUFFER];

    lv_disp_buf_t       disp_buffer;     // related disp buffer
    lv_disp_drv_t       disp_driver;     // related disp driver

    //{{
    lv_style_t  stcanvas;
    lv_obj_t    *obj_canvas;    // canvas for user info
    char        canvas_buffer[LVGUI_SCREEN_BUFFER];

    lv_style_t  sttherm;
    lv_obj_t    *obj_therm;     // label  for therm

    lv_style_t  sttips;
    lv_obj_t    *obj_tips;     // label  for background & tips

    lv_style_t  stbar;
    lv_obj_t    *obj_bar;    // canvas for user info
    char        bar_buffer[LVGUI_UBAR_BUFFER];

    lv_style_t  stuname;
    lv_obj_t    *obj_uname;     // label  for user name

    lv_style_t  studesc;
    lv_obj_t    *obj_udesc;     // label  for user desc

    lv_img_dsc_t  uimg;       // buffer of user image
    uint8_t uimg_buffer[LVGUI_UIMG_BUFFER];
    //}}

    // users in fdr
    pthread_mutex_t     lock;

    struct list_head    show_users;
    struct list_head    free_users;

    struct event ev_timer;

    struct event_base* base;
    struct bufferevent *bepair[2];

    int exitloop;
}lvgui_t;

typedef struct {
    int     type;
    union{
        void    *data;
        int     mode;
        float   therm;
    };
    int seconds;
}lvgui_evmsg_t;


typedef void (*lvgui_paint_proc)(lvgui_t *gui);
typedef struct {
    int state;
    lvgui_paint_proc  painter;
}lvgui_state_paint_t;

static void lvgui_draw_none(lvgui_t *gui);
static void lvgui_draw_init(lvgui_t *gui);
static void lvgui_draw_reco(lvgui_t *gui);
static void lvgui_draw_info(lvgui_t *gui);
static void lvgui_draw_visi(lvgui_t *gui);
static void lvgui_draw_tips(lvgui_t *gui);
static void lvgui_draw_advt(lvgui_t *gui);
static void lvgui_draw_test(lvgui_t *gui);

static lvgui_state_paint_t lvgui_paints[] = {
    { LVGUI_MODE_NONE,         lvgui_draw_none  },
    { LVGUI_MODE_INIT,         lvgui_draw_init  },
    { LVGUI_MODE_RECO,         lvgui_draw_reco  },
    { LVGUI_MODE_INFO,         lvgui_draw_info  },
    { LVGUI_MODE_VISI,         lvgui_draw_visi  },
    { LVGUI_MODE_TIPS,         lvgui_draw_tips  },
    { LVGUI_MODE_ADVT,         lvgui_draw_advt  },
    { LVGUI_MODE_TEST,         lvgui_draw_test  },
};

static const char *lvgui_draw_ftfonts       = "./data/fonts.ttf";
static const char *lvgui_draw_default_user  = LVGUI_SCREEN_PATH"/user.bmp";
static const char *lvgui_draw_default_init  = LVGUI_SCREEN_PATH"/init.png";
static const char *lvgui_draw_default_visi  = LVGUI_SCREEN_PATH"/visi.png";
static const char *lvgui_draw_default_tips  = LVGUI_SCREEN_PATH"/tips.png";

static void lvgui_paint(lvgui_t *gui);

#define GUI_CHECK_NULL(obj_val)             \
        do{                                 \
            if(obj_val == NULL){            \
                log_warn("create %s fail\n", #obj_val);     \
                return -1;                \
            }                               \
        }while(0)


static lvgui_t *lvgui_get(){
    static lvgui_t gui = {};

    return &gui;
}

static void lvgui_paint(lvgui_t *gui){
    lvgui_state_paint_t *sp;

    pthread_mutex_lock(&gui->lock);

    if((gui->state >= LVGUI_MODE_NONE) && (gui->state < LVGUI_MODE_MAX)){
        sp = &lvgui_paints[gui->state];
        sp->painter(gui);
    }

    pthread_mutex_unlock(&gui->lock);
}

static void lvgui_settimer(struct event *ev, int seconds, int useconds){
    // log_info("lvgui_settimer ==> seconds : %d\n", seconds);

    if((seconds > 0)||(useconds > 0)){
        struct timeval tv = {seconds, useconds};
        event_add(ev, &tv);
    }else{
        event_del(ev);
    }
}

static void lvgui_event_handler(struct bufferevent *bev, void *ctx){
    fdrcp_conf_t *conf = fdrcp_conf_get();
    lvgui_t *gui = (lvgui_t *)ctx;
    lvgui_evmsg_t em;
    lvgui_user_t *user = NULL, *pos;

    struct evbuffer *input = bufferevent_get_input(bev);

    while(evbuffer_get_length(input) >= sizeof(em)){
        evbuffer_remove(input, &em, sizeof(em));

        // log_info("lvgui_event_handler receive event : %d\n", em.type);

        switch(em.type){
        case LVGUI_EVMSG_USER:
            user = (lvgui_user_t *)em.data;
            list_add_tail(&user->node, &gui->show_users);
            continue;

        case LVGUI_EVMSG_MODE:
            if((em.mode >= LVGUI_MODE_NONE) && (em.mode < LVGUI_MODE_MAX)){
                // mode changed, clear unshown user info
                if(gui->state == LVGUI_MODE_RECO){
                    list_for_each_entry_safe(user, pos, &gui->show_users, node){
                        list_del(&user->node);
                        lvgui_free_user(user);
                    }

                    user = NULL;
                }

                gui->state = em.mode;
                lvgui_paint(gui);

                lvgui_settimer(&gui->ev_timer, em.seconds, 0);

                usleep(1000*100);    // fps = 10
            }
            break;

        case LVGUI_EVMSG_THERM:
            if(gui->state == LVGUI_MODE_RECO){
                char therm[16];
                snprintf(therm, 15, "%.2f", em.therm);

                pthread_mutex_lock(&gui->lock);

                lv_label_set_text(gui->obj_therm, therm);
                if(em.therm > conf->therm_threshold){
                    gui->sttherm.body.main_color = LVGUI_COLOR_THERM_DENY;
                    gui->sttherm.body.main_color = LVGUI_COLOR_THERM_DENY;
                }else{
                    gui->sttherm.body.main_color = LVGUI_COLOR_THERM_PERM;
                    gui->sttherm.body.main_color = LVGUI_COLOR_THERM_PERM;
                }
                lv_label_set_style(gui->obj_therm, LV_LABEL_STYLE_MAIN, &gui->sttherm);
                lv_obj_invalidate(gui->obj_therm);

                pthread_mutex_unlock(&gui->lock);
            }
            break;

        default:
            log_warn("lvgui_event_handler => receive unkown msg:%d\n", em.type);
            break;
        }

    }

    if(user != NULL){
        gui->state = LVGUI_MODE_RECO;
        lvgui_settimer(&gui->ev_timer, 0, 10000);
    }
}

static void lvgui_draw_none(lvgui_t *gui){
    lv_obj_set_hidden(gui->obj_therm, true);
    lv_obj_set_hidden(gui->obj_tips, true);
    lv_obj_set_hidden(gui->obj_bar, true);
    lv_obj_set_hidden(gui->obj_uname, true);
    lv_obj_set_hidden(gui->obj_udesc, true);

    lv_obj_set_hidden(gui->obj_canvas, true);
}

static void lvgui_draw_init(lvgui_t *gui){
    lv_obj_set_hidden(gui->obj_therm, true);
    lv_obj_set_hidden(gui->obj_tips, true);
    lv_obj_set_hidden(gui->obj_bar, true);
    lv_obj_set_hidden(gui->obj_uname, true);
    lv_obj_set_hidden(gui->obj_udesc, true);

    lv_canvas_draw_img(gui->obj_canvas, 0, 0, lvgui_draw_default_init, &gui->stcanvas);
    lv_obj_set_hidden(gui->obj_canvas, true);
}

static void lvgui_draw_info(lvgui_t *gui){
}

static void lvgui_draw_advt(lvgui_t *gui){
}

static void lvgui_draw_visi(lvgui_t *gui){
    lv_obj_set_hidden(gui->obj_therm, true);
    lv_obj_set_hidden(gui->obj_tips, true);
    lv_obj_set_hidden(gui->obj_bar, true);
    lv_obj_set_hidden(gui->obj_uname, true);
    lv_obj_set_hidden(gui->obj_udesc, true);

    lv_canvas_draw_img(gui->obj_canvas, 0, 0, lvgui_draw_default_visi, &gui->stcanvas);
    lv_obj_set_hidden(gui->obj_canvas, true);
}

static void lvgui_draw_tips(lvgui_t *gui){
    lv_obj_set_hidden(gui->obj_therm, true);
    lv_obj_set_hidden(gui->obj_uname, true);
    lv_obj_set_hidden(gui->obj_udesc, true);

    lv_label_set_text(gui->obj_tips, "\r未能识别您的信息\r短按下方按钮进行操作\r");
    lv_obj_set_hidden(gui->obj_tips, false);

    lv_canvas_fill_bg(gui->obj_bar, LV_COLOR_TRANSP);
    lv_canvas_draw_img(gui->obj_bar, LVGUI_TIPS_IMG_X, LVGUI_TIPS_IMG_Y, lvgui_draw_default_tips, &gui->stbar);
    lv_obj_set_hidden(gui->obj_bar, false);
 }

 static void lvgui_load_user_image(lvgui_t *gui, lvgui_user_t *user){
     int skip = sizeof(lvgui_bmp_header_t) + sizeof(lvgui_bmp_info_t);
     int len = LVGUI_UIMG_W *LVGUI_UIMG_H * LV_COLOR_SIZE / 8;
     int rc;

    rc = fdrcp_utils_read_user_file(user->uid, skip, gui->uimg_buffer, &len, ".bmp");

    if(rc != 0){
        log_warn("lvgui_load_user_image => read %s.bmp fail\n", user->uid);

        fdrcp_utils_read_file(lvgui_draw_default_user, skip, gui->uimg_buffer, &len);

        return;
    }
}

static void lvgui_draw_user(lvgui_t *gui, int x, int y, lvgui_user_t *user){
    lv_color_t cl;

    // draw reco user image
    gui->stbar.body.radius = 2;
    gui->stbar.body.opa = 0xFF;

    if(user->mask&LVGUI_MASK_PASS){
        cl = LVGUI_COLOR_PASS;
    }else if(user->mask & LVGUI_MASK_DENY){
        cl = LVGUI_COLOR_DENY;
    }else if(user->mask & LVGUI_MASK_EXPIRED){
        cl = LVGUI_COLOR_EXPIRED;
    }else{
        cl = LVGUI_COLOR_UNKOWN;
    }

    gui->stbar.body.main_color =  cl;
    gui->stbar.body.grad_color =  cl;

    lvgui_load_user_image(gui, user);

    lv_canvas_draw_rect(gui->obj_bar, x + LVGUI_UIMG_X - 4, y + LVGUI_UIMG_Y - 4, LVGUI_UIMG_W + LVGUI_UIMG_BOX_W*2, LVGUI_UIMG_H + LVGUI_UIMG_BOX_W*2, &gui->stbar);
    lv_canvas_draw_img(gui->obj_bar, x + LVGUI_UIMG_X, y + LVGUI_UIMG_Y, &gui->uimg, &gui->stbar);
}

static void lvgui_draw_reco(lvgui_t *gui){
    fdrcp_conf_t *conf = fdrcp_conf_get();
    lvgui_user_t *u0 = NULL, *u1 = NULL, *u2 = NULL;
    int x = 0;
    int y = 0;

    // 0. if no user, return
    if(list_empty(&gui->show_users)){
        return;
    }

    lv_obj_set_hidden(gui->obj_canvas, false);
    lv_canvas_fill_bg(gui->obj_canvas, LV_COLOR_BLACK);
    lv_obj_move_background(gui->obj_canvas);

    //log_info("lvgui_draw_reco -> 1\n");

    // 1. get users
    u0 = list_first_entry(&gui->show_users, lvgui_user_t, node);
    list_del(&u0->node);

    if(!list_empty(&gui->show_users)){
        u1 = list_first_entry(&gui->show_users, lvgui_user_t, node);
        list_del(&u1->node);
        //log_info("lvgui_draw_reco -> get user 1\n");
    }

    if(!list_empty(&gui->show_users)){
        u2 = list_first_entry(&gui->show_users, lvgui_user_t, node);
        list_del(&u2->node);
        //log_info("lvgui_draw_reco -> get user 2\n");
    }

    //log_info("lvgui_draw_reco -> 2\n");

    // 2. reset canvas pos & size
    lv_obj_set_hidden(gui->obj_tips, false);
    lv_label_set_text(gui->obj_tips, "");

    // 3. therm
    if(!lv_obj_get_hidden(gui->obj_therm)){
        if(conf->therm_mode){
            lv_obj_set_hidden(gui->obj_therm, false);
            lv_label_set_text(gui->obj_therm, "请将手腕靠近测温传感器");
        }
    }

    //log_info("lvgui_draw_reco -> 4\n");
    // 4. draw user
    // draw u0 user image
    x = 0;
    y = 0;
    lv_obj_set_hidden(gui->obj_bar, false);
    lv_canvas_fill_bg(gui->obj_bar, LV_COLOR_TRANSP);

    lvgui_draw_user(gui, x, y, u0);

    list_add_tail(&u0->node, &gui->free_users);

    //log_info("lvgui_draw_reco -> x\n");
    #if 1
    if(u1 == NULL){
        lv_obj_set_hidden(gui->obj_uname, false);
        lv_obj_set_hidden(gui->obj_udesc, false);

        lv_label_set_text(gui->obj_uname, u0->name);
        lv_label_set_text(gui->obj_udesc, u0->desc);
    }else{
        lv_obj_set_hidden(gui->obj_uname, true);
        lv_obj_set_hidden(gui->obj_udesc, true);

        x = LVGUI_SCREEN_WIDTH / 3;
        lvgui_draw_user(gui, x, y, u1);

        list_add_tail(&u1->node, &gui->free_users);

        if(u2 != NULL){
            x = (LVGUI_SCREEN_WIDTH / 3) * 2;
            lvgui_draw_user(gui, x, y, u2);

            list_add_tail(&u2->node, &gui->free_users);
        }
    }
    #endif

    //log_info("lvgui_draw_reco -> 5\n");
    // 5. invalid bg & fg
    lv_obj_invalidate(gui->obj_bar);
 }

static void lvgui_draw_test(lvgui_t *gui){

    lv_obj_set_hidden(gui->obj_canvas, false);
    lv_canvas_fill_bg(gui->obj_canvas, LV_COLOR_BLUE);

    static int c = 0x10;

    c += 16;

    memset(gui->uimg_buffer, c, gui->uimg.data_size);

    lv_canvas_draw_img(gui->obj_canvas, 100, 100, &gui->uimg, &gui->stcanvas);
    lv_obj_invalidate(gui->obj_canvas);
}

static void lvgui_timer_proc(evutil_socket_t sock, short events, void *args){
    lvgui_t *gui = (lvgui_t *)args;
    fdrcp_conf_t *conf = fdrcp_conf_get();

    // log_info("lvgui_timer_proc -> draw %d\n", gui->state);

    if((gui->state >= LVGUI_MODE_MAX) || (gui->state < 0)){
        log_warn("lvgui_timer_proc -> gui in wrong state %d\n", gui->state);
        gui->state = LVGUI_MODE_NONE;
    }

    if(gui->state == LVGUI_MODE_RECO){
        if(!list_empty(&gui->show_users)){
            lvgui_paint(gui);
            lvgui_settimer(&gui->ev_timer, conf->reco_duration, 0);

            return ;
        }else{
            gui->state = LVGUI_MODE_NONE;
        }
    }

    // FIXME: just for test
    //gui->state =  LVGUI_MODE_TEST;
    lvgui_paint(gui);
    //lvgui_settimer(&gui->ev_timer, 1, 0);
}

static void *lvgui_main_routine(void *args){
    lvgui_t *gui = (lvgui_t *)args;
    int rc;

    gui->base = event_base_new();
    if (gui->base == NULL){
        log_warn("lvgui_main_routine => create event_base failed!\n");
        gui->exitloop = 1;
        return (void*)-1;
    }

    // init pip
    rc = bufferevent_pair_new(gui->base, BEV_OPT_THREADSAFE|BEV_OPT_CLOSE_ON_FREE, gui->bepair);
    if(0 != rc){
        log_warn("lvgui_main_routine  => create event pair fail:%d\n", rc);
        event_base_free(gui->base);
        gui->exitloop = 1;
        return (void*)-1;
    }
    bufferevent_enable(gui->bepair[1], EV_READ);
    bufferevent_enable(gui->bepair[0], EV_WRITE);
    bufferevent_setcb (gui->bepair[1], lvgui_event_handler, NULL, NULL, gui);

    // init timer
    event_assign(&gui->ev_timer, gui->base,  -1,  EV_TIMEOUT, lvgui_timer_proc, gui);

    lvgui_settimer(&gui->ev_timer, 1, 0);

    gui->init = 1;

    log_info("lvgui_main_routine => looping ... \n");

    event_base_loop(gui->base, EVLOOP_NO_EXIT_ON_EMPTY);

    bufferevent_free(gui->bepair[0]);
    bufferevent_free(gui->bepair[1]);

    event_base_free(gui->base);

    return NULL;
}

static int lvgui_lvgl_init(lvgui_t *gui)
{
    if(lv_font_freestyle_init(lvgui_draw_ftfonts) != 0){
        log_warn("lvgui_lvgl_init => load font %s fail\n", lvgui_draw_ftfonts);
        return -1;
    }

    lv_init();

    lvgui_decoder_init();

#ifdef XM6XX
    xm6xx_init();
#else
    monitor_init();
#endif

    lv_disp_buf_init(&gui->disp_buffer, gui->buffer, NULL, LVGUI_SCREEN_BUFFER);
    lv_disp_drv_init(&gui->disp_driver);
    gui->disp_driver.buffer = &gui->disp_buffer;

#ifdef XM6XX
    gui->disp_driver.flush_cb = xm6xx_flush;
#else
    gui->disp_driver.flush_cb = monitor_flush;
#endif

    lv_disp_drv_register(&gui->disp_driver);

#ifdef MACOSX_EMU
    // only used in macos
    lv_obj_t *bk = lv_img_create(lv_scr_act(), NULL);
    lv_obj_set_pos(bk, 0, 0);
    lv_obj_set_size(bk, LVGUI_SCREEN_WIDTH, LVGUI_SCREEN_HEIGHT);
    lv_img_set_src(bk, lvgui_draw_default_init);
#endif

    //{{
    // therm label
    lv_style_copy(&gui->sttherm, &lv_style_plain);
    gui->sttherm.body.main_color = LVGUI_COLOR_THERM_PERM;
    gui->sttherm.body.grad_color = LVGUI_COLOR_THERM_PERM;
    gui->sttherm.body.opa        = LV_OPA_60;
    gui->sttherm.body.padding.top = 0;
    gui->sttherm.body.padding.bottom = 0;
    gui->sttherm.text.color      = LV_COLOR_WHITE;
    gui->sttherm.text.font       = &lv_font_ft32;

    gui->obj_therm = lv_label_create(lv_scr_act(), NULL);
    GUI_CHECK_NULL(gui->obj_therm);
    lv_label_set_long_mode(gui->obj_therm, LV_LABEL_LONG_CROP);
    lv_obj_set_pos(gui->obj_therm, 0, LVGUI_SCREEN_HEIGHT - LVGUI_UBAR_HEIGHT - LVGUI_THERM_HEIGHT);
    lv_obj_set_size(gui->obj_therm, LVGUI_SCREEN_WIDTH, LVGUI_THERM_HEIGHT);
    lv_label_set_style(gui->obj_therm, LV_LABEL_STYLE_MAIN, &gui->sttherm);
    lv_label_set_align(gui->obj_therm, LV_LABEL_ALIGN_CENTER);
    lv_label_set_body_draw(gui->obj_therm, true);
    lv_obj_set_hidden(gui->obj_therm, true);

    // tips & background label
    lv_style_copy(&gui->sttips, &lv_style_plain);
    gui->sttips.body.main_color = LVGUI_COLOR_USER_BK;
    gui->sttips.body.grad_color = LVGUI_COLOR_USER_BK;
    gui->sttips.body.padding.top= 0;
    gui->sttips.body.padding.bottom = 0;
    gui->sttips.body.opa        = LV_OPA_60;
    gui->sttips.text.color      = LV_COLOR_WHITE;
    gui->sttips.text.font       = &lv_font_ft32;

    gui->obj_tips = lv_label_create(lv_scr_act(), NULL);
    GUI_CHECK_NULL(gui->obj_tips);
    lv_label_set_long_mode(gui->obj_tips, LV_LABEL_LONG_CROP);
    lv_obj_set_pos(gui->obj_tips, 0, LVGUI_SCREEN_HEIGHT - LVGUI_UBAR_HEIGHT);
    lv_obj_set_size(gui->obj_tips, LVGUI_SCREEN_WIDTH, LVGUI_UBAR_HEIGHT);
    lv_label_set_style(gui->obj_tips, LV_LABEL_STYLE_MAIN, &gui->sttips);
    lv_label_set_align(gui->obj_tips, LV_LABEL_ALIGN_CENTER);
    lv_label_set_body_draw(gui->obj_tips, true);
    lv_obj_set_hidden(gui->obj_tips, true);

    // bar for user info
    lv_style_copy(&gui->stbar, &lv_style_plain);

    gui->obj_bar = lv_canvas_create(lv_scr_act(), NULL);
    GUI_CHECK_NULL(gui->obj_bar);
    lv_canvas_set_buffer(gui->obj_bar, gui->bar_buffer, LVGUI_SCREEN_WIDTH, LVGUI_UBAR_HEIGHT, LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED);
    lv_obj_set_pos(gui->obj_bar, 0, LVGUI_SCREEN_HEIGHT - LVGUI_UBAR_HEIGHT);
    lv_obj_set_size(gui->obj_bar, LVGUI_SCREEN_WIDTH, LVGUI_UBAR_HEIGHT);
    lv_obj_set_hidden(gui->obj_bar, true);

    // label for user name
    lv_style_copy(&gui->stuname, &lv_style_plain);
    gui->stuname.text.color      = LV_COLOR_WHITE;
    gui->stuname.text.font       = &lv_font_ft32;

    gui->obj_uname = lv_label_create(lv_scr_act(), NULL);
    GUI_CHECK_NULL(gui->obj_uname);
    lv_label_set_long_mode(gui->obj_uname, LV_LABEL_LONG_CROP);
    lv_obj_set_pos(gui->obj_uname, LVGUI_USER_NAME_X, LVGUI_USER_NAME_Y);
    lv_obj_set_size(gui->obj_uname, LVGUI_USER_NAME_W, LVGUI_USER_NAME_H);
    lv_label_set_style(gui->obj_uname, LV_LABEL_STYLE_MAIN, &gui->stuname);
    lv_label_set_align(gui->obj_uname, LV_LABEL_ALIGN_LEFT);
    lv_obj_set_hidden(gui->obj_uname, true);

    // label for user desc
    lv_style_copy(&gui->studesc, &lv_style_plain);
    gui->studesc.text.color      = LV_COLOR_WHITE;
    gui->studesc.text.font       = &lv_font_ft24;

    gui->obj_udesc = lv_label_create(lv_scr_act(), NULL);
    GUI_CHECK_NULL(gui->obj_udesc);
    lv_label_set_long_mode(gui->obj_udesc, LV_LABEL_LONG_CROP);
    lv_obj_set_pos(gui->obj_udesc, LVGUI_USER_DESC_X, LVGUI_USER_DESC_Y);
    lv_obj_set_size(gui->obj_udesc, LVGUI_USER_DESC_W, LVGUI_USER_DESC_H);
    lv_label_set_style(gui->obj_udesc, LV_LABEL_STYLE_MAIN, &gui->studesc);
    lv_label_set_align(gui->obj_udesc, LV_LABEL_ALIGN_LEFT);
    lv_obj_set_hidden(gui->obj_udesc, true);

    // canvas for others
    lv_style_copy(&gui->stcanvas, &lv_style_plain);

    gui->obj_canvas = lv_canvas_create(lv_scr_act(), NULL);
    GUI_CHECK_NULL(gui->obj_canvas);
    lv_canvas_set_buffer(gui->obj_canvas, gui->canvas_buffer, LVGUI_SCREEN_WIDTH, LVGUI_SCREEN_HEIGHT, LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED);
    lv_obj_set_pos(gui->obj_canvas, 0, 0);
    lv_obj_set_size(gui->obj_canvas, LVGUI_SCREEN_WIDTH, LVGUI_SCREEN_HEIGHT);
    lv_obj_set_hidden(gui->obj_canvas, true);

    // user image buffer
    gui->uimg.data = gui->uimg_buffer;
    gui->uimg.data_size = LVGUI_UIMG_W *LVGUI_UIMG_H * LV_COLOR_SIZE / 8;
    gui->uimg.header.h = LVGUI_UIMG_H;
    gui->uimg.header.w = LVGUI_UIMG_W;
    gui->uimg.header.reserved = 0;
    gui->uimg.header.always_zero = 0;
    gui->uimg.header.cf = LV_IMG_CF_TRUE_COLOR;
    //}}

    return 0;
}

void lvgui_loop(){
    lvgui_t *gui = lvgui_get();
    lvgui_user_t *pos, *temp;

    while(!gui->exitloop){

        pthread_mutex_lock(&gui->lock);

        usleep(10000);
        lv_task_handler();
        lv_tick_inc(10);

        pthread_mutex_unlock(&gui->lock);

#ifdef MACOSX_EMU
        SDL_Event event;
        while(SDL_PollEvent(&event)){
        }
#endif
    }

    pthread_mutex_lock(&gui->lock);

    list_for_each_entry_safe(pos, temp, &gui->show_users, node){
        list_del(&pos->node);
        free(pos);
    }

    list_for_each_entry_safe(pos, temp, &gui->free_users, node){
        list_del(&pos->node);
        free(pos);
    }

    pthread_mutex_unlock(&gui->lock);

    pthread_mutex_destroy(&gui->lock);
}

int lvgui_init(){
    lvgui_t *gui = lvgui_get();
    int i;
    int rc;

    memset(gui, 0, sizeof(lvgui_t));

    gui->bufsize = LVGUI_SCREEN_BUFFER;

    pthread_mutex_init(&gui->lock, NULL);

    INIT_LIST_HEAD(&gui->show_users);
    INIT_LIST_HEAD(&gui->free_users);

    for(i = 0; i < 16; i ++){
        lvgui_user_t *user;

        user = malloc(sizeof(lvgui_user_t));
        if(user != NULL){
            memset(user, 0, sizeof(lvgui_user_t));
            INIT_LIST_HEAD(&user->node);

            list_add(&user->node, &gui->free_users);
        }
    }

    lvgui_lvgl_init(gui);
    gui->state = LVGUI_MODE_INIT;

    rc = pthread_create(&gui->thrd, NULL, lvgui_main_routine, gui);
    if(rc != 0){
        gui->exitloop = 1;
        log_warn("lvgui_init => create tick thread fail %d\n", rc);
        return -1;
    }

    while(!gui->init){
        usleep(10*1000);
    }

    return 0;
}

int lvgui_fini(){
    lvgui_t *gui = lvgui_get();

    gui->exitloop = 1;

    return 0;
}

lvgui_user_t *lvgui_alloc_user(){
    lvgui_t *gui = lvgui_get();

    lvgui_user_t *fr = NULL;

    pthread_mutex_lock(&gui->lock);

    if(!list_empty(&gui->free_users)){
        fr = list_first_entry(&gui->free_users, lvgui_user_t, node);
        list_del(&fr->node);
    }

    pthread_mutex_unlock(&gui->lock);

    return fr;
}

int lvgui_free_user(lvgui_user_t *user){
    lvgui_t *gui = lvgui_get();

    pthread_mutex_lock(&gui->lock);

    list_add_tail(&user->node, &gui->free_users);

    pthread_mutex_unlock(&gui->lock);

    return 0;
}

int lvgui_dispatch(lvgui_user_t *user){
    lvgui_t *gui = lvgui_get();
    fdrcp_conf_t *conf = fdrcp_conf_get();
    lvgui_evmsg_t em;

    em.type = LVGUI_EVMSG_USER;
    em.data = user;
    em.seconds = conf->reco_duration;

    if(bufferevent_write(gui->bepair[0], &em, sizeof(em))) {
        log_warn("lvgui_dispatch => bufferevent_write failed\r\n");
        return -1;
    }

    return 0;
}

int lvgui_setmode(int mode, int seconds){
    lvgui_t *gui = lvgui_get();
    lvgui_evmsg_t em;

    em.type = LVGUI_EVMSG_MODE;
    em.mode = mode;
    em.seconds = seconds;

    if(bufferevent_write(gui->bepair[0], &em, sizeof(em))) {
        log_warn("lvgui_setmode => bufferevent_write failed\r\n");
        return -1;
    }

    return 0;
}

int lvgui_therm_update(float therm){
    lvgui_t *gui = lvgui_get();
    lvgui_evmsg_t em;

    em.type = LVGUI_EVMSG_THERM;
    em.therm = therm;
    em.seconds = 0;

    if(bufferevent_write(gui->bepair[0], &em, sizeof(em))) {
        log_warn("lvgui_setmode => bufferevent_write failed\r\n");
        return -1;
    }

    return 0;
}
