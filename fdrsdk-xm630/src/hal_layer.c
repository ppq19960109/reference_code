
#include "sys_comm.h"
#include "xm_comm.h"
#include "hal_osd.h"
#include "hal_layer.h"
#include "hal_log.h"
#include "hal_vio.h"
#include "bitmap_rotate.h"
#include "board.h"

#define OSD_RGN_HANDLE      2
#define OSD_TIME_HANDLE     OSD_RGN_HANDLE + 1
#define OSD_ONLINE_HANDLE   OSD_RGN_HANDLE + 2
#define OSD_IPADDR_HANDLE   OSD_RGN_HANDLE + 3

#define WINDOW_LINE_HANDLE  10
#define WINDOW_OSD_HANDLE  11


typedef struct {
    uint32_t align;//1 - right position, 0 - left postision
    uint32_t hdl;
    RECT_S rect;
}hal_osd_attr_t;


static void *g_pic_vaddr = NULL;
static uint32_t g_pic_phyaddr = 0;

static hal_osd_attr_t g_osd_attr[MAX_OSD_LAYER_E] = {
                                    {1, OSD_TIME_HANDLE, {760, 50, 1, 1}},
                                    {1, OSD_ONLINE_HANDLE, {15, 20, 1, 1}},
                                    {0, OSD_IPADDR_HANDLE, {755, 300, 1, 1}}};



int32_t hal_window_enable(void)
{
    return XM_MPI_RGN_Enable(FDR_RGB_SHOW_VI_CHN, WINDOW_LINE_HANDLE, COVER_RGN);
}

int32_t hal_window_disable(void)
{
    return XM_MPI_RGN_Disable(FDR_RGB_SHOW_VI_CHN, WINDOW_LINE_HANDLE, COVER_RGN);
}

static int32_t hal_window_init(void)
{
	RGN_ATTR_S stRgnAttr;
	int ret = 0;
	stRgnAttr.enType = COVER_RGN;
	stRgnAttr.u32Handle = WINDOW_LINE_HANDLE;  //从5开始 最大31
	stRgnAttr.unAttr.stCover.u32Color = 0x1dff6b;
	stRgnAttr.unAttr.stCover.u32Effect = 0; //0空心  1实心
	stRgnAttr.unAttr.stCover.stRect.s32X = 200;
	stRgnAttr.unAttr.stCover.stRect.s32Y = 40;
	stRgnAttr.unAttr.stCover.stRect.u32Width = 400;
	stRgnAttr.unAttr.stCover.stRect.u32Height = 400;
	ret = XM_MPI_RGN_Create(FDR_RGB_SHOW_VI_CHN, &stRgnAttr);
	if (ret != 0) {
		hal_error("XM_MPI_RGN_Create u32Handle=%d err 0x%x\n", stRgnAttr.u32Handle, ret);
		return XM_FAILURE;
	}
	return hal_window_enable();
}

static int32_t hal_window_destroy(void)
{
	RGN_ATTR_S stRgnAttr;
	int ret = 0;

    ret = hal_window_disable();
    if (ret != 0) {
		hal_error("hal_window_disable failed\r\n");
	}
    
	stRgnAttr.enType = COVER_RGN;
	stRgnAttr.u32Handle = WINDOW_LINE_HANDLE;
	ret = XM_MPI_RGN_Destroy(FDR_RGB_SHOW_VI_CHN, &stRgnAttr);
	if (ret != 0) {
		hal_error("XM_MPI_RGN_Destroy u32Handle=%d err 0x%x\n", stRgnAttr.u32Handle, ret);
		return XM_FAILURE;
	}
	return ret;
}

int32_t hal_osd_window_enable(void)
{
    return XM_MPI_RGN_Enable(FDR_RGB_SHOW_VI_CHN, WINDOW_OSD_HANDLE, COVER_RGN);
}

int32_t hal_osd_window_disable(void)
{
    return XM_MPI_RGN_Disable(FDR_RGB_SHOW_VI_CHN, WINDOW_OSD_HANDLE, COVER_RGN);
}

