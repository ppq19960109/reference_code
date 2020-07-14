
#include "sys_comm.h"
#include "xm_comm.h"
#include "hal_jpeg.h"
#include "hal_log.h"
#include "hal_vio.h"


typedef struct {
    uint32_t phyaddr;
    uint8_t *viraddr;
    pthread_mutex_t mutex;
    hal_jpeg_render_t render;
}hal_jpeg_handle_t;

static hal_jpeg_handle_t jpeg_handle;


static int32_t hal_save_jpeg(const char *filename, VENC_STREAM_S *pstStream)
{
    FILE *fp = NULL;
    uint32_t i = 0;
    int32_t ret = -1;

    fp = fopen(filename, "w+");
    if(NULL == fp) {
        hal_error("fopen %s failed, %s\r\n", filename, strerror(errno));
        return -1;
    }

    ret = 0;
    //hal_debug("pkg num: %d\r\n", pstStream->u32PackCount);
    for (i = 0; i < pstStream->u32PackCount; i++) {
        //hal_debug("write size %d\r\n", pstStream->pstPack[i].u32Len);
        if(pstStream->pstPack[i].u32Len != fwrite(pstStream->pstPack[i].pu8Addr, 1, pstStream->pstPack[i].u32Len, fp)) {
            hal_error("fwrite %s failed, %s\r\n", filename, strerror(errno));
            ret = -1;
            break;
        }
        fflush(fp);
    }

    fclose(fp);
    return ret;
}

int32_t hal_frame_to_jpeg(const char *filename, void *frame)
{
    JPEG_ENC_ATTR_S attr;
    VENC_STREAM_S stream;
    VENC_PACK_S pstPack;
    VIDEO_FRAME_INFO_S *p = frame;
    int32_t ret = -1;

    if(0 == jpeg_handle.phyaddr) {
        return -1;
    }
    pstPack.pu8Addr = jpeg_handle.viraddr;
    pstPack.u32PhyAddr = jpeg_handle.phyaddr;
    pstPack.u32Len = FDR_IMAGE_SIZE_WIDTH * FDR_IMAGE_SIZE_HEIGHT * 3 / 4;

    stream.pstPack = &pstPack;
    attr.lumWidthSrc = p->stVFrame.u32Width;
    attr.lumHeightSrc = p->stVFrame.u32Height;
    attr.horOffsetSrc = 0;
    attr.verOffsetSrc = 0;
    attr.dstWidth = p->stVFrame.u32Width;
    attr.dstHeight = p->stVFrame.u32Height;

    pthread_mutex_lock(&jpeg_handle.mutex);
    ret = XM_MPI_VENC_EncodeJpeg(p, &attr, &stream);
    pthread_mutex_unlock(&jpeg_handle.mutex);
    if(ret) {
        hal_error("XM_MPI_VENC_EncodeJpeg failed, ret = %x.\r\n", ret);
        return -1;
    }

    if(hal_save_jpeg(filename, &stream)) {
        hal_error("hal_save_jpeg failed.\r\n");
        return -1;
    }
    
    return 0;
}

int32_t hal_jpeg_play(uint32_t idx, void *frame)
{
    JPEG_ENC_ATTR_S attr;
    VENC_STREAM_S stream;
    VENC_PACK_S pstPack;
    VIDEO_FRAME_INFO_S *p = frame;
    int32_t ret = -1;

    if(idx >= INVALID_INDEX || 0 == jpeg_handle.phyaddr || NULL == jpeg_handle.render) {
        return -1;
    }
    pstPack.pu8Addr = jpeg_handle.viraddr;
    pstPack.u32PhyAddr = jpeg_handle.phyaddr;
    pstPack.u32Len = FDR_IMAGE_SIZE_WIDTH * FDR_IMAGE_SIZE_HEIGHT * 3 / 4;

    stream.pstPack = &pstPack;
    attr.lumWidthSrc = p->stVFrame.u32Width;
    attr.lumHeightSrc = p->stVFrame.u32Height;
    attr.horOffsetSrc = 0;
    attr.verOffsetSrc = 0;
    attr.dstWidth = p->stVFrame.u32Width;
    attr.dstHeight = p->stVFrame.u32Height;

    pthread_mutex_lock(&jpeg_handle.mutex);
    ret = XM_MPI_VENC_EncodeJpeg(p, &attr, &stream);
    pthread_mutex_unlock(&jpeg_handle.mutex);
    if(ret) {
        hal_error("XM_MPI_VENC_EncodeJpeg failed, ret = %x.\r\n", ret);
        return -1;
    }
    jpeg_handle.render(idx, stream.pstPack[0].pu8Addr, stream.pstPack[0].u32Len);
    return 0;
}

