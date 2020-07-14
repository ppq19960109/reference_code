
#include "sys_comm.h"
#include "xm_comm.h"
#include "hal_vio.h"
#include "hal_log.h"
#include "hal_sys.h"
#include "board.h"


static int32_t _hal_vio_init(uint8_t mode)
{
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    VI_CHN_ATTR_S stAttr;
    VI_EXT_CHN_ATTR_S stExtAttr;
    VO_PUB_ATTR_S pubAttr;
    VO_CHN_ATTR_S chnAttr;

    stAttr.stCapRect.s32X = 2;
    stAttr.stCapRect.s32Y = 2;
    stAttr.stCapRect.u32Width = FDR_IMAGE_SIZE_WIDTH;
    stAttr.stCapRect.u32Height = FDR_IMAGE_SIZE_HEIGHT;
    stAttr.stDestSize.u32Width = LCD_WIDTH;
    stAttr.stDestSize.u32Height = LCD_HEIGHT;
    /*
    stAttr.bMirror = 1;
    stAttr.bFlip = 1;
    */
    stAttr.bFlip = 0;
    XM_MPI_VI_SetChnAttr(0, &stAttr);
    XM_MPI_VI_EnableChn(0);

    stExtAttr.stDestSize.u32Width = 264;
    stExtAttr.stDestSize.u32Height = 160;
    XM_MPI_VI_SetExtChnAttr(FDR_RGB_SHOW_VI_CHN, &stExtAttr);
    XM_MPI_VI_EnableChn(FDR_RGB_SHOW_VI_CHN);


    stExtAttr.stDestSize.u32Width = FDR_IMAGE_SIZE_WIDTH;
	stExtAttr.stDestSize.u32Height = FDR_IMAGE_SIZE_HEIGHT;
	XM_MPI_VI_SetExtChnAttr(FDR_IR_REC_VI_CHN, &stExtAttr);
	XM_MPI_VI_EnableChn(FDR_IR_REC_VI_CHN);


	stExtAttr.stDestSize.u32Width = LCD_WIDTH;
	stExtAttr.stDestSize.u32Height = LCD_HEIGHT;
	XM_MPI_VI_SetExtChnAttr(FDR_IR_SHOW_VI_CHN, &stExtAttr);
    if(VO_BLACK_MODE == mode) {
        XM_MPI_VI_EnableChn(FDR_IR_SHOW_VI_CHN);
    }

    pubAttr.u32BgColor = 0;
    pubAttr.enIntfType = VO_INTF_LCD;
    pubAttr.enIntfSync = MIP_VO_INTERFACE;
    XM_MPI_VO_SetPubAttr(0, &pubAttr);

    chnAttr.u32Priority = 0;
    chnAttr.stRect.s32X = 586;
    chnAttr.stRect.s32Y = 0;
    chnAttr.stRect.u32Width = 264;
    chnAttr.stRect.u32Height = 160;
    XM_MPI_VO_SetChnAttr(0, FDR_RGB_SHOW_VO_CHN, &chnAttr);
    XM_MPI_VO_SetChnAttr(0, FDR_IR_SHOW_VO_CHN, &chnAttr);

    stSrcChn.enModId = XM_ID_VIU;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = FDR_RGB_SHOW_VI_CHN;
    stDestChn.enModId = XM_ID_VOU;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = FDR_RGB_SHOW_VO_CHN;
    XM_MPI_SYS_Bind(&stSrcChn, &stDestChn);

    stSrcChn.enModId = XM_ID_VIU;
	stSrcChn.s32DevId = 0;
	stSrcChn.s32ChnId = FDR_IR_SHOW_VI_CHN;
	stDestChn.enModId = XM_ID_VOU;
	stDestChn.s32DevId = 0;
	stDestChn.s32ChnId = FDR_IR_SHOW_VO_CHN;
	XM_MPI_SYS_Bind(&stSrcChn, &stDestChn);

    if(VO_COLOR_MODE == mode) {
        XM_MPI_VO_DisableChn(0, FDR_IR_SHOW_VO_CHN);
        XM_MPI_VO_EnableChn(0, FDR_RGB_SHOW_VO_CHN);
    } else if (VO_BLACK_MODE == mode){
        XM_MPI_VO_DisableChn(0, FDR_RGB_SHOW_VO_CHN);
        XM_MPI_VO_EnableChn(0, FDR_IR_SHOW_VO_CHN);
    } else {
        // do not display
    }

    printf("lcd init ok\r\n");
    return 0;
}

int32_t hal_vio_disable(void)
{
    return XM_MPI_VO_DisableChn(0, FDR_RGB_SHOW_VO_CHN);
}

int32_t hal_vio_enable(void)
{
    return XM_MPI_VO_EnableChn(0, FDR_RGB_SHOW_VO_CHN);
}

static int32_t hal_isp_init(void)
{
    uint8_t u8Mode = 0; // 0:auto 1:manual
    uint32_t u32SnsShut = 100;
    uint32_t u32SnsAGain = 1024;
    uint32_t u32SnsDGain = 1024;
    uint32_t u32IspDGain = 1024;

    system(ISP_BIN_PATH);
    sleep(1);

    if(camera_init(NULL)) {
        hal_error("camera_init failed.\r\n");
    }

#if 1
    if(camera_set_refrence_level(50)) {
        hal_error("camera_set_refrence_level failed.\r\n");
    }
    hal_debug("camera_get_refrence_level ok %d.\r\n", camera_get_refrence_level());
#endif

    if(camera_set_ae_attr(u8Mode,u32SnsShut,u32SnsAGain,u32SnsDGain,u32IspDGain) != 0) {
        hal_error("ERR: camera_set_ae_attr failed!\n");
        return -1;
    }

    reg_write(0x1002009c, 0x84);
    reg_write(0x100200a4, 0x84);

//#if defined(XM650)
    reg_write(0x1002006C, 0xc00);//LCD_PWM/GPIO27 HIGH
    reg_write(0x10020074, 0x04);
//#endif

    return 0;
}

int32_t hal_vio_init(void)
{
    int32_t ret = 0;

    ret += XM_MPI_SYS_Init();
    hal_isp_init();
    hal_debug("isp init ok\r\n");
#if (VO_COLOR_MODE == VO_MODE) || (VO_BLACK_MODE == VO_MODE)
#if defined(LCD_SIZE_TYPE) && (LCD_SIZE_TYPE == LCD_SIZE_43_INCH)
    /* 4.3 inch lcd*/
    hal_debug("4.3 lcd\r\n");
    reg_write(0x20000000, 1);
    reg_write(0x20000044, 0x2b83);
    reg_write(0x20000000, 0);
#elif defined(LCD_SIZE_TYPE) && (LCD_SIZE_TYPE == LCD_SIZE_50_INCH)
    hal_debug("5.0 lcd\r\n");
    /* 5.0 inch lcd */
    reg_write(0x20000000, 1);
	reg_write(0x20000044, 0x2f2f);//0x1f1f: 1188M / (0x1f + 1) = 37; 0x2f2f: 25M
	reg_write(0x20000000, 0);
#else
    hal_debug("7.0 lcd\r\n");
    /* 5.0 inch lcd */
    reg_write(0x20000000, 1);
	reg_write(0x20000044, 0x1c1c);//0x1f1f: 1188M / (0x1f + 1) = 37; 0x2f2f: 25M
	reg_write(0x20000000, 0);
    reg_write(0x30085030, 2);//flip
#endif
#endif
    ret += _hal_vio_init(VO_MODE);/* BLACK_MODE, COLOR_MODE */


    return ret;
}

