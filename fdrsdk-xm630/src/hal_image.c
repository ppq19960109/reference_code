
#include "sys_comm.h"
#include "xm_comm.h"
#include "hal_image.h"
#include "hal_vio.h"
#include "hal_log.h"
#include "hal_fdr.h"
#include "hal_layer.h"
#include "hal_jpeg.h"
#include "zxing_wrapper.h"

typedef struct {
    pthread_t tid;
    uint8_t use;
    uint8_t mode;
    pthread_mutex_t mutex;
    void *vaddr[2];
    VIDEO_FRAME_INFO_S frame[2];
    hal_qrcode_cbfn cb;
    void *cb_hdl;
}hal_image_hdl_t;

static hal_image_hdl_t image_handle;

XM_VOID * XM_MPI_SYS_Mmap_Cached(XM_U32 u32PhyAddr, XM_U32 u32Size);
static int32_t hal_get_image(void **vaddr, VIDEO_FRAME_INFO_S *frame, uint32_t chn, uint32_t tus)
{
    int32_t ret = 0;
    uint32_t count = 1;
    uint32_t now = 0;
    struct timeval tv;
    
    *vaddr = NULL;
    gettimeofday(&tv, NULL);
    now = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
    
    while(count--) {
        ret = XM_MPI_VI_GetFrame(chn, frame);
        if(ret) {
            hal_error("XM_MPI_VI_GetFrame failed, chn = %d.\r\n", chn);
            return -1;
        }
        
        if(frame->stVFrame.u64pts > now || now - frame->stVFrame.u64pts < tus || 0 == count) {
            break;
        }

        ret = XM_MPI_VI_ReleaseFrame(chn, frame);
        if(ret) {
            hal_error("XM_MPI_VI_ReleaseFrame failed, chn = %d, count = %d.\r\n", chn, count);
        }
    }
    //XM_MPI_SYS_Mmap_Cached()  / XM_MPI_SYS_Mmap
    *vaddr = XM_MPI_SYS_Mmap_Cached(frame->stVFrame.u32PhyAddr[0], frame->stVFrame.u32Width * frame->stVFrame.u32Height * 3 / 2);
    if ((*vaddr == NULL)) {
        hal_error("XM_MPI_SYS_Mmap_Cached error\n");
        XM_MPI_VI_ReleaseFrame(chn, frame);
        return -1;
    }
    if(XM_MPI_SYS_MmzFlushCache(frame->stVFrame.u32PhyAddr[0], *vaddr, frame->stVFrame.u32Width * frame->stVFrame.u32Height * 3 / 2)) {
        hal_error("XM_MPI_SYS_MmzFlushCache failed\r\n");
    }

    return 0;
}

static int32_t hal_destroy_img(void *vaddr, VIDEO_FRAME_INFO_S *frame, uint32_t chn)
{
    int32_t ret = 0;

    ret = XM_MPI_SYS_Munmap(vaddr, frame->stVFrame.u32Width * frame->stVFrame.u32Height * 3 / 2);
	if(ret != 0) {
		hal_error("XM_MPI_SYS_Munmap fail ret = %d, chn = %d\r\n", ret, chn);
	}
    ret = XM_MPI_VI_ReleaseFrame(chn, frame);
	if(ret != 0) {
		hal_error("XM_MPI_VI_ReleaseFrame fail ret = %d, chn = %d\r\n", ret, chn);
	}

    return ret;
}

int32_t hal_release_all_frame(void)
{
    pthread_mutex_lock(&image_handle.mutex);
    if(0 == image_handle.use) {
        pthread_mutex_unlock(&image_handle.mutex);
        return -1;
    }
    hal_destroy_img(image_handle.vaddr[0], &image_handle.frame[0], FDR_RGB_REC_VI_CHN);
    hal_destroy_img(image_handle.vaddr[1], &image_handle.frame[1], FDR_IR_REC_VI_CHN);
    image_handle.vaddr[0] = NULL;
    image_handle.vaddr[1] = NULL;
    image_handle.use = 0;
    pthread_mutex_unlock(&image_handle.mutex);
    return 0;
}