void hal_jpeg_regist_render(hal_jpeg_render_t render)
{
    jpeg_handle.render = render;
}


static int32_t _hal_jpeg_init(void)
{
    VENC_CHN_ATTR_S stVencChnAttr;
    VENC_ATTR_JPEG_S stJpegAttr;
	VENC_CHN jpegChn = 0;
    int32_t ret = -1;

    stVencChnAttr.stVeAttr.enType = PT_JPEG;
    stJpegAttr.u32PicWidth  = FDR_IMAGE_SIZE_WIDTH;
    stJpegAttr.u32PicHeight = FDR_IMAGE_SIZE_HEIGHT;
    stJpegAttr.u32MaxPicWidth= FDR_IMAGE_SIZE_WIDTH;
    stJpegAttr.u32MaxPicHeight= FDR_IMAGE_SIZE_HEIGHT;
    stJpegAttr.u32BufSize = FDR_IMAGE_SIZE_WIDTH * FDR_IMAGE_SIZE_HEIGHT * 3 / 4;
    stJpegAttr.bByFrame = XM_FALSE;
    stJpegAttr.bSupportDCF = XM_FALSE;
    memcpy(&stVencChnAttr.stVeAttr.stAttrJpeg, &stJpegAttr, sizeof(VENC_ATTR_JPEG_S));

    ret = XM_MPI_VENC_CreateChn(jpegChn, &stVencChnAttr);
    if (XM_SUCCESS != ret) {
        hal_error("XM_MPI_VENC_CreateChn [%d] faild with %#x!\r\n", jpegChn, ret);
        return -1;
    }

    ret = XM_MPI_VENC_StartRecvPic(jpegChn);
    if (XM_SUCCESS != ret) {
        hal_error("XM_MPI_VENC_StartRecvPic [%d] faild with %#x!\r\n", jpegChn, ret);
        return -1;
    }
    
    return 0;
}

int32_t hal_jpeg_init(void)
{
    uint32_t size = FDR_IMAGE_SIZE_WIDTH * FDR_IMAGE_SIZE_HEIGHT * 3 / 4;
    pthread_mutex_init(&jpeg_handle.mutex, NULL);
    if( 0 == jpeg_handle.phyaddr) {
        if(XM_MPI_SYS_MmzAlloc(&jpeg_handle.phyaddr, (void **)&jpeg_handle.viraddr, "jpeg", NULL, size)) {
            hal_error("XM_MPI_SYS_MmzAlloc failed, size = %d\r\n", size);
            return -1;
        }
    }
    
    if(_hal_jpeg_init()) {
        hal_error("_hal_jpeg_init failed.\r\n");
        return -1;
    }
    
    return 0;
}

int32_t hal_jpeg_destroy(void)
{
    VENC_CHN jpegChn = 0;
    if(XM_MPI_SYS_MmzFree(jpeg_handle.phyaddr, jpeg_handle.viraddr)) {
		hal_error("XM_MPI_SYS_MmzFree failed\r\n");
	}
    if (XM_MPI_VENC_DestroyChn(jpegChn)) {
        hal_error("XM_MPI_VENC_CreateChn [%d] faild!\r\n", jpegChn);
    }

    jpeg_handle.phyaddr = 0;
    jpeg_handle.viraddr = NULL;
    return 0;
}