static int32_t hal_osd_window_init(void)
{
	RGN_ATTR_S stRgnAttr;
	int ret = 0;
	stRgnAttr.enType = COVER_RGN;
	stRgnAttr.u32Handle = WINDOW_OSD_HANDLE;  //从5开始 最大31
	stRgnAttr.unAttr.stCover.u32Color = 0x1dff6b;
	stRgnAttr.unAttr.stCover.u32Effect = 1; //0空心  1实心
	stRgnAttr.unAttr.stCover.stRect.s32X = 730;
	stRgnAttr.unAttr.stCover.stRect.s32Y = 0;
	stRgnAttr.unAttr.stCover.stRect.u32Width = 70;
	stRgnAttr.unAttr.stCover.stRect.u32Height = 480;
	ret = XM_MPI_RGN_Create(FDR_RGB_SHOW_VI_CHN, &stRgnAttr);
	if (ret != 0) {
		hal_error("XM_MPI_RGN_Create u32Handle=%d err 0x%x\n", stRgnAttr.u32Handle, ret);
		return XM_FAILURE;
	}
	return XM_MPI_RGN_Enable(FDR_RGB_SHOW_VI_CHN, WINDOW_OSD_HANDLE, COVER_RGN);
}

static int32_t hal_osd_window_destroy(void)
{
	RGN_ATTR_S stRgnAttr;
	int ret = 0;

    ret = hal_window_disable();
    if (ret != 0) {
		hal_error("hal_window_disable failed\r\n");
	}
    
	stRgnAttr.enType = COVER_RGN;
	stRgnAttr.u32Handle = WINDOW_OSD_HANDLE;
	ret = XM_MPI_RGN_Destroy(FDR_RGB_SHOW_VI_CHN, &stRgnAttr);
	if (ret != 0) {
		hal_error("XM_MPI_RGN_Destroy u32Handle=%d err 0x%x\n", stRgnAttr.u32Handle, ret);
		return XM_FAILURE;
	}
	return ret;
}


#if 0
int32_t osd_bitmap_show(const BIT_MAP_INFO *map_info)
{
     /*for debug*/
    uint32_t x;
    uint32_t y;
    uint8_t i;
    uint32_t offset = 0;
    
    printf("bitmap: \r\n");
    for(y = 0; y < map_info->height; y++) {
        for(x = 0; x < map_info->width / 8; x++) {
            offset = y * (map_info->width / 8) + x;
            for(i = 0; i < 8; i++) {
                printf("%d ", (map_info->raster[offset] >> (7 - i)) & 0x01);
            }
        }
        printf("\r\n");
    }

    return 0;
}
#endif

static int32_t _hal_osd_init(uint32_t layer);
static int32_t _hal_osd_destroy(uint32_t layer);
static int32_t hal_osd_rotate(BIT_MAP_INFO *map_info)
{
    uint32_t x;
    uint32_t y;
    uint32_t w = map_info->width;
    uint32_t h = map_info->height;
    uint8_t *new_raster = (unsigned char*)malloc(w * h / 8);
    uint8_t *bitmap1 = (unsigned char*)malloc(w * h);
    uint8_t *bitmap2 = (unsigned char*)malloc(w * h);
    uint8_t i;
    uint32_t offset = 0;
    
    
    if(NULL == new_raster || NULL == bitmap1 || NULL == bitmap2) {
        free(new_raster);
        free(bitmap1);
        free(bitmap2);
        hal_error("malloc failed\r\n");
        return -1;
    }

    //step1: raster to bitmap
    //hal_debug("step1...\r\n");
    for(y = 0; y < h; y++) {
        for(x = 0; x < w / 8; x++) {
            offset = y * (w / 8) + x;
            for(i = 0; i < 8; i++) {
                bitmap1[offset * 8 + i] = (map_info->raster[offset] >> (7 - i)) & 0x01;
            }
        }
    }
    //bitmap_show(bitmap1, w, h);

    //step2: rotate
    //hal_debug("step2...\r\n");
    bitmap_rotate(bitmap1, bitmap2, w, h, LEFT_ROTATE);
    //bitmap_show(bitmap2, h, w);

    map_info->width = h;
    map_info->height = w;
    w = map_info->width;
    h = map_info->height;

    //step3: bitmap to new raster
    //hal_debug("step3...\r\n");
    for(y = 0; y < h; y++) {
        for(x = 0; x < w / 8; x++) {
            offset = y * w / 8 + x;
            new_raster[offset] = 0;
            for(i = 0; i < 8; i++) {
                new_raster[offset] |= (bitmap2[offset * 8 + i] ? 1 : 0) << (7 - i);
            }
        }
    }

    free(map_info->raster);
    map_info->raster = new_raster;
    free(bitmap1);
    free(bitmap2);
    //hal_debug("rotate ok\r\n");
    return 0;
}

