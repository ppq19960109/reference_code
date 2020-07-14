

#include "hal_sdk.h"
#include "hal_jpeg.h"
#include "hal_log.h"
#include "hal_vio.h"

typedef struct {
    uint32_t phyaddr;
    uint8_t *viraddr;
}hal_jpeg_mem_hdl_t;

static hal_jpeg_mem_hdl_t g_jpeg_mem;

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

    if(0 == g_jpeg_mem.phyaddr) {
        return -1;
    }
    pstPack.pu8Addr = g_jpeg_mem.viraddr;
    pstPack.u32PhyAddr = g_jpeg_mem.phyaddr;
    pstPack.u32Len = IMAGE_SIZE_WIDTH * IMAGE_SIZE_HEIGHT * 3 / 4;

    stream.pstPack = &pstPack;
    attr.lumWidthSrc = p->stVFrame.u32Width;
    attr.lumHeightSrc = p->stVFrame.u32Height;
    attr.horOffsetSrc = 0;
    attr.verOffsetSrc = 0;
    attr.dstWidth = p->stVFrame.u32Width;
    attr.dstHeight = p->stVFrame.u32Height;

    ret = XM_MPI_VENC_EncodeJpeg(p, &attr, &stream);
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

static int32_t _hal_jpeg_init(void)
{
    VENC_CHN_ATTR_S stVencChnAttr;
    VENC_ATTR_JPEG_S stJpegAttr;
	VENC_CHN jpegChn = 0;
    int32_t ret = -1;

    stVencChnAttr.stVeAttr.enType = PT_JPEG;
    stJpegAttr.u32PicWidth  = IMAGE_SIZE_WIDTH;
    stJpegAttr.u32PicHeight = IMAGE_SIZE_HEIGHT;
    stJpegAttr.u32MaxPicWidth= IMAGE_SIZE_WIDTH;
    stJpegAttr.u32MaxPicHeight= IMAGE_SIZE_HEIGHT;
    stJpegAttr.u32BufSize = IMAGE_SIZE_WIDTH * IMAGE_SIZE_HEIGHT * 3 / 4;
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
    uint32_t size = IMAGE_SIZE_WIDTH * IMAGE_SIZE_HEIGHT * 3 / 4;
    if( 0 == g_jpeg_mem.phyaddr) {
        if(XM_MPI_SYS_MmzAlloc(&g_jpeg_mem.phyaddr, (void **)&g_jpeg_mem.viraddr, "jpeg", NULL, size)) {
            hal_error("XM_MPI_SYS_MmzAlloc failed, size = %d\r\n", size);
            return -1;
        }
    }
    
    if(_hal_jpeg_init()) {
        hal_error("hal_jpeg_init failed.\r\n");
        return -1;
    }
    
    return 0;
}

int32_t hal_jpeg_destroy(void)
{
    VENC_CHN jpegChn = 0;
    if(XM_MPI_SYS_MmzFree(g_jpeg_mem.phyaddr, g_jpeg_mem.viraddr)) {
		hal_error("XM_MPI_SYS_MmzFree failed\r\n");
	}
    if (XM_MPI_VENC_DestroyChn(jpegChn)) {
        hal_error("XM_MPI_VENC_CreateChn [%d] faild!\r\n", jpegChn);
    }

    g_jpeg_mem.phyaddr = 0;
    g_jpeg_mem.viraddr = NULL;
    return 0;
}

