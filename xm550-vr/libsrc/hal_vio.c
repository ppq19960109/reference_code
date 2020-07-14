

#include "hal_vio.h"
#include "hal_log.h"
#include "hal_sys.h"

#include "hal_sdk.h"

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
    stAttr.stCapRect.u32Width = IMAGE_SIZE_WIDTH;
    stAttr.stCapRect.u32Height = IMAGE_SIZE_HEIGHT;
    stAttr.stDestSize.u32Width = LCD_WIDTH;
    stAttr.stDestSize.u32Height = LCD_HEIGHT;
    /*
    stAttr.bMirror = 1;
    stAttr.bFlip = 1;
    */
    stAttr.bFlip = 0;
    XM_MPI_VI_SetChnAttr(0, &stAttr);
    XM_MPI_VI_EnableChn(0);

    stExtAttr.stDestSize.u32Width = LCD_WIDTH;
    stExtAttr.stDestSize.u32Height = LCD_HEIGHT;
    XM_MPI_VI_SetExtChnAttr(RGB_SHOW_VI_CHN, &stExtAttr);
    XM_MPI_VI_EnableChn(RGB_SHOW_VI_CHN);

    stExtAttr.stDestSize.u32Width = IMAGE_SIZE_WIDTH;
	stExtAttr.stDestSize.u32Height= IMAGE_SIZE_HEIGHT;
	XM_MPI_VI_SetExtChnAttr(RGB_REC_VI_CHN, &stExtAttr);
    /*
	if(XM_MPI_VI_SetFrmRate(RGB_REC_VI_CHN, 30, 30)) {
        hal_debug("XM_MPI_VI_SetFrmRate failed\r\n");
    }*/
	XM_MPI_VI_EnableChn(RGB_REC_VI_CHN);
    
    pubAttr.u32BgColor = 0;
    pubAttr.enIntfType = VO_INTF_LCD;
    pubAttr.enIntfSync = MIP_VO_INTERFACE;
    XM_MPI_VO_SetPubAttr(0, &pubAttr);

    chnAttr.u32Priority = 0;
    chnAttr.stRect.s32X = 0;
    chnAttr.stRect.s32Y = 0;
    chnAttr.stRect.u32Width = LCD_WIDTH;
    chnAttr.stRect.u32Height = LCD_HEIGHT;
    XM_MPI_VO_SetChnAttr(0, RGB_SHOW_VO_CHN, &chnAttr);

    stSrcChn.enModId = XM_ID_VIU;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = RGB_SHOW_VI_CHN;
    stDestChn.enModId = XM_ID_VOU;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = RGB_SHOW_VO_CHN;
    XM_MPI_SYS_Bind(&stSrcChn, &stDestChn);

    if(COLOR_MODE == mode) {
        XM_MPI_VO_EnableChn(0, RGB_SHOW_VO_CHN);
    } else {
        XM_MPI_VO_DisableChn(0, RGB_SHOW_VO_CHN);
    }

    return 0;
}

static int32_t fdr_isp_init(camera_attr_t *attr)
{
    uint8_t mode = 1; // 0:auto 1:manual
    
    if(NULL == attr) {
        return -1;
    }
    if(30 == attr->fps) {
        system("/var/sd/usr/isp -v n &");
        // hal_info("fps 30\n");
    } else {
        // hal_info("fps 20\n");
        system("/var/sd/usr/isp &");
    }
    sleep(1);

    if(camera_init(NULL)) {
        hal_error("camera_init failed.\r\n");
    }
    
    
    if(camera_set_ae_attr(mode, attr->shut, attr->sagain, attr->sdgain, attr->idgain) != 0) {
        hal_error("ERR: camera_set_ae_attr failed!\n");
        return -1;
    }

#if 1
    reg_write(0x1002009c, 0x84);
    reg_write(0x100200a4, 0x84);
#endif
    return 0;
}

int32_t isp_debug(void)
{
    uint8_t u8Mode = 0; // 0:auto 1:manual
    uint32_t u32SnsShut = 100;
    uint32_t u32SnsAGain = 800;
    uint32_t u32SnsDGain = 800;
    uint32_t u32IspDGain = 100;
    
    if(camera_get_ae_attr(&u8Mode, &u32SnsShut, &u32SnsAGain, &u32SnsDGain, &u32IspDGain) != 0) {
        hal_error("ERR: camera_get_ae_attr failed!\n");
        return -1;
    }
    hal_debug("attr: %d - %u - %u - %u - %u\r\n", u8Mode, u32SnsShut, u32SnsAGain, u32SnsDGain, u32IspDGain);
    return 0;
}

int32_t hal_vio_init(camera_attr_t *attr)
{
    int32_t ret = 0;
    
    ret += XM_MPI_SYS_Init();

    fdr_isp_init(attr);

#if defined(LCD_SIZE_TYPE) && (LCD_SIZE_TYPE == LCD_SIZE_43_INCH)
    /* 4.3 inch lcd*/
    hal_debug("4.3 lcd\r\n");
    reg_write(0x20000000, 1);
    reg_write(0x20000044, 0x2b83);
    reg_write(0x20000000, 0);
#else
    hal_debug("5.0 lcd\r\n");
    /* 5.0 inch lcd */
    reg_write(0x20000000, 1);
	reg_write(0x20000044, 0x1f1f);//0x2b83
	reg_write(0x20000000, 0);
#endif
    ret += _hal_vio_init(COLOR_MODE);/* BLACK_MODE, COLOR_MODE */


    return ret;
}