/*
@brief: osd show function
@param[in] align: 0 - left align, other right align
*/
static int32_t hal_osd_show_v2(char *str, uint8_t rgn_hdl, uint32_t x, uint32_t y, uint8_t align)
{
    BIT_MAP_INFO map_info;
    BITMAP_S bitmap;
	RGN_ATTR_S rgn_attr;
	int32_t ret = -1;

	map_info.raster = NULL;
	ret = GetBitMap(str, &map_info);
    
	if(ret != 0) {
		hal_error("GetBitMap err %d\r\n", ret);
        return -1;
	}
	if(map_info.raster == NULL) {
		hal_error("map_info.raster is NULL\r\n");
        return -1;
	}

    
    //osd_bitmap_show(&map_info);
    hal_debug("w * h: [%d *  %d]\r\n",map_info.width, map_info.height);
    if(hal_osd_rotate(&map_info)) {
        hal_error("hal_osd_rotate failed\r\n");
        goto end;
    }
    //hal_debug("w * h: [%d *  %d]\r\n",map_info.width, map_info.height);
    //osd_bitmap_show(&map_info);

    rgn_attr.enType = OVERLAY_RGN;
	rgn_attr.u32Handle = rgn_hdl;
	rgn_attr.unAttr.stOverlay.u32ColorMap = 0xF6;
	rgn_attr.unAttr.stOverlay.u32Effect = 0x00000000;
	rgn_attr.unAttr.stOverlay.u32Format = 0;
	rgn_attr.unAttr.stOverlay.stRect.s32X = x;
	rgn_attr.unAttr.stOverlay.stRect.s32Y = align ? y : ((y > map_info.height + 20) ? (y - map_info.height) : 20);
	rgn_attr.unAttr.stOverlay.stRect.u32Width = map_info.width;
	rgn_attr.unAttr.stOverlay.stRect.u32Height = map_info.height;
	ret = XM_MPI_RGN_Create(FDR_RGB_SHOW_VI_CHN, &rgn_attr);
	if (ret != 0) {
		hal_error("XM_MPI_RGN_Create rgn_hdl = %d, err %d\r\n", rgn_hdl, ret);
        goto end;
	}


    bitmap.u32Handle = rgn_hdl;
	bitmap.u32Width= map_info.width;
	bitmap.u32Height = map_info.height;
	bitmap.u32Format = 0;
	bitmap.pData = map_info.raster;
	ret = XM_MPI_RGN_SetBitMap(FDR_RGB_SHOW_VI_CHN, &bitmap);
	if (ret != 0) {
		hal_error("xM_MPI_RGN_SetBitMap err %d\r\n", ret);
        goto end;
	}

    ret = XM_MPI_RGN_Enable(FDR_RGB_SHOW_VI_CHN, rgn_hdl, OVERLAY_RGN);
    if (ret != 0) {
		hal_error("XM_MPI_RGN_Enable failed %d\r\n", ret);
        goto end;
	}

    ret = 0;
end:
    free(map_info.raster);
    return ret;
}

