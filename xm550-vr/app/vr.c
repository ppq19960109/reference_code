
#include "hal_vio.h"
#include "hal_sdk.h"
#include "hal_types.h"
#include "hal_log.h"
#include "hal_image.h"
#include "hal_timer.h"
#include "hal_layer.h"
#include "cJSON.h"

#include "vr.h"
#include "udp_server.h"
#include "uart.h"

#define VR_RECT_THICK 3
#define VR_RECT_SIZE 20
#define VR_RECT_SIZE_HALF (VR_RECT_SIZE / 2)

typedef struct
{
    pthread_t tid;
    void *vr_light_handle;
    uint32_t state;
    uint32_t snapshot;
    uint8_t picname[64];
    pthread_mutex_t mutex;
    uint32_t onuse;
    uint32_t idx;
    uint8_t *image;
    uint32_t w;
    uint32_t h;
} vr_handle_t;

static vr_handle_t vr_handle;

#if 1
// static void vr_result_show(IA_LIGHT_RSLT_S *rt)
// {
//     uint32_t i = 0;
//     hal_rect_t rect;
//     uint32_t sz = 0;

//     hal_clear_layer(0, 0, LCD_WIDTH, LCD_HEIGHT);
//     for (i = 0; i < rt->iCount; i++)
//     {
//         rect.x = rt->astDetRslt[i].iCenterX * LCD_WIDTH / IMAGE_SIZE_WIDTH;
//         rect.y = rt->astDetRslt[i].iCenterY * LCD_HEIGHT / IMAGE_SIZE_HEIGHT;

//         rect.w = rt->astDetRslt[i].iWidth * LCD_WIDTH / IMAGE_SIZE_WIDTH;
//         rect.h = rt->astDetRslt[i].iHeight * LCD_HEIGHT / IMAGE_SIZE_HEIGHT;
//         //rect.x = (rect.x + rect.w < LCD_WIDTH) ? (rect.x + rect.w) : LCD_WIDTH;
//         //rect.y = (rect.y + rect.h < LCD_HEIGHT) ? (rect.y + rect.h) : LCD_HEIGHT;
//         rect.x = (rect.x > rect.w) ? (rect.x - rect.w) : 0;
//         rect.y = (rect.y + rect.h) ? (rect.y - rect.h) : 0;
//         sz = (rect.w >= rect.h) ? rect.w * 2 : rect.h * 2;
//         rect.w = sz;
//         rect.h = sz;
//         hal_layer_set_rect_corner(&rect, VR_RECT_THICK, sz, 0x0195);
//         //hal_info("RECT: [%d, %d, %d, %d]\r\n", rect.x, rect.y, rect.w, rect.h);
//     }
// }
#else
static void vr_result_show(IA_LIGHT_RSLT_S *rt)
{
    uint32_t i = 0;
    hal_rect_t rect;

    rect.w = VR_RECT_SIZE;
    rect.h = VR_RECT_SIZE;

    hal_clear_layer(0, 0, LCD_WIDTH, LCD_HEIGHT);
    for (i = 0; i < rt->iCount; i++)
    {
        rect.x = rt->astDetRslt[i].iCenterX * LCD_WIDTH / IMAGE_SIZE_WIDTH;
        rect.y = rt->astDetRslt[i].iCenterY * LCD_HEIGHT / IMAGE_SIZE_HEIGHT;

        if (rect.x < VR_RECT_SIZE_HALF)
        {
            rect.x = VR_RECT_SIZE_HALF;
        }
        else if (rect.x + VR_RECT_SIZE_HALF >= LCD_WIDTH)
        {
            rect.x = LCD_WIDTH - VR_RECT_SIZE_HALF - 1;
        }

        if (rect.y < VR_RECT_SIZE_HALF)
        {
            rect.y = VR_RECT_SIZE_HALF;
        }
        else if (rect.y + VR_RECT_SIZE_HALF >= LCD_HEIGHT)
        {
            rect.y = LCD_HEIGHT - VR_RECT_SIZE_HALF - 1;
        }

        rect.x -= VR_RECT_SIZE_HALF;
        rect.y -= VR_RECT_SIZE_HALF;
        hal_layer_set_rect_corner(&rect, VR_RECT_THICK, VR_RECT_SIZE, 0x0195);
        //hal_info("RECT: [%d, %d, %d, %d]\r\n", rect.x, rect.y, rect.w, rect.h);
    }
}
#endif

// static void vr_result_log(IA_LIGHT_RSLT_S *rt, uint32_t cost)
// {
//     uint32_t i = 0;
//     cJSON *root = cJSON_CreateObject();
//     cJSON *array = cJSON_CreateArray();
//     char *log = NULL;
//     cJSON *item = NULL;

