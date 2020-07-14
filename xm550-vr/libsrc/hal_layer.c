
#include "hal_sdk.h"
#include "hal_layer.h"
#include "hal_log.h"
#include "hal_vio.h"



static void *layer_vaddr = NULL;
static uint32_t layer_phyaddr = 0;


int32_t hal_clear_layer(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    uint32_t i = 0;
    uint16_t *vaddr = layer_vaddr;

    if(x + w > LCD_WIDTH || y + h > LCD_HEIGHT) {
        return -1;
    }
    for(i = y; i < y + h; i++) {
        memset(vaddr + ((LCD_HEIGHT - 1 - i) * LCD_WIDTH + x), 0, sizeof(uint16_t) * w);
    }
    return 0;
}

int32_t hal_layer_set_rect_corner(hal_rect_t *rect, uint32_t thickness, uint32_t size, uint16_t color)
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t pos = 0;
    uint32_t x = rect->x;
    uint32_t y = rect->y;
    uint32_t w = rect->w;
    uint32_t h = rect->h;
    uint16_t *image = layer_vaddr;

    if(NULL == layer_vaddr) {
        hal_warn("layer need init.\r\n");
        return -1;
    }

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

int32_t hal_layer_enable(void)
{
    return XM_MPI_VO_EnableImageLayer(0);
}

int32_t hal_layer_disable(void)
{
    return XM_MPI_VO_DisableImageLayer(0);
}

int32_t hal_layer_init(void)
{
    void *vaddr = NULL;
    uint32_t phyaddr = 0;
    uint32_t size = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t);
    int32_t ret = 0;
    VO_IMAGE_LAYER_ATTR_S attr;

    
    
	ret = XM_MPI_SYS_MmzAlloc(&phyaddr, &vaddr, "layer", NULL, size);
    if(ret != 0 || NULL == vaddr) {
		hal_error("XM_MPI_SYS_MmzAlloc failed %d\r\n", ret);
		return -1;
	}
    memset(vaddr, 0 ,size);
    layer_vaddr = vaddr;
    layer_phyaddr = phyaddr;
    attr.stDispRect.s32X = 0;
	attr.stDispRect.s32Y = 0;
	attr.stDispRect.u32Width = LCD_WIDTH;
	attr.stDispRect.u32Height = LCD_HEIGHT;
	attr.enPixFormat = PIXEL_FORMAT_RGB_1555;
	attr.u32Effect = 0xff00ff | (0xff << 8); //0x000000FFFFFF, 0xff80ff
	attr.u32PhyAddr = layer_phyaddr;
	XM_MPI_VO_SetImageLayerAttr(0, &attr);
    hal_layer_disable();
    return 0;
}

int32_t hal_layer_destroy(void)
{
    void *vaddr = layer_vaddr;
    uint32_t phyaddr = layer_phyaddr;
    int32_t ret = 0;
    
    if(hal_layer_disable()) {
        hal_error("hal_layer_disable failed.\r\n");
    }

    ret = XM_MPI_SYS_MmzFree(phyaddr, vaddr);
    if(ret != 0) {
		hal_error("XM_MPI_SYS_MmzAlloc failed %d\r\n", ret);
	}
    layer_vaddr = NULL;
    layer_phyaddr = 0;

    return 0;
}