int32_t hal_osd_show_test(void)
{
    char tmstr[36] = {0};
    time_t t = time(NULL);
    struct tm tm;

    setenv("TZ", "GMT-8", 1);
    localtime_r(&t, &tm);
    sprintf(tmstr, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    hal_osd_show_v2(tmstr, OSD_TIME_HANDLE, 20, 20, 1);
    return 0;
}

int32_t hal_osd_show(uint32_t layer, char *str)
{
    BIT_MAP_INFO map_info;
    BITMAP_S bitmap;
	int32_t ret = -1;

	map_info.raster = NULL;
	ret = GetBitMap(str, &map_info);
    
	if(ret != 0) {
		hal_error("GetBitMap err %d\r\n", ret);
        return -1;
	}
	if(map_info.raster == NULL) {
		hal_error("map_info.raster is NULL\r\n");
        return -1;
	}

    //osd_bitmap_show(&map_info);
    if(map_info.width != g_osd_attr[layer].rect.u32Width
        || map_info.height != g_osd_attr[layer].rect.u32Height) {
        hal_debug("w * h: [%d *  %d]\r\n",map_info.width, map_info.height);
        g_osd_attr[layer].rect.u32Width = map_info.width;
        g_osd_attr[layer].rect.u32Height = map_info.height;
        _hal_osd_destroy(layer);
        _hal_osd_init(layer);
    }

    if(hal_osd_rotate(&map_info)) {
        hal_error("hal_osd_rotate failed\r\n");
        goto end;
    }
 
    bitmap.u32Handle = g_osd_attr[layer].hdl;
	bitmap.u32Width= map_info.width;
	bitmap.u32Height = map_info.height;
	bitmap.u32Format = 0;
	bitmap.pData = map_info.raster;
	ret = XM_MPI_RGN_SetBitMap(FDR_RGB_SHOW_VI_CHN, &bitmap);
	if (ret != 0) {
		hal_error("xM_MPI_RGN_SetBitMap err %d\r\n", ret);
        goto end;
	}
    XM_MPI_RGN_Enable(FDR_RGB_SHOW_VI_CHN, g_osd_attr[layer].hdl, OVERLAY_RGN);
end:
    free(map_info.raster);
    return ret;
}

int32_t hal_osd_close(uint32_t layer)
{
    return XM_MPI_RGN_Disable(FDR_RGB_SHOW_VI_CHN, g_osd_attr[layer].hdl, OVERLAY_RGN);
}

/*
struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};
*/
int32_t _hal_osd_show_time(void)
{
    char tmstr[36] = {0};
    time_t t = time(NULL);
    struct tm tm;

    setenv("TZ", "GMT-8", 1);
    localtime_r(&t, &tm);
    //sprintf(tmstr, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    sprintf(tmstr, "%02d:%02d", tm.tm_hour, tm.tm_min);
    return hal_osd_show(OSD_TIME_LAYER_E, tmstr);
}

static int32_t hal_osd_close_show_time(void)
{
    if(XM_MPI_RGN_Disable(FDR_RGB_SHOW_VI_CHN, g_osd_attr[OSD_TIME_LAYER_E].hdl, OVERLAY_RGN)) {
        hal_error("XM_MPI_RGN_Disable failed\r\n");
        return -1;
    }
    return 0;
}

int32_t hal_osd_show_time(uint8_t enable)
{
    if(enable) {
        return _hal_osd_show_time();
    }

    return hal_osd_close_show_time();
}


int32_t hal_osd_show_online(uint8_t online)
{
    return hal_osd_show(OSD_ONLINE_LAYER_E, online ? "online" : "offline");
}

int32_t hal_osd_close_online(void)
{
    return hal_osd_close(OSD_ONLINE_LAYER_E);
}

static int32_t _hal_osd_init(uint32_t layer)
{
    int32_t ret;
    RGN_ATTR_S rgn_attr;
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
    
    if(layer >= MAX_OSD_LAYER_E) {
        return -1;
    }
    
    rgn_attr.enType = OVERLAY_RGN;
	rgn_attr.unAttr.stOverlay.u32ColorMap = 0xF6;
	rgn_attr.unAttr.stOverlay.u32Effect = 0x00000000;
	rgn_attr.unAttr.stOverlay.u32Format = 0;

    x = g_osd_attr[layer].rect.s32X;
    y = g_osd_attr[layer].rect.s32Y;
    w = g_osd_attr[layer].rect.u32Height;
    h = g_osd_attr[layer].rect.u32Width;
    rgn_attr.u32Handle = g_osd_attr[layer].hdl;
    rgn_attr.unAttr.stOverlay.stRect.s32X = x;
	rgn_attr.unAttr.stOverlay.stRect.s32Y = g_osd_attr[layer].align ? y : ((y > h + 20) ? (y - h) : 20);
	rgn_attr.unAttr.stOverlay.stRect.u32Width = w;
	rgn_attr.unAttr.stOverlay.stRect.u32Height = h;
    ret = XM_MPI_RGN_Create(FDR_RGB_SHOW_VI_CHN, &rgn_attr);
	if (ret != 0) {
		hal_error("XM_MPI_RGN_Create rgn_hdl = %d, err %d\r\n", g_osd_attr[layer].hdl, ret);
	}
    ret = XM_MPI_RGN_Disable(FDR_RGB_SHOW_VI_CHN, g_osd_attr[layer].hdl, OVERLAY_RGN);
    if (ret != 0) {
		hal_error("XM_MPI_RGN_Enable failed %d\r\n", ret);
	}

    return ret;
}
static int32_t hal_osd_init(void)
{
    osdInit(FONT_PATH);

    uint32_t i = 0;
    int32_t ret = 0;
    
    for(i = 0; i < MAX_OSD_LAYER_E; i++) {
        _hal_osd_init(i);
    }
    //hal_osd_show_time(1);
    //hal_osd_show_company();
    return ret;
}

static int32_t _hal_osd_destroy(uint32_t layer)
{
    int32_t ret = 0;
    RGN_ATTR_S rgn_attr;

    rgn_attr.enType = OVERLAY_RGN;
    if(layer >= MAX_OSD_LAYER_E) {
        return -1;
    }

    ret = XM_MPI_RGN_Disable(FDR_RGB_SHOW_VI_CHN, g_osd_attr[layer].hdl, OVERLAY_RGN);
    if (ret != 0) {
		hal_error("XM_MPI_RGN_Disable failed %d\r\n", ret);
	}
    rgn_attr.u32Handle = g_osd_attr[layer].hdl;
    ret = XM_MPI_RGN_Destroy(FDR_RGB_SHOW_VI_CHN, &rgn_attr);
	if (ret != 0) {
		hal_error("XM_MPI_RGN_Destroy failed\r\n");
	}
    
    return ret;
}

static int32_t hal_osd_destroy(void)
{
    uint32_t i = 0;
    int32_t ret = 0;

    for(i = 0; i < MAX_OSD_LAYER_E; i++) {
        _hal_osd_destroy(i);
    }
    
    return ret;
}


int32_t hal_bmp_show(char *path, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint8_t flag, uint8_t alpha)
{
    FILE *fp = NULL;
    uint32_t size = 0;
    VO_IMAGE_LAYER_ATTR_S layer;
    uint32_t i = 0;
    uint16_t *vaddr = g_pic_vaddr;

    fp = fopen(path, "rb");
	if(fp == NULL) {
		hal_error("open %s failed, %s.\r\n", path, strerror(errno));
		return -1;
	}
    fseek(fp, 54, SEEK_CUR);
    size = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t);
    memset(vaddr, 0 ,size);
    #if 1
    if(flag) {
        fseek(fp, (y1 * LCD_WIDTH) * 2, SEEK_CUR);
    }
    //hal_debug("pos = %d\r\n", ftell(fp));
    for(i = y1; i < y2; i++) {
        if(flag) {
            fseek(fp,  x1 * 2, SEEK_CUR);
        }
        //hal_debug("pos = %d - %d\r\n", ftell(fp), (i * LCD_WIDTH + x1) * 2 + 54);
		fread(vaddr + ((LCD_HEIGHT - 1 - i) * LCD_WIDTH + x1), sizeof(uint16_t), x2 - x1, fp);
        if(flag) {
            fseek(fp, (LCD_WIDTH - x2) * 2, SEEK_CUR);
        }
	}
    #else
    for(i = 0; i < LCD_HEIGHT; i++) {
		fread(vaddr + (LCD_HEIGHT - 1 - i) * LCD_WIDTH, sizeof(uint16_t), LCD_WIDTH, fp);
	}
    #endif
    fclose(fp);
	layer.stDispRect.s32X = 0;
	layer.stDispRect.s32Y = 0;
	layer.stDispRect.u32Width = LCD_WIDTH;
	layer.stDispRect.u32Height = LCD_HEIGHT;
	layer.enPixFormat = PIXEL_FORMAT_RGB_1555;
	layer.u32Effect = 0xff00ff | (alpha << 8); //0x000000FFFFFF, 0xff80ff
	layer.u32PhyAddr = g_pic_phyaddr;
	XM_MPI_VO_SetImageLayerAttr(0, &layer);
    return 0;
}

