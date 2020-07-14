

#include "bisp_aie.h"
#include "bisp_hal.h"
#include "bisp_gui.h"
#include "fdr-cp.h"

#include "logger.h"
#include "sys_comm.h"
#include "linux/reboot.h"
#include "sys/reboot.h"
#include "xm_comm.h"
#include "hal_vio.h"
#include "hal_hw_ctl.h"
#include "hal_layer.h"
#include "hal_image.h"
#include "hal_485.h"
#include "hal_gpio.h"
#include "hal_adc.h"
#include "hal_log.h"
#include "hal_timer.h"
#include "hal_wifi.h"
#include "hal_fdr.h"
#include "hal_led.h"
#include "hal_relay.h"
#include "hal_key.h"
#include "hal_pir.h"
#include "hal_jpeg.h"
#include "hal_jpeg_player.h"
#include "hal_audio.h"
#include "fdr-path.h"

#define FDR_GUI_FILENAME_MAX        128
#define FDR_VOICE_FILENAME_MAX      128

typedef struct {
    char *filename;
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
    uint8_t flag;   //clear center rect, (x, y, w, h) = (222, 71, 338, 338)
    uint8_t alpha;
}fdr_gui_t;

typedef struct {
    char *filename;
}fdr_perm_gui_t;

static const fdr_gui_t fdr_gui[BISP_GUI_FDRMODE_MAX] = {
    {"startup.bmp", 0, 0, 800, 480, 0, 0xff},
    {NULL, 0, 0, 0, 0, 0, 0},
    {"active-online.bmp", 0, 0, 730, 480, 0, 0xff},
    {"active-offline.bmp", 0, 0, 730, 480, 0, 0xff},
    {"stranger.bmp", 640, 0, 160, 480, 0, 0xb0},
    {"visitor.bmp", 0, 0, 800, 480, 1, 0xb0},
    {"visitor-ok.bmp", 0, 0, 800, 480, 1, 0xb0},
    {"visitor-fail.bmp", 0, 0, 800, 480, 1, 0xb0},
    {"config-local.bmp", 0, 0, 730, 480, 0, 0xff},
    {"config-online.bmp", 0, 0, 730, 480, 0, 0xff},
    {"config-offline.bmp", 0, 0, 730, 480, 0, 0xff},
    {NULL, 0, 0, 0, 0, 0, 0},
};

static const fdr_perm_gui_t fdr_perm_gui[BISP_GUI_FDRWIN_MAX] = {{NULL}, {"permit.bmp"}, {"deny.bmp"}, {"expired.bmp"}};

static const char *fdr_voice[BISP_HAL_VOICE_MAX] = {"success.pcm", "fail.pcm", NULL, NULL, NULL, NULL, NULL, NULL};

int camera_get_refrence_level();
int camera_set_refrence_level(int level);

static bisp_aie_fdr_dispatch fdr_dispatch = NULL;
static int32_t _bisp_aie_fdr_dispatch(void *handle, void *fr, uint32_t rnum, void *img)
{
    bisp_aie_frval_t *rfs = fr;
    
    if(NULL == fdr_dispatch) {
        return -1;
    }
    rfs->rect.x0 = rfs->rect.x0 * LCD_WIDTH / 8192;
    rfs->rect.x1 = rfs->rect.x1 * LCD_WIDTH / 8192;
    rfs->rect.y0 = rfs->rect.y0 * LCD_HEIGHT / 8192;
    rfs->rect.y1 = rfs->rect.y1 * LCD_HEIGHT / 8192;
    return fdr_dispatch(0, NULL, rnum, rfs, img);
}

void bisp_aie_set_fdrdispatch(bisp_aie_fdr_dispatch dispatch){
    fdr_dispatch = dispatch;
    hal_set_fdr_cb(_bisp_aie_fdr_dispatch, NULL);
}

