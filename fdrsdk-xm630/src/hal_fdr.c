
#include "sys_comm.h"
#include "xm_comm.h"
#include "smart_fdr.h"
#include "smart_comm.h"
#include "hal_fdr.h"
#include "hal_log.h"
#include "hal_vio.h"
#include "hal_led.h"
#include "hal_timer.h"
#include "board.h"


#define FACE_REC_NAME_LENGTH    128
#define FACE_FEATURE_LEN        (FACE_REC_NAME_LENGTH + sizeof(XM_IA_FR_FEATRUES))
//#define FEATURE_PATH            "/var/sd/feature_list.txt"
#define FEATURE_PATH            "./feature_list.txt"

#define FACE_REC_CLEAR_THRESHOLD    92

typedef struct {
    void *hdl;
    fdr_face_match_cb cb;
    void *cb_hdl;
    uint8_t init;
    uint8_t mode;
    uint8_t enable;
    pthread_mutex_t mutex;
}fdr_handle_t;

static fdr_handle_t fdr_handle;



int32_t hal_release_all_frame(void);
static int32_t hal_face_handle(XM_IA_FDR_RESULTS_S *stRslts, void *rgb_img)
{
    uint32_t i = 0;
    
    if(stRslts->stFdResult.iFaceNum > 0) {
        hal_led_on_duration(2000);
    }
/*
    if(stRslts->stFdResult.iFaceNum || stRslts->stFrResult.iFaceNum) {
        hal_debug("fdr num: %d - %d\r\n", stRslts->stFdResult.iFaceNum, stRslts->stFrResult.iFaceNum);
        hal_debug("fdr id: %d - %d\r\n", stRslts->stFdResult.astFaceRect[0].iFaceId, stRslts->stFrResult.astFaceRecInfo[0].iFaceId);
    }
*/
  
    for(i = 0; i < stRslts->stFdResult.iFaceNum; i++) {
        hal_debug("face detect idx: %d, faceid: %d, Score: %f, Clearness: %f, alive: %d, alivescore: %d\r\n",
                i, stRslts->stFdResult.astFaceRect[i].iFaceId,
                stRslts->stFdResult.astFaceRect[i].fScore, stRslts->stFdResult.astFaceRect[i].fClearness,
                stRslts->stFdResult.astFaceRect[i].iAliveFace, stRslts->stFdResult.astFaceRect[i].iAliveScore);
    }
    for(i = 0; i < stRslts->stFrResult.iFaceNum; i++) {
        hal_debug("face rec idx: %d, faceid: %d\r\n", i, stRslts->stFrResult.astFaceRecInfo[i].iFaceId);
    }
    if(stRslts->stFdResult.iFaceNum)  {
        printf("\r\n");
    }
    if(stRslts->stFrResult.iFaceNum == 0) {
		return -1;
	}
    
    if(fdr_handle.cb) {
        if(fdr_handle.cb(fdr_handle.cb_hdl, &(stRslts->stFrResult.astFaceRecInfo[0]), stRslts->stFrResult.iFaceNum, NULL)) {
            hal_error("face match failed\r\n");
            return -1;
        }
    } else {
        hal_release_all_frame();
    }

    return 0;
}


