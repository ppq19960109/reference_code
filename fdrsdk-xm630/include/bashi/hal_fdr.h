
#ifndef _HAL_FDR_H_
#define _HAL_FDR_H_

#include "stdint.h"
#include "xm_comm_video.h"




typedef enum {
    FACE_DETECT_REC_MODE_E,
    FACE_EXTRACT_FEATURE_E
}fdr_work_mode_t;

/*
* @brief: get face info callback.
* @parm[in] feature: face feature
* @parm[out] info: face info
*/
typedef int32_t (*fdr_face_match_cb)(void *handle, void *rfs, uint32_t rfn, void *img);



int32_t hal_face_detect_rec(VIDEO_FRAME_INFO_S *im_frame, void *rgb_img, VIDEO_FRAME_INFO_S *ir_frame, void *ir_img);

int32_t hal_get_face_feature(void *frame, uint32_t width, uint32_t height, float *feature);

int32_t hal_set_fdr_cb(fdr_face_match_cb func, void *handle);

int32_t hal_fdr_config(uint32_t mode);

int32_t hal_fdr_init(uint32_t mode);

int32_t hal_fdr_destroy(void);

int32_t hal_fdr_enable(void);

int32_t hal_fdr_disable(void);


#endif