static bisp_aie_qrc_dispatch qrc_dispatch = NULL;
static int _bisp_aie_qrc_dispatch(void *handle, const char *qrcode, int len)
{
    if(qrc_dispatch) {
        return qrc_dispatch(qrcode, len);
    }
    return -1;
}

void bisp_aie_set_qrcdispatch(bisp_aie_qrc_dispatch dispatch){
    qrc_dispatch = dispatch;
    hal_set_qrcode_cb(_bisp_aie_qrc_dispatch, NULL);
}

// write frame data to jpeg file
int bisp_aie_writetojpeg(const char *path, void *frame){
    return hal_image_capture(path, 0);
}

// release frame data
void bisp_aie_release_frame(void *frame){
    hal_release_all_frame();
}

// get algm face feature
// void *img, int type, - image data, type, NV12 for now
// int width, int heigh, - image size
// float *feature, int *len - face feature buffer, alloc by input, len - input/output size
int bisp_aie_get_fdrfeature(void *img, int type, int width, int heigh, float *feature, size_t *len){
    int ret = -1;
    
    if(*len < (FACE_REC_FEATURE_NUM * sizeof(float))) {
        return -1;
    }
    if(hal_fdr_config(FACE_EXTRACT_FEATURE_E)) {
        return -1;
    }
    ret = 0;
    if(hal_get_face_feature(img, width, heigh, feature)) {
        hal_error("get feature failed\r\n");
        ret = -1;
    }
    hal_fdr_config(FACE_DETECT_REC_MODE_E);
    return ret;
}

int bisp_aie_set_mode(int mode){
    hal_info("set mode %d\r\n", mode);
    if(mode >= BISP_AIE_MODE_NONE) {
        return -1;
    } else if(mode == BISP_AIE_MODE_QRCODE) {
        hal_led_on_duration(UINT32_MAX / 2);
    } else {
        hal_led_on_duration(2000);
    }
    return hal_image_set_mode(mode);
}

int bisp_aie_init(){
    hal_jpeg_init();
    
#ifdef TEST_VERSION
    jpeg_player_init();
    hal_jpeg_regist_render(jpeg_player_image);
#endif

    hal_fdr_init(FACE_DETECT_REC_MODE_E);
    hal_image_init();
    return 0;
}

int bisp_aie_fini(){
    return 0;
}

static bisp_hal_key_dispatch key_dispatch = NULL;
static int _bisp_hal_key_dispatch(void *handle, int status)
{
    if(key_dispatch) {
        return key_dispatch(status);
    }
    return -1;
}
// set key callback
int bisp_hal_set_keydispatch(bisp_hal_key_dispatch dispatch){
    key_dispatch = dispatch;
    return hal_key_regist_event(_bisp_hal_key_dispatch, NULL);
}


// restart os
void bisp_hal_restart(){
    sync();
    reboot(LINUX_REBOOT_CMD_RESTART);
}


// get light sensor lumen
int bisp_hal_get_lumen(){
    uint32_t adc = hal_adc_read();
    return adc;
}

// get PIR sessor status
int bisp_hal_get_pir(){
    return -1;
}

// enable = 1, light on;  enable = 0, light off
int bisp_hal_enable_light(int enable){
    if(enable) {
        hal_led_on(WHITE_LED_E);
        hal_led_on(IR_LED_E);
    } else {
        hal_led_off(WHITE_LED_E);
        hal_led_off(IR_LED_E);
    }
    return 0;
}

// send data to serial port
// idx - id of serial port
// data - for writing data
// len - length of data
int bisp_hal_write_serialport(int idx, const void *data, unsigned int len){
    return hal_485_write((uint8_t *)data, len);
}

// read data from serial port
int bisp_hal_read_serialport(int idx, void *data, unsigned int *len){
    return hal_485_read((uint8_t *)data, len);
}

// relay control
int bisp_hal_enable_relay(int idx, int enable){
    if(enable) {
        return hal_relay_on();
    }
    return hal_relay_off();
}