int32_t hal_bmp_set_rect_corner(hal_rect_corner_t *rect, uint32_t thickness, uint32_t size, uint16_t color)
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t pos = 0;
    uint32_t x = rect->x;
    uint32_t y = rect->y;
    uint32_t w = rect->w;
    uint32_t h = rect->h;
    uint16_t *image = g_pic_vaddr;

    if(thickness >=  size || x + w > LCD_WIDTH || y + h > LCD_HEIGHT) {
        hal_error("invalid rect parm\r\n");
        return -1;
    }
    if(size > w) {
        size = w - 1;
    }
    if(size > h) {
        size = h - 1;
    }
    if( 0 == size) {
        size = 1;
    }
    
    for(i = y; i < y + h; i++) {
        if((i > y + size - 1) && (i < y + (h - size))) {
            continue;
        }
        for(j = x; j < x + w; j++) {
            if((j > x + size - 1) && (j < x + (w - size))) {
                continue;
            }
            if((i > y + thickness - 1) && (i < y + (h - thickness))
                 && (j > x + thickness -1) && (j < x + (w - thickness))) {
                continue;
            }
            pos = i * LCD_WIDTH + j;
            image[pos] = color;
        }
    }
    
    return 0;
}

int32_t hal_bmp_set_layer(char *bitmap, uint32_t x1, uint32_t y1, uint32_t w, uint32_t h, uint8_t alpha)
{
    VO_IMAGE_LAYER_ATTR_S layer;
    uint32_t i = 0;
    uint16_t *vaddr = g_pic_vaddr;
    uint32_t x2 = x1 + w;
    uint32_t y2 = y1 + h;
    if(x2 > LCD_WIDTH || y2 > LCD_HEIGHT) {
        return -1;
    }

    memset(vaddr, 0 , LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t));

    for(i = 0; i < h; i++) {
		memcpy(vaddr + ((LCD_HEIGHT - 1 - y1 - i) * LCD_WIDTH + x1), bitmap, sizeof(uint16_t) * w);
        bitmap += sizeof(uint16_t) * w;
	}

	layer.stDispRect.s32X = 0;
	layer.stDispRect.s32Y = 0;
	layer.stDispRect.u32Width = LCD_WIDTH;
	layer.stDispRect.u32Height = LCD_HEIGHT;
	layer.enPixFormat = PIXEL_FORMAT_RGB_1555;
	layer.u32Effect = 0xff00ff | (alpha << 8); //0x000000FFFFFF, 0xff80ff
	layer.u32PhyAddr = g_pic_phyaddr;
	XM_MPI_VO_SetImageLayerAttr(0, &layer);
    return 0;
}

