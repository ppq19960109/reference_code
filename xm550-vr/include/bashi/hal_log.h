
#ifndef _HAL_LOG_H_
#define _HAL_LOG_H_


#include "hal_types.h"


/*
1> format:
    \033[method;前景色;背景色m

      1）显示方式：0（默认值）、1（高亮）、22（非粗体）、4（下划线）、24（非下划线）、5（闪烁）、25（非闪烁）、7（反显）、27（非反显）
      2）前景色：30（黑色）、31（红色）、32（绿色）、 33（黄色）、34（蓝色）、35（洋红）、36（青色）、37（白色）
      3）背景色：40（黑色）、41（红色）、42（绿色）、 43（黄色）、44（蓝色）、45（洋红）、46（青色）、47（白色）
      
2) eg:
    printf( "\033[1;31;40m Red: hello world\r\n \033[0m" );
*/
#if 1
#define NORMAL_COLOR  "\033[0m"
#else
#define NORMAL_COLOR
#endif
#define RED_COLOR  "\033[1;31m"
#define GREEN_COLOR  "\033[1;32m"
#define YELLO_COLOR  "\033[1;33m"
#define BLUE_COLOR  "\033[1;34m"



#if 1
#define hal_printf(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define hal_printf(fmt, ...)
#endif



#define hal_log(tag, fmt, ...) \
    do {\
        hal_printf(tag ": %s@%s#%d: " fmt, __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__); \
    }while(0)
#define hal_debug(fmt, ...)  \
    do {\
        hal_log(GREEN_COLOR"[DEBUG]", fmt NORMAL_COLOR, ##__VA_ARGS__); \
    }while(0)
    
#define hal_info(fmt, ...)  \
    do {\
        hal_log(BLUE_COLOR"[INFO]", fmt NORMAL_COLOR, ##__VA_ARGS__);  \
    }while(0)

#define hal_warn(fmt, ...)  \
    do {\
        hal_log(YELLO_COLOR"[WARN]", fmt NORMAL_COLOR, ##__VA_ARGS__);  \
    }while(0)

#define hal_error(fmt, ...) \
    do {\
        hal_log(RED_COLOR"[ERROR]", fmt NORMAL_COLOR, ##__VA_ARGS__); \
    }while(0)
    
#define hal_fatal(fmt, ...) \
    do {\
        hal_log(RED_COLOR"[FATAL]", fmt NORMAL_COLOR, ##__VA_ARGS__);; \
    }while(0)


#endif