// void play
int bisp_hal_voice_play(unsigned int mode)
{
    char filename[FDR_VOICE_FILENAME_MAX] = {0};
    
    if(mode >= BISP_HAL_VOICE_MAX) {
        hal_warn("unkown voice mode: %d\r\n", mode);
        return -1;
    }

    if(NULL == fdr_voice[mode]) {
        hal_warn("no voice file, mode: %d\r\n", mode);
        return -1;
    }
    
    snprintf(filename, FDR_VOICE_FILENAME_MAX - 1, "%s/%s",
                        FDR_VOICE_PATH, fdr_voice[mode]);

    return hal_audio_play(filename, 100);
}

// init hwp
int bisp_hal_init(){
    hal_hw_init();
    hal_audio_init();
    return 0;
}

// fini hwp
int bisp_hal_fini(){
    return 0;
}

int32_t hal_window_enable(void);
int32_t hal_window_disable(void);
int32_t hal_osd_window_enable(void);
int32_t hal_osd_window_disable(void);
int32_t hal_osd_show(uint32_t layer, char *str);
int32_t hal_osd_close(uint32_t layer);


char *fdr_get_ipaddr(const char * ifname);

// bisp gui init / fini
int bisp_gui_init(){
    hal_vio_init();
    hal_layer_init();
    
    hal_window_disable();
    hal_osd_window_disable();
    
    return 0;
}

int bisp_gui_fini(){
    hal_layer_destory();
    return 0;
}

int32_t fdr_osd_show_ipaddr(void)
{
    hal_osd_window_enable();
    hal_osd_show(OSD_IPADDR_LAYER_E, fdr_get_ipaddr("eth0"));
    return 0;
}

int32_t fdr_osd_close_ipaddr(void)
{
    hal_osd_window_disable();
    hal_osd_close(OSD_IPADDR_LAYER_E);
    return 0;
}


// show fdr mode : active, config, visitor, stranger
int bisp_gui_show_fdrmode(uint32_t m){
    static uint32_t mode = BISP_GUI_FDRMODE_FACEREC;
    char filename[FDR_GUI_FILENAME_MAX] = {0};
    uint8_t flag = 0;
    hal_rect_corner_t rect;
    rect.x = 200;
    rect.y = 40;
    rect.w = 400;
    rect.h = 400;
    
    if(m >= BISP_GUI_FDRMODE_MAX || m == mode) {
        return -1;
    }
    
    mode = m;
    switch (mode) {
    case BISP_GUI_FDRMODE_USER_REC:
        fdr_osd_close_ipaddr();
        return 0;
        break;
    case BISP_GUI_FDRMODE_STRANGER:
        fdr_osd_close_ipaddr();
        flag = 1;
        break;
    case BISP_GUI_FDRMODE_AC_CONNECTED:
    case BISP_GUI_FDRMODE_AC_UNCONNECTED:
    case BISP_GUI_FDRMODE_CF_CLOUD_CONNECTED:
    case BISP_GUI_FDRMODE_CF_CLOUD_UNCONNECTED:
    case BISP_GUI_FDRMODE_CF_LOCAL:
        fdr_osd_show_ipaddr();
        flag = 1;
        break;
    default:
        fdr_osd_close_ipaddr();
        flag = 0;
        break;
    }
    
    if(NULL == fdr_gui[mode].filename) {
        hal_bmp_layer_disable();
        return 0;
    }
    snprintf(filename, FDR_GUI_FILENAME_MAX - 1, "%s/%s",
                        FDR_UI_PATH, fdr_gui[mode].filename);
    
    if(hal_bmp_show(filename, fdr_gui[mode].x, fdr_gui[mode].y, 
                        fdr_gui[mode].x + fdr_gui[mode].w,
                        fdr_gui[mode].y + fdr_gui[mode].h,
                        flag, fdr_gui[mode].alpha)) {
        return -1;
    }
    if(fdr_gui[mode].flag) {
        hal_bmp_clear_layer(222, 71, 338, 338);
    }
    if(mode == BISP_GUI_FDRMODE_STRANGER) {
        if(hal_bmp_set_rect_corner(&rect, 3, 16, 0x0195)) {
            hal_error("draw rect corner failed\r\n");
        } else {
            hal_info("draw rect corner ok\r\n");
        }
    }
    hal_bmp_layer_enable();
    if(mode == BISP_GUI_FDRMODE_STRANGER) {
        bisp_hal_voice_play(BISP_HAL_VOICE_RECOG_FAILURE);
    }
    return 0;
}