static void *hal_image_handle(void *parm)
{
    prctl(PR_SET_NAME,(unsigned long)"hal_image_handle");
    uint8_t mode = 0;
    
    while(1) {
        pthread_mutex_lock(&image_handle.mutex);
        if(image_handle.use) {
            pthread_mutex_unlock(&image_handle.mutex);
            usleep(1000 * 20);
            continue;
        }
        if(hal_get_image(&image_handle.vaddr[0], &image_handle.frame[0], FDR_RGB_REC_VI_CHN, 50000)) {
            pthread_mutex_unlock(&image_handle.mutex);
            usleep(1000 * 20);
            continue;
        }

        if(hal_get_image(&image_handle.vaddr[1], &image_handle.frame[1], FDR_IR_REC_VI_CHN, 50000)) {
            pthread_mutex_unlock(&image_handle.mutex);
            hal_destroy_img(image_handle.vaddr[0], &image_handle.frame[0], FDR_RGB_REC_VI_CHN);
            image_handle.vaddr[0] = NULL;
            usleep(1000 * 20);
            continue;
        }
        image_handle.use = 1;
        mode = image_handle.mode;
        pthread_mutex_unlock(&image_handle.mutex);
        hal_jpeg_play(0, &image_handle.frame[0]);
        hal_jpeg_play(1, &image_handle.frame[1]);
        if(mode == IMG_FACE_MODE) {
            if(hal_face_detect_rec(&image_handle.frame[0], image_handle.vaddr[0], &image_handle.frame[1], image_handle.vaddr[1])) {
                hal_release_all_frame();
            }
        } else {
            /*FIXME: QRCODE MODE*/
            if(image_handle.cb) {
                char qrcode[256] = {0};
                if(0 == qrcode_read(qrcode, 255, image_handle.vaddr[0], 0, 0, 1920, 1080)) {
                    image_handle.cb(image_handle.cb_hdl, qrcode, strlen(qrcode));
                }
            }
            
            hal_release_all_frame();
        }
        
    }

    pthread_exit(NULL);
}

int32_t hal_image_init(void)
{
    VI_EXT_CHN_ATTR_S stExtChnAttr;
	XM_S32 ret = -1;
    
	stExtChnAttr.stDestSize.u32Width = FDR_IMAGE_SIZE_WIDTH;
	stExtChnAttr.stDestSize.u32Height= FDR_IMAGE_SIZE_HEIGHT;
    //stExtChnAttr.enPixFormat= PIXEL_FORMAT_YUV_PLANAR_420;
	ret = XM_MPI_VI_SetExtChnAttr(FDR_RGB_REC_VI_CHN, &stExtChnAttr);
	if(ret != 0) {
		hal_error("XM_MPI_VI_SetExtChnAttr fail ret=%d \n", ret);
		return -1;
	}
	ret = XM_MPI_VI_SetFrmRate(FDR_RGB_REC_VI_CHN, 25, 25);
	if(ret != 0) {
		hal_error("XM_MPI_VI_SetFrmRate fail ret=%d \n", ret);
		return -1;
	}
	ret = XM_MPI_VI_EnableChn(FDR_RGB_REC_VI_CHN);
	if(ret != 0) {
		hal_error("XM_MPI_VI_EnableChn fail ret=%d \n", ret);
		return -1;
	}

    XM_MPI_VI_SetFrmRate(FDR_IR_REC_VI_CHN, 25, 25);
    XM_MPI_VI_EnableChn(FDR_IR_REC_VI_CHN);

    pthread_mutex_init(&image_handle.mutex, NULL);
    
    if(pthread_create(&image_handle.tid, NULL, hal_image_handle, NULL) != 0) {
		hal_error("pthread_create fail\n");
		return -1;
	}

    pthread_detach(image_handle.tid);
	return 0;
}

int32_t hal_image_capture(const char *filename, uint8_t type)
{
    VIDEO_FRAME_INFO_S *frame = &image_handle.frame[0];
    int32_t rc = 0;
    
    if(type != IMG_COLOR_TYPE) {
        frame = &image_handle.frame[1];
    }

    pthread_mutex_lock(&image_handle.mutex);
    if(0 == image_handle.use) {
        pthread_mutex_unlock(&image_handle.mutex);
        return -1;
    }
    rc = hal_frame_to_jpeg(filename, frame);
    pthread_mutex_unlock(&image_handle.mutex);

    return rc;
}

int32_t hal_image_set_mode(uint8_t mode)
{
    if(mode >= IMG_INVALID_MODE) {
        return -1;
    }

    pthread_mutex_lock(&image_handle.mutex);
    image_handle.mode = mode;
    pthread_mutex_unlock(&image_handle.mutex);
    return 0;
}

uint8_t hal_image_get_mode(uint8_t mode)
{
    return image_handle.mode;
}

int32_t hal_set_qrcode_cb(hal_qrcode_cbfn cb, void *handle)
{
    image_handle.cb = cb;
    image_handle.cb_hdl = handle;

    return 0;
}

