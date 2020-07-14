#ifndef __LIGHT_INTERFACE_H__
#define __LIGHT_INTERFACE_H__

#ifdef  __cplusplus
extern "C"{
#endif

#define LIGHT_MAX_RESULT_NUM (50)

#define LIGHT_OK             (0)
#define LIGHT_PARAM_FAIL     (-1)
#define LIGHT_MEM_FAIL       (-2)

/** 矩形结构定义 */
typedef struct tagIaRectSt
{
    int i16X1;        /**< 左上角x坐标 */
    int i16Y1;        /**< 左上角y坐标 */
    int i16X2;        /**< 右下角x坐标 */
    int i16Y2;        /**< 右下角y坐标 */
}IA_RECT_S;

typedef struct tagIaImageSt
{
    unsigned short usWidth;        /* 图像宽度 */
    unsigned short usHeight;       /* 图像高度 */
    unsigned short usBitCnt;       /* 像素Bit数 */

    unsigned char *pucImgData;     /* 图像数据通道Y */
    unsigned char *pucImgData1;    /* 图像数据通道U */
    unsigned char *pucImgData2;    /* 图像数据通道V */
    unsigned short usStride;       /* 图像跨距 */
    unsigned int uiReserved[16];   /* 保留字节 */
}IA_IMAGE_S;

typedef struct tagIaLightInterfaceSt
{
    /* 输入图像的信息 */
    int iImgWid;   /* 输入图像宽度,以像素点为单位 */
    int iImgHgt;   /* 输入图像高度,以像素点为单位 */
    int iStride;   /* 输入图像跨距,以像素点为单位若无左右扩边,设置值等同于iImgWid */

    int iBitCnt;   /* 每个像素点所占的bit位数, 和采集系统位数相关,8bit的RGB图像设置为24 */

    int iResver[3];
} IA_LIGHT_INTERFACE_S;

/* 感兴趣区域配置结构体 */
typedef struct tagIaLightSetSt
{
    IA_RECT_S stAreaSet;        /* 检测区域 */
    IA_IMAGE_S stImage;         /* 输入图像首地址 */
} IA_LIGHT_AREA_S;


/* 结果保存结构体 */
typedef struct tagIaLightDetSt
{
    int iCenterX;
    int iCenterY;
	int iWidth;
	int iHeight;
} IA_LIGHT_DET_S;

/* 结果保存结构体 */
typedef struct tagIaLightRsltSt
{
    int iCount;      /* 亮点个数 */
    IA_LIGHT_DET_S astDetRslt[LIGHT_MAX_RESULT_NUM];
} IA_LIGHT_RSLT_S;

char * IA_Light_GetVersion();
int IA_Light_Init(IA_LIGHT_INTERFACE_S *pstLightProperty, void** ppvLight);
int IA_Light_Reset(IA_LIGHT_INTERFACE_S *pstLightProperty, void* ppvLight);
int IA_Light_Work(void* pvLight, IA_LIGHT_AREA_S* pstAllRect, IA_LIGHT_RSLT_S *pstRecResult);
int IA_Light_Destroy(void **ppvLight);

#ifdef  __cplusplus
}
#endif /* end of __cplusplus */

#endif
