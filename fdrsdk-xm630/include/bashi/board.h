
#ifndef _BOARD_H_
#define _BOARD_H_

#include "stdint.h"


 #define BSM650FV100 0
 #define BSM650FV101 1
 #define BSM650LV100 2

// #define CONFIG_BSBOARD BSM650LV100
#if !defined(CONFIG_BSBOARD)

#error "Please define the mother board type"

#elif (CONFIG_BSBOARD == BSM650FV100) || (CONFIG_BSBOARD == BSM650FV101)

/* key */
#define HAL_KEY_GPIO_NM         84
#define KEY_PRESS_UP_STATE      GPIO_LOW
/* RS485 */
#define RS485_RTX_PIN           86
/* audio */
#define AUDIO_ENABLE_PIN        72
/* fdr */
#define FACE_CLASSIFIER_PATH    "/var/sd/app/data/classifier"
#define ISP_BIN_PATH            "/var/sd/app/usr/isp &"
#define FONT_PATH               "/var/sd/app/data/fonts/font.bin"
/* led */
#define WHITE_LED_GPIO_PIN      73
#define IR_LED_GPIO_PIN         1
/* relay */
#define HAL_RELAY_GPIO_NM       69
/* wifi */
#define USB_WIFI_ENABLE_PIN     85


#elif (CONFIG_BSBOARD == BSM650LV100)

/* key */
#define HAL_KEY_GPIO_NM         71
#define KEY_PRESS_UP_STATE      GPIO_LOW
/* RS485 */
#define RS485_RTX_PIN           2
/* audio */
#define AUDIO_ENABLE_PIN        72
/* fdr */
#define FACE_CLASSIFIER_PATH    "/var/sd/app/data/classifier"
#define ISP_BIN_PATH            "/var/sd/app/usr/isp &"
#define FONT_PATH               "/var/sd/app/data/fonts/font.bin"
/* led */
#define WHITE_LED_GPIO_PIN      29
#define IR_LED_GPIO_PIN         1
/* relay */
#define HAL_RELAY_GPIO_NM       69
/* wifi */
#define USB_WIFI_ENABLE_PIN     49

#else
#error "Please unknow board type"
#endif





#endif

