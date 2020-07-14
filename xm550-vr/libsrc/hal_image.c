

#include "hal_sdk.h"
#include "hal_image.h"
#include "hal_vio.h"
#include "hal_log.h"
#include "hal_jpeg.h"
#include "hal_timer.h"

typedef struct {
    pthread_t tid;
    uint8_t use[2];
    pthread_mutex_t mutex[2];
    void *vaddr[2];
    VIDEO_FRAME_INFO_S frame[2];
    hal_image_dispatch_t dispatch;
}hal_image_t;

static hal_image_t image_handle;

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
    *vaddr = XM_MPI_SYS_Mmap(frame->stVFrame.u32PhyAddr[0], frame->stVFrame.u32Width * frame->stVFrame.u32Height * 3 / 2);
    if ((*vaddr == NULL)) {
        hal_error("XM_MPI_SYS_Mmap error\n");
        XM_MPI_VI_ReleaseFrame(chn, frame);
        return -1;
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

int32_t hal_image_release(uint32_t idx)
{
    if(idx > 1) {
        return -1;
    }

    pthread_mutex_lock(&image_handle.mutex[idx]);
    if(0 == image_handle.use[idx]) {
        pthread_mutex_unlock(&image_handle.mutex[idx]);
        return -1;
    }
    hal_destroy_img(image_handle.vaddr[idx], &image_handle.frame[idx], RGB_REC_VI_CHN);
    image_handle.vaddr[idx] = NULL;
    image_handle.use[idx] = 0;
    pthread_mutex_unlock(&image_handle.mutex[idx]);
#ifdef LOG_DEBUG
    hal_info("release image idx: %d, TM: %u\r\n", idx, hal_time_get_ms());
#endif
    return 0;
}

static void *hal_image_handle(void *parm)
{
    uint32_t idx = 0;
    prctl(PR_SET_NAME,(unsigned long)"image_handle");
    
    while(1) {
        pthread_mutex_lock(&image_handle.mutex[idx]);
        if(image_handle.use[idx]) {
            pthread_mutex_unlock(&image_handle.mutex[idx]);
            usleep(10);
            continue;
        }
        if(hal_get_image(&image_handle.vaddr[idx], &image_handle.frame[idx], RGB_REC_VI_CHN, 50000)) {
            pthread_mutex_unlock(&image_handle.mutex[idx]);
            usleep(10);
            continue;
        }
        image_handle.use[idx] = 1;
        pthread_mutex_unlock(&image_handle.mutex[idx]);
#ifdef LOG_DEBUG
        hal_info("get image idx: %d, TM: %u\r\n", idx, hal_time_get_ms());
#endif
        if(image_handle.dispatch.dispatch) {
            image_handle.dispatch.dispatch(image_handle.dispatch.handle, idx, image_handle.vaddr[idx], 
                image_handle.frame[idx].stVFrame.u32Width, image_handle.frame[idx].stVFrame.u32Height);
        } else {
            hal_image_release(idx);
        }
        
        idx = idx ? 0 : 1;
    }

    pthread_exit(NULL);
}

int32_t hal_image_capture(uint32_t idx, const char *filename)
{
    VIDEO_FRAME_INFO_S *frame = NULL;
    int32_t rc = 0;
    
    if(idx > 1) {
        return -1;
    }
    frame = &image_handle.frame[idx];
    pthread_mutex_lock(&image_handle.mutex[idx]);
    if(0 == image_handle.use[idx]) {
        pthread_mutex_unlock(&image_handle.mutex[idx]);
        return -1;
    }
    rc = hal_frame_to_jpeg(filename, frame);
    pthread_mutex_unlock(&image_handle.mutex[idx]);

    return rc;
}

int32_t hal_image_regist_dispatch(hal_image_dispatch_t *dispatch)
{
    if(NULL == dispatch) {
        return -1;
    }
    image_handle.dispatch.handle = dispatch->handle;
    image_handle.dispatch.dispatch = dispatch->dispatch;
    return 0;
}

int32_t hal_image_init(void)
{
    pthread_mutex_init(&image_handle.mutex[0], NULL);
    pthread_mutex_init(&image_handle.mutex[1], NULL);
    if(pthread_create(&image_handle.tid, NULL, hal_image_handle, NULL) != 0) {
		hal_error("pthread_create fail\n");
		return -1;
	}
    pthread_detach(image_handle.tid);
	return 0;
}