static int32_t fdr_bmp_preprocessor(void *bmp, uint32_t w, uint32_t h)
{
    uint16_t *p = bmp;
    uint32_t i = 0;
    uint32_t sz = w * h;

    for(i = 0; i < sz; i++) {
        if(0 == p[i]) {
            p[i] = 1;
        }
    }

    return 0;
}

static int32_t fdr_gui_gen_perm_win(char *bitmap, int w, int h, int perm)
{
    char filename[FDR_GUI_FILENAME_MAX] = {0};
    FILE *fp = NULL;
    size_t  sz = 0;
    void *bmp = NULL;
    uint32_t tsz = w * h * sizeof(uint16_t);

    if(perm >= BISP_GUI_FDRWIN_MAX || NULL == bitmap) {
        hal_warn("unkown perm: %d\r\n", perm);
        return -1;
    }
    if(NULL == fdr_perm_gui[perm].filename) {
        return -1;
    }
    snprintf(filename, FDR_GUI_FILENAME_MAX - 1, "%s/%s", FDR_UI_PATH, fdr_perm_gui[perm].filename);

    fdr_bmp_preprocessor(bitmap, w, h);
    fp = fopen(filename, "rb");
    if(NULL == fp) {
        hal_error("open %s failed, %s.\r\n", filename, strerror(errno));
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    bmp = malloc(sz);
    if(bmp == NULL){
        hal_warn("malloc image buffer fail!\r\n");
        fclose(fp);
        return -1;
    }

    if(fread(bmp, 1, sz, fp) != sz){
        hal_error("read fail!\r\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    sz -= 54;
    memcpy(bitmap + tsz - sz, bmp + 54, sz);
    free(bmp);
    return 0;
}

// show fdr recognized window
int bisp_gui_show_fdrwin(void *bitmap, int width, int heigh, bisp_gui_fdrwin_e perm){
    hal_info("perm: %d\r\n", perm);
    if(BISP_GUI_FDRWIN_NONE == perm) {
        return bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_FACEREC);
    }
    if(fdr_gui_gen_perm_win(bitmap, width, heigh, perm)) {
        return -1;
    }

    if(bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_USER_REC)) {
        return -1;
    }

    if(hal_bmp_set_layer(bitmap, 640, 0, width, heigh, 0xff)) {
        return -1;
    }
    hal_bmp_layer_enable();

#if 1
    if(BISP_GUI_FDRWIN_PERM == perm) {
        bisp_hal_voice_play(BISP_HAL_VOICE_RECOG_SUCCESS);
    } else {
        bisp_hal_voice_play(BISP_HAL_VOICE_RECOG_FAILURE);
    }
#endif

    return 0;
}

int bisp_aie_version(){
    return (0x02 << 16) + (0x01 << 8)  + 2;
}

int bisp_init_before(){
    bisp_gui_init();
    bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_START);
    bisp_aie_init();
    bisp_hal_init();
    return 0;
}

int bisp_init_after(){
    bisp_gui_show_fdrmode(BISP_GUI_FDRMODE_FACEREC);
    bisp_aie_set_fdrdispatch(fdr_cp_fdrdispatch);
    bisp_aie_set_qrcdispatch(fdr_cp_qrcdispatch);
    bisp_hal_set_keydispatch(fdr_cp_keydispatch);
    return 0;
}