//     cJSON_AddItemToObject(root, "timestamp", cJSON_CreateNumber(hal_time_get_ms()));
//     cJSON_AddItemToObject(root, "costms", cJSON_CreateNumber(cost));
//     cJSON_AddItemToObject(root, "num", cJSON_CreateNumber(rt->iCount));
//     for (i = 0; i < rt->iCount; i++)
//     {
//         item = cJSON_CreateObject();
//         cJSON_AddItemToObject(item, "x", cJSON_CreateNumber(rt->astDetRslt[i].iCenterX));
//         cJSON_AddItemToObject(item, "y", cJSON_CreateNumber(rt->astDetRslt[i].iCenterY));
//         cJSON_AddItemToObject(item, "w", cJSON_CreateNumber(rt->astDetRslt[i].iWidth));
//         cJSON_AddItemToObject(item, "h", cJSON_CreateNumber(rt->astDetRslt[i].iHeight));
//         cJSON_AddItemToArray(array, item);
//     }
//     cJSON_AddItemToObject(root, "targets", array);

//     log = cJSON_PrintUnformatted(root);
//     cJSON_Delete(root);
//     if (NULL != log)
//     {
//         // udp_report_log((uint8_t *)log, strlen(log));
//         free(log);
//     }
// }

static void vr_image_proc(uint8_t *image, uint32_t w, uint32_t h)
{
    IA_LIGHT_AREA_S area;
    IA_LIGHT_RSLT_S rt;

    // uint32_t start = 0;
    // uint32_t end = 0;
    area.stAreaSet.i16X1 = 0;
    area.stAreaSet.i16Y1 = 0;
    area.stAreaSet.i16X2 = w;
    area.stAreaSet.i16Y2 = h;
    area.stImage.pucImgData = image;
    area.stImage.usBitCnt = 8;
    area.stImage.usHeight = h;
    area.stImage.usWidth = w;
    area.stImage.usStride = w;

    if (IA_Light_Work(vr_handle.vr_light_handle, &area, &rt))
    {
        hal_error("IA_Light_Work failed\r\n");
        goto fail;
        return;
    }
    // start = hal_time_get_ms();
    int32_t ret=send_light_position(&rt);
    // end = hal_time_get_ms();
    if(ret==0)
    {
        // hal_info("time:%d ms\n",end-start);
    }
    return;
fail:
    hal_layer_disable();
    return;
}

static void vr_image_dispatch(void *handle, uint32_t idx, uint8_t *image, uint32_t w, uint32_t h)
{
    while (vr_handle.onuse)
    {
        usleep(1);
    }
    pthread_mutex_lock(&vr_handle.mutex);
    vr_handle.idx = idx;
    vr_handle.image = image;
    vr_handle.w = w;
    vr_handle.h = h;
    vr_handle.onuse = 1;
    pthread_mutex_unlock(&vr_handle.mutex);
}

static void *vr_proc(void *parm)
{
    while (1)
    {
        if (0 == vr_handle.onuse)
        {
            usleep(1);
            continue;
        }
        pthread_mutex_lock(&vr_handle.mutex);
        vr_image_proc(vr_handle.image, vr_handle.w, vr_handle.h);
#ifdef LOG_DEBUG
        hal_debug("idx: %d, TM: %u\r\n", vr_handle.idx, hal_time_get_ms());
#endif
        if (vr_handle.snapshot)
        {
            hal_image_capture(vr_handle.idx, (char *)vr_handle.picname);
            vr_handle.snapshot = 0;
        }
        hal_image_release(vr_handle.idx);
        vr_handle.onuse = 0;
        pthread_mutex_unlock(&vr_handle.mutex);
    }
    pthread_exit(NULL);
}

static int32_t _vr_init(void)
{
    IA_LIGHT_INTERFACE_S iface;

    iface.iBitCnt = 8;
    iface.iImgHgt = IMAGE_SIZE_HEIGHT;
    iface.iImgWid = IMAGE_SIZE_WIDTH;
    iface.iStride = IMAGE_SIZE_WIDTH;

    if (IA_Light_Init(&iface, &vr_handle.vr_light_handle))
    {
        hal_error("ir light init failed\r\n");
        return -1;
    }
    return 0;
}

int32_t vr_set_state(uint32_t state)
{
    if (state > VR_DEBUG_E)
    {
        return -1;
    }

    vr_handle.state = state;
    return 0;
}

int32_t vr_snapshot(const char *path, char *filename)
{
    if (NULL == path || NULL == filename)
    {
        return -1;
    }

    pthread_mutex_lock(&vr_handle.mutex);
    snprintf((char *)vr_handle.picname, 64 - 1, "%s/%s", path, filename);
    vr_handle.snapshot = 1;
    pthread_mutex_unlock(&vr_handle.mutex);
    return 0;
}

int32_t vr_init(void)
{
    hal_image_dispatch_t dispatch;

    if (_vr_init())
    {
        return -1;
    }
    vr_set_state(VR_LOG_E);

    dispatch.dispatch = vr_image_dispatch;
    dispatch.handle = NULL;
    hal_image_regist_dispatch(&dispatch);

    pthread_mutex_init(&vr_handle.mutex, NULL);
    if (pthread_create(&vr_handle.tid, NULL, vr_proc, NULL) != 0)
    {
        hal_error("pthread_create fail\n");
        return -1;
    }
    hal_image_init();
    return 0;
}
