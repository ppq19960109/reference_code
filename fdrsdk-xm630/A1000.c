
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
#include "hal_pwm.h"
#include "board.h"

void pwm_test(void)
{
    uint32_t duty = 0;
    uint8_t pwm = 0;
    static uint8_t old = 0;
    for(duty = 0; duty <= 100; duty += 5) {
        for(pwm = 0; pwm < HAL_PWM_MAX; pwm++) {
            hal_pwm_config(old, duty, 5000, 0);
            hal_pwm_config(pwm, duty, 5000, 1);
            hal_info("duty = %d\r\n", duty);
            old = pwm;
            sleep(1);
        }
    }
}

int _audio_test(void *handle, int status)
{
    static uint8_t start = 0;
    static char filename[32] = {0};

    if(status) {
        hal_info("start...\r\n");
        sprintf(filename, "%d.pcm", start);
        hal_audio_record_start(filename);
    } else {
        start++;
        hal_info("stop...\r\n");
        hal_audio_record_stop();
        hal_audio_play(filename, 100);
    }
    return 0;
}


int32_t main(int32_t argc, char *argv[])
{
    hal_info("board: %d, LCD_SIZE_TYPE: %d, VO_MODE: %d\r\n", CONFIG_BSBOARD, LCD_SIZE_TYPE, VO_MODE);

    hal_vio_init();

    hal_audio_init();

    hal_hw_init();
    //hal_key_regist_event(_audio_test, NULL);
    
    hal_jpeg_init();
    
    jpeg_player_init();
    hal_jpeg_regist_render(jpeg_player_image);
    
    hal_fdr_init(FACE_DETECT_REC_MODE_E);

    //hal_image_set_mode(IMG_QRCODE_MODE);
    //hal_set_qrcode_cb(qrcode_handler, NULL);
    hal_image_init();

    //hal_hw_init();
    hal_info("init all over\r\n");
    
    while(1) {
        pwm_test();
        sleep(2);
    }
    return 0;
}


