
#include "hal_adc.h"
#include "hal_sys.h"


uint32_t hal_adc_read(void)
{
    uint32_t val = 0;
    
    reg_read(0x10020480, &val);
    reg_write(0x10020480, val|0x03); //ADC enable
    reg_read(0x10020494, &val);
    val &= ~0x03;
    reg_write(0x10020494, val|0x02); //选择外部ADC输入
    reg_read(0x10020484, &val);
    val = (val & 0xff) >> 2;

    return val;
}