int32_t hal_face_detect_rec(VIDEO_FRAME_INFO_S *im_frame, void *rgb_img, VIDEO_FRAME_INFO_S *ir_frame, void *ir_img)
{
    static uint8_t first_img = 1;
    XM_IA_IMAGE_S stImg;
	XM_IA_IMAGE_S stImg_bw;
	XM_IA_FDR_IN_S stFrmIn;
    XM_IA_FDR_RESULTS_S stRslts;

    pthread_mutex_lock(&fdr_handle.mutex);
    if(0 == fdr_handle.init || fdr_handle.mode != FACE_DETECT_REC_MODE_E || !fdr_handle.enable) {
        pthread_mutex_unlock(&fdr_handle.mutex);
        return -1;
    }
    
    memset(&stImg, 0, sizeof(stImg));
    memset(&stImg_bw, 0, sizeof(stImg_bw));
    memset(&stFrmIn, 0, sizeof(stFrmIn));
    memset(&stRslts, 0, sizeof(XM_IA_FDR_RESULTS_S));
    
    stFrmIn.ucIsFirstFrm = first_img;
    stFrmIn.eFdrMode = XM_IA_FDR_MODE_FDR;
    
    stImg.usWidth = im_frame->stVFrame.u32Width;
    stImg.usHeight = im_frame->stVFrame.u32Height;
    stImg.usBitCnt = 8;
    stImg.eImgType = XM_IMG_TYPE_NV12;
    stImg.pvImgData = rgb_img;
    stImg.pvImgData1 = NULL;
    stImg.pvImgData2 = NULL;
    stImg.u32PhyAddr = im_frame->stVFrame.u32PhyAddr[0];
    stImg.u32PhyAddr1 = 0;
    stImg.u32PhyAddr2 = 0;
    stImg.usStride = im_frame->stVFrame.u32Width;
    stImg.usStride1 = 0;
    stImg.usStride2 = 0;
    stFrmIn.pstImg[0] = &stImg;

    stImg_bw.usWidth = ir_frame->stVFrame.u32Width;
    stImg_bw.usHeight = ir_frame->stVFrame.u32Height;
    stImg_bw.usBitCnt = 8;		
    stImg_bw.eImgType = XM_IMG_TYPE_NV12;
    stImg_bw.pvImgData = ir_img;
    stImg_bw.pvImgData1 = NULL;
    stImg_bw.pvImgData2 = NULL;
    stImg_bw.u32PhyAddr = ir_frame->stVFrame.u32PhyAddr[0];	
    stImg_bw.u32PhyAddr1 = 0;
    stImg_bw.u32PhyAddr2 = 0;
    stImg_bw.usStride = ir_frame->stVFrame.u32Width;
    stImg_bw.usStride1 = 0;
    stImg_bw.usStride2 = 0;
    stFrmIn.pstImg[1]= &stImg_bw;

    if(XM_IA_Run(fdr_handle.hdl, &stFrmIn, &stRslts)) {
        pthread_mutex_unlock(&fdr_handle.mutex);
        hal_error("XM_IA_Run failed.\r\n");
        return -1;
    }
    
    pthread_mutex_unlock(&fdr_handle.mutex);
    if(first_img) {
        first_img = 0;
    }
    return hal_face_handle(&stRslts, im_frame);
}

int32_t hal_get_face_feature(void *frame, uint32_t width, uint32_t height, float *feature)
{
    XM_IA_IMAGE_S stImg;
    XM_IA_FDR_IN_S stFrmIn;
    XM_IA_FDR_RESULTS_S stRslts;

    pthread_mutex_lock(&fdr_handle.mutex);
    if(0 == fdr_handle.init || fdr_handle.mode != FACE_EXTRACT_FEATURE_E) {
        pthread_mutex_unlock(&fdr_handle.mutex);
        return -1;
    }

    hal_debug("w = %d, h = %d\r\n", width, height);
    memset(&stImg, 0, sizeof(stImg));
    memset(&stFrmIn, 0, sizeof(stFrmIn));
    memset(&stRslts, 0, sizeof(XM_IA_FDR_RESULTS_S));
    
    stImg.usWidth = width;
    stImg.usHeight = height;
    stImg.usBitCnt = 8;		
    stImg.eImgType = XM_IMG_TYPE_NV12;
    stImg.pvImgData = frame;
    stImg.pvImgData1 = NULL;
    stImg.pvImgData2 = NULL;
    stImg.u32PhyAddr = (uint32_t)frame;
    stImg.u32PhyAddr1 = 0;
    stImg.u32PhyAddr2 = 0;
    stImg.usStride = ((width + 15) >> 4) << 4;
    stImg.usStride1 = 0;
    stImg.usStride2 = 0;
    stFrmIn.pstImg[0] = &stImg;
    stFrmIn.eFdrMode = XM_IA_FDR_MODE_EXTRACT_FEATURE;

    if(XM_IA_Run(fdr_handle.hdl, &stFrmIn, &stRslts)) {
        pthread_mutex_unlock(&fdr_handle.mutex);
        hal_error("XM_IA_Run failed.\r\n");
        return -1;
    }
    pthread_mutex_unlock(&fdr_handle.mutex);
    
    if (stRslts.stFrResult.iFaceNum != 1) {
        hal_error("face num = %d\r\n", stRslts.stFrResult.iFaceNum);
        return -1;
    }
    
    memcpy(feature, stRslts.stFrResult.astFaceRecInfo[0].stFrFeatures.afFaceFeat, FACE_REC_FEATURE_NUM * sizeof(float));

    return 0;
}

