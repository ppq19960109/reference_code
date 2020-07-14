
#include "sys_comm.h"
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
#include "hal_jpeg.h"
#include "hal_audio.h"
#include "hal_key.h"
#include "hal_jpeg_player.h"
#include "by_f300.h"

int _audio_test(void *handle, int status)
{
    static uint8_t start = 0;
    static char filename[32] = {0};

    if(status) {
        sprintf(filename, "%d.pcm", start);
        hal_audio_record_start(filename);
    } else {
        start++;
        hal_audio_record_stop();
        hal_audio_play(filename, 100);
    }

    return 0;
}

int qrcode_handler(void *handle, const char *qrcode, int len)
{
    return 0;
}

#if 1
int32_t hal_window_enable(void);
int32_t hal_window_disable(void);
int32_t hal_osd_window_enable(void);
int32_t hal_osd_window_disable(void);
int32_t main(int32_t argc, char *argv[])
{
    hal_vio_init();

    hal_layer_init();
    // hal_window_disable();
    // hal_osd_window_disable();
    hal_bmp_show("/var/sd/app/data/ui/startup.bmp", 0, 0, 640, 480, 1, 0xff);
    hal_bmp_layer_enable();

    hal_audio_init();

    hal_hw_init();
    hal_key_regist_event(_audio_test, NULL);

    by_f300_init();

    hal_wifi_enable();

    hal_jpeg_init();

    jpeg_player_init();
    hal_jpeg_regist_render(jpeg_player_image);

    hal_fdr_init(FACE_DETECT_REC_MODE_E);

    //hal_image_set_mode(IMG_QRCODE_MODE);
    //hal_set_qrcode_cb(qrcode_handler, NULL);
    hal_image_init();

    hal_info("init all over\r\n");

    while(1) {
        //hal_debug("TM: %u, adc value = %d\r\n", hal_time_get_ms(), hal_adc_read());
        by_f300_play(0);
        sleep(2);
        by_f300_play(1);
        sleep(2);
    }
    return 0;
}

#else
int32_t main(int32_t argc, char *argv[])
{
    by_f300_init();

    while(1) {
        //hal_debug("TM: %u, adc value = %d\r\n", hal_time_get_ms(), hal_adc_read());
        by_f300_play(1);
        sleep(2);
        by_f300_play(2);
        sleep(2);
    }
    return 0;
}

#endif