int32_t hal_bmp_set_layer1(char *bitmap, uint32_t x1, uint32_t y1, uint32_t w, uint32_t h, uint8_t alpha)
{
    VO_IMAGE_LAYER_ATTR_S layer;
    uint32_t i = 0;
    uint16_t *vaddr = g_pic_vaddr;
    uint32_t x2 = x1 + w;
    uint32_t y2 = y1 + h;
    if(x2 > LCD_WIDTH || y2 > LCD_HEIGHT) {
        return -1;
    }

    for(i = 0; i < h; i++) {
		memcpy(vaddr + ((y1 + i) * LCD_WIDTH + x1), bitmap, sizeof(uint16_t) * w);
        bitmap += sizeof(uint16_t) * w;
	}

	layer.stDispRect.s32X = 0;
	layer.stDispRect.s32Y = 0;
	layer.stDispRect.u32Width = LCD_WIDTH;
	layer.stDispRect.u32Height = LCD_HEIGHT;
	layer.enPixFormat = PIXEL_FORMAT_RGB_1555;
	layer.u32Effect = 0xff00ff | (alpha << 8); //0x000000FFFFFF, 0xff80ff
	layer.u32PhyAddr = g_pic_phyaddr;
	XM_MPI_VO_SetImageLayerAttr(0, &layer);
    return 0;
}