int32_t hal_fdr_config(uint32_t mode)
{
    int32_t ret = 0;
    XM_IA_FDR_CONFIG_S config;
    pthread_mutex_lock(&fdr_handle.mutex);
    if(mode == fdr_handle.mode) {
        goto end;
    }

    config.stFaceDetConfig.iFaceMinWid = 910;
	config.stFaceDetConfig.iFaceMaxWid = 5*910;
    config.stFaceRecConfig.iClearThr = 0;
	config.stFaceRecConfig.stFaceRecRegion.s16X1 = 0;
    config.stFaceRecConfig.stFaceRecRegion.s16Y1 = 0;
    config.stFaceRecConfig.stFaceRecRegion.s16X2 = 8191;
    config.stFaceRecConfig.stFaceRecRegion.s16Y2 = 8191;
    
    fdr_handle.mode = (mode == FACE_EXTRACT_FEATURE_E) ? FACE_EXTRACT_FEATURE_E : FACE_DETECT_REC_MODE_E;
    if(fdr_handle.mode == FACE_EXTRACT_FEATURE_E) {
        config.eAlgSense = IA_SENSE_HIGH;
        config.eRotate = XM_IA_ROTATE_NO;
    } else {
        config.eAlgSense = IA_SENSE_MID;
        config.eRotate = XM_IA_ROTATE_CLOCKWISE;
        config.stFaceRecConfig.iClearThr = FACE_REC_CLEAR_THRESHOLD;
    }
    ret = XM_IA_Configuration(&config, fdr_handle.hdl);
    if (ret != 0) {
        hal_error("XM_IA_Configuration fail, mode = %d", mode);
    }

end:
    pthread_mutex_unlock(&fdr_handle.mutex);
    return ret;
}

