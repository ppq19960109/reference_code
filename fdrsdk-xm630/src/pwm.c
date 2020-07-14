#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>

#include "hal_sys.h"
#include "hal_log.h"
#include "pwm.h"

#define GPIO_BASE                   0x10020000
#define PWM_BASE                    0x10022000
#define PWM_DVR_START               0x00
#define PWM_CYC                     0x04
#define PWM_POLE                    0x08
#define PWM_STEP_DIRECT             0x0C
#define PWM_STEP_LENGTH             0x10
#define PWM_STEP_NUM                0x14
#define PWM_PHA_INI                 0x18
#define PWM_PHB_INI                 0x1C
#define PWM_INT_EN                  0x20
#define PWM_INT_CLEAR               0x24
#define PWM_INT_SRC_STATE           0x28
#define PWM_INT_MSK_STATE           0x2C
#define PWM_OEN                     0x3C
#define PWM_PHASE_VAL(n)            (0x40 + (n) * 4)

#define PWM_CLK  12000000
#define CLIP3(x,min,max)         ( (x)< (min) ? (min) : ((x)>(max)?(max):(x)) )

static int g_level = -1;


int SetPwm(int gpio, PWM_PARAM_S *pstParam)
{
    int i;
    unsigned int ratio;
    unsigned int reg_val;
    unsigned int max_level;

    int pwm_i = (gpio - 27) / 4;  //pwm 0, 1, 2, 3
    int pwm_b = ((gpio - 27) / 2) % 2;  //pwm A or B
    int pwm_n = (gpio - 27) % 2;  //pwm A or An

    if(gpio < 27 || gpio > 42)
    {
        hal_warn("invalid pwm gpio %d!\n", gpio);
        return -1;
    }
    if(pstParam->level > 100)
    {
        hal_warn("invalid level %d!\n", pstParam->level);
        return -1;
    }
    if(pstParam->freq <= (PWM_CLK * 2 / 65536) || pstParam->freq > (PWM_CLK / 100))
    {
        hal_warn("invalid freq %d!\n", pstParam->freq);
        return -1;
    }

    if(!reg_read(0x20000F2C, &reg_val) && (reg_val == 0x24242424))
    {
        max_level = PWM_CLK * 2 / pstParam->freq - 1;
    }
    else
    {
        max_level = PWM_CLK / pstParam->freq - 1;
    }

    ratio = CLIP3(pstParam->level, 0, 100);
    if(ratio == g_level)    
        return 0;

    if(pwm_i < 2)
    {
        reg_write(GPIO_BASE + gpio * 4, 0x8);
        pwm_i += 2;
    }
    else
    {
        reg_write(GPIO_BASE + gpio * 4, 0x2);
        pwm_i -= 2;
    }

    //先复位PWM模块
    reg_write(0x200000F8 + pwm_i * 0x4, 0xA9B1001A + pwm_i);
    usleep(1);
    reg_write(0x200000F8 + pwm_i * 0x4, 1);
    usleep(1);

    g_level = ratio;
    ratio = max_level - max_level * ratio / 100;
    for(i = 0; i < 32; i++)
    {
        reg_write(PWM_BASE + pwm_i * 0x1000 + PWM_PHASE_VAL(i), ratio);
    }
    reg_write(PWM_BASE + pwm_i * 0x1000 + (pwm_b ? PWM_PHB_INI : PWM_PHA_INI), 0);
    reg_write(PWM_BASE + pwm_i * 0x1000 + PWM_POLE, pwm_n);
    reg_write(PWM_BASE + pwm_i * 0x1000 + PWM_CYC, max_level);
    reg_write(PWM_BASE + pwm_i * 0x1000 + PWM_DVR_START, 1);
    reg_write(PWM_BASE + pwm_i * 0x1000 + PWM_OEN, 1);

    return 0;
}

#if 0
int main(int argc, char *argv[])
{
    if(argc < 3) {
        printf("usage: %s [gpio] [duty]\r\n", argv[0]);
        return -1;
    }
    PWM_PARAM_S param;
    param.level = atoi(argv[2]);
    param.freq = 1000;
    SetPwm(atoi(argv[1]), &param);

    return 0;
}
#endif