int32_t hal_bmp_clear_layer(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    uint32_t i = 0;
    uint16_t *vaddr = g_pic_vaddr;

    if(x + w > LCD_WIDTH || y + h > LCD_HEIGHT) {
        return -1;
    }
    for(i = y; i < y + h; i++) {
        memset(vaddr + ((LCD_HEIGHT - 1 - i) * LCD_WIDTH + x), 0, sizeof(uint16_t) * w);
    }
    return 0;
}

/*
static void bmp_show_test(char *path){
    FILE    *fp;
    size_t  sz;
    void    *bmp;

    fp =  fopen(path, "rb");
    if(fp == NULL){
        hal_warn("open user image fail:%s\n", path);
        return ;
    }
    
    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    hal_debug("bmp size = %d\r\n", sz);
    bmp = malloc(sz);
    if(bmp == NULL){
        hal_warn("alloc image buffer fail!\n");
        fclose(fp);
        return ;
    }

    if(fread(bmp, 1, sz, fp) != sz){
        hal_warn("read user image fail!\n");
        fclose(fp);
        return ;
    }
    fclose(fp);

    hal_bmp_set_layer(bmp + 54, 640, 0, 160, 480, 0x80);
    free(bmp);
}*/

int32_t hal_bmp_layer_enable(void)
{
    return XM_MPI_VO_EnableImageLayer(0);
}

int32_t hal_bmp_layer_disable(void)
{
    return XM_MPI_VO_DisableImageLayer(0);
}

static int32_t hal_bmp_init(void)
{
    void *vaddr = NULL;
    uint32_t phyaddr = 0;
    uint32_t size = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t);
    int32_t ret = 0;
    
	ret = XM_MPI_SYS_MmzAlloc(&phyaddr, &vaddr, "layer", NULL, size);
    if(ret != 0 || NULL == vaddr) {
		hal_error("XM_MPI_SYS_MmzAlloc failed %d\r\n", ret);
		return -1;
	}
    memset(vaddr, 0 ,size);
    g_pic_vaddr = vaddr;
    g_pic_phyaddr = phyaddr;
    hal_bmp_layer_enable();
    return 0;
}

static int32_t hal_bmp_destroy(void)
{
    void *vaddr = g_pic_vaddr;
    uint32_t phyaddr = g_pic_phyaddr;
    int32_t ret = 0;
    
    if(hal_bmp_layer_disable()) {
        hal_error("hal_bmp_layer_disable failed.\r\n");
    }

    ret = XM_MPI_SYS_MmzFree(phyaddr, vaddr);
    if(ret != 0) {
		hal_error("XM_MPI_SYS_MmzAlloc failed %d\r\n", ret);
	}
    g_pic_vaddr = NULL;
    g_pic_phyaddr = 0;

    return 0;
}

int32_t hal_layer_init(void)
{
    hal_window_init();
    
    hal_osd_window_init();
    
    hal_bmp_init();

    hal_osd_init();

    return 0;
}

int32_t hal_layer_destory(void)
{
    hal_window_destroy();
    
    hal_osd_window_destroy();
    
    hal_bmp_destroy();

    hal_osd_destroy();
    
    return 0;
}