static int32_t _hal_fdr_init(void)
{
    XM_IA_TYPE_E eIAtype = XM_FACE_DET_REC_IA; /*XM_FACE_DET_SSH_IA*/
    XM_IA_FDR_INIT_S stIAInit;
    VI_CHN_ATTR_S stAttr;
    
    XM_MPI_VI_GetChnAttr(0, &stAttr);

    hal_debug("AI mode: %d, width: %d, height: %d\r\n", fdr_handle.mode, stAttr.stCapRect.u32Width, stAttr.stCapRect.u32Height);
    memset(&stIAInit, 0, sizeof(XM_IA_FDR_INIT_S));
    snprintf(stIAInit.acClassBinPath, XM_IA_PATH_MAX_LEN, FACE_CLASSIFIER_PATH);
	stIAInit.uiInStructSize = sizeof(XM_IA_FDR_IN_S);
	stIAInit.uiInitStructSize = sizeof(XM_IA_FDR_INIT_S);
	stIAInit.uiConfigStructSize = sizeof(XM_IA_FDR_CONFIG_S);
	stIAInit.uiRsltStructSize = sizeof(XM_IA_FDR_RESULTS_S);
	stIAInit.eDevImgType = XM_IMG_TYPE_NV12;
    stIAInit.ePlatForm = XM_IA_PLATFORM_XM550;
    stIAInit.eFaceAngleMode = XM_IA_FP_MODE_ACTIVE;
	stIAInit.stDevImgSize.s16X = stAttr.stCapRect.u32Width;
	stIAInit.stDevImgSize.s16Y = stAttr.stCapRect.u32Height;
	stIAInit.stFdrConfig.stFaceDetConfig.iFaceMinWid = 910;
	stIAInit.stFdrConfig.stFaceDetConfig.iFaceMaxWid = 5*910;
    stIAInit.stFdrConfig.stFaceRecConfig.iClearThr = FACE_REC_CLEAR_THRESHOLD;
	stIAInit.stFdrConfig.stFaceRecConfig.stFaceRecRegion.s16X1 = 0;
    stIAInit.stFdrConfig.stFaceRecConfig.stFaceRecRegion.s16Y1 = 0;
    stIAInit.stFdrConfig.stFaceRecConfig.stFaceRecRegion.s16X2 = 8191;
    stIAInit.stFdrConfig.stFaceRecConfig.stFaceRecRegion.s16Y2 = 8191;
    if(fdr_handle.mode == FACE_EXTRACT_FEATURE_E) {
        stIAInit.iRotateMode = 0;
        stIAInit.stFdrConfig.eAlgSense = IA_SENSE_HIGH;
        stIAInit.stFdrConfig.eRotate = XM_IA_ROTATE_NO;
    	stIAInit.eInfraredMode = XM_IA_ALIVE_MODE_INFRARED_INACTIVE; /*XM_IA_ALIVE_MODE_INFRARED_ACTIVE, XM_IA_ALIVE_MODE_INFRARED_INACTIVE*/
    } else {
        stIAInit.iRotateMode = 1;
        stIAInit.stFdrConfig.eAlgSense = IA_SENSE_HIGH;
        stIAInit.stFdrConfig.eRotate = XM_IA_ROTATE_CLOCKWISE;
    	stIAInit.eInfraredMode = XM_IA_ALIVE_MODE_INFRARED_ACTIVE; /*XM_IA_ALIVE_MODE_INFRARED_ACTIVE, XM_IA_ALIVE_MODE_INFRARED_INACTIVE*/
    }

	if (XM_IA_Create(eIAtype, (void *)&stIAInit, &(fdr_handle.hdl))) {
		hal_error("XM_IA_Create failed.\r\n");
		return -1;
	}
    hal_debug("XM_IA_Create ok\r\n");
    return 0;
}

int32_t hal_fdr_init(uint32_t mode)
{
    static uint8_t init = 0;
    if(0 == init) {
        pthread_mutex_init(&fdr_handle.mutex, NULL);
        init = 1;
    }
    pthread_mutex_lock(&fdr_handle.mutex);
    if(fdr_handle.init) {
        pthread_mutex_unlock(&fdr_handle.mutex);
        hal_error("algrithm had been init, please destroy first.\r\n");
        return -1;
    }

    fdr_handle.mode = (mode == FACE_EXTRACT_FEATURE_E) ? mode : FACE_DETECT_REC_MODE_E;
    if(_hal_fdr_init()) {
        pthread_mutex_unlock(&fdr_handle.mutex);
        return -1;
    }
    
    fdr_handle.enable = 1;
    fdr_handle.init = 1;
    pthread_mutex_unlock(&fdr_handle.mutex);
    return 0;
}

int32_t hal_set_fdr_cb(fdr_face_match_cb func, void *handle)
{
    fdr_handle.cb = func;
    fdr_handle.cb_hdl = handle;
    return 0;
}

int32_t hal_fdr_destroy(void)
{
    pthread_mutex_lock(&fdr_handle.mutex);
    if(XM_IA_Destruction(fdr_handle.hdl)) {
        pthread_mutex_unlock(&fdr_handle.mutex);
        hal_error("XM_IA_Destruction failed\r\n");
        return -1;
    }
    fdr_handle.hdl = NULL;
    fdr_handle.init = 0;
    pthread_mutex_unlock(&fdr_handle.mutex);
    hal_debug("XM_IA_Destruction ok\r\n");
    return 0;
}

int32_t hal_fdr_enable(void)
{
    pthread_mutex_lock(&fdr_handle.mutex);
    fdr_handle.enable = 1;
    pthread_mutex_unlock(&fdr_handle.mutex);

    return 0;
}

int32_t hal_fdr_disable(void)
{
    pthread_mutex_lock(&fdr_handle.mutex);
    fdr_handle.enable = 0;
    pthread_mutex_unlock(&fdr_handle.mutex);

    return 0;
}

