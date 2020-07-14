/*****************************************************************************

------------------------------------------------------------------------------
                            xm_ia_comm.h
产品名:    智能分析算法
模块名:    公共文件
生成日期:  2018-06-11
作者:     王则浪
文件描述:  智能分析对外公共头文件
*****************************************************************************/
#ifndef _XM_IA_COMM_H_
#define _XM_IA_COMM_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* 雄迈智能分析模块错误返回码 */
#define XM_IA_SUCCESS          (0)
#define XM_IA_MALLOC_FAIL      (1)
#define XM_IA_FREE_FAIL        (2)
#define XM_IA_NULL_POINTER     (3)
#define XM_IA_HISI_IVE_FAIL    (4)
#define XM_IA_PARA_FAULT       (5)

/* 雄迈智能分析模块常量定义 */
#define XM_IA_RELATIVE_COORD_SCALE          (8192)          /* 相对坐标量程 */
#define XM_IA_PATH_MAX_LEN                  (256)           /* 路径最大字符长度(字节) */

/* 芯片选择 */
typedef enum
{
    XM_IA_PLATFORM_XM530,             /* XM530,约束人脸数为6 */
    XM_IA_PLATFORM_XM550,            /*  XM550,最大人脸数为100 */
    XM_IA_PLATFORM_NUM              /* 平台数 */

}XM_IA_PLATFORM_E;

/* 旋转角度 */
typedef enum
{
    XM_IA_ROTATE_NO = 0,                /* 不旋转 */
    XM_IA_ROTATE_CLOCKWISE = 1,         /* 顺时针旋转90度 */
    XM_IA_ROTATE_CLOCKWISE_INVERT = 2   /* 逆时针旋转90度/顺时针旋转270度 */

}XM_IA_ROTATE_E;

/* 图像类型结构体 */
typedef enum
{
    XM_IMG_TYPE_U8C1 = 0x0,         /* 单通道8bit型图像 */
    XM_IMG_TYPE_S8C1 = 0x1,

    XM_IMG_TYPE_U16C1 = 0x2,        /* 单通道16bit型图像 */
    XM_IMG_TYPE_S16C1 = 0x3,

    XM_IMG_TYPE_U32C1 = 0x4,        /* 单通道32bit型图像 */
    XM_IMG_TYPE_S32C1 = 0x5,

    XM_IMG_TYPE_U64C1 = 0x6,        /* 单通道64bit型图像 */
    XM_IMG_TYPE_S64C1 = 0x7,

    XM_IMG_TYPE_U8C3_PKG = 0x8,     /* 三通道8bit Package型图像 */
    XM_IMG_TYPE_U8C3_PLN = 0x9,     /* 三通道8bit Planar型图像 */

    XM_IMG_TYPE_U16C3_PKG = 0xa,    /* 三通道16bit Package型图像 */
    XM_IMG_TYPE_U16C3_PLN = 0xb,    /* 三通道16bit Planar型图像 */

    /* YUV420 */
    XM_IMG_TYPE_YUV420SP = 0x10,    /* YUV420SP型图像 */
    XM_IMG_TYPE_NV12 = 0x11,        /* NV12型图像 */
    XM_IMG_TYPE_NV21 = 0x12,        /* NV21型图像 */

    XM_IMG_TYPE_YUV422SP = 0x20,    /* YUV422SP型图像 */
    XM_IMG_TYPE_YUV444 = 0x30,      /* YUV444型图像 */

    XM_IMG_TYPE_FREQENCY_DOMAIN = 0x40, /* 单通道float型频域RI图像 */

    XM_IMG_TYPE_UNKNOWN

}XM_IA_IMG_TYPE_E;

/** 点结构定义 */
typedef struct
{
    short s16X;         /**< 点坐标x */
    short s16Y;         /**< 点坐标y */
}XM_IA_POINT_S;

/** 矩形结构定义 */
typedef struct
{
    short s16X1;        /**< 左上角x坐标 */
    short s16Y1;        /**< 左上角y坐标 */
    short s16X2;        /**< 右下角x坐标 */
    short s16Y2;        /**< 右下角y坐标 */
}XM_IA_RECT_S;

/* 图像信息结构体 */
/* 【XM_IA图像规则】 */
/* 规则1：关于不同图像类型所使用的图像数据指针和跨距问题 */
/* 所有类型可适用单个数据指针和跨距，此时默认不同颜色分量接连且跨距相同 */
/* 若需要使用多通道的数据指针和跨距,RGB_PLN 3通道、YUV420SP 3通道、NV12和NV21 2通道其中通道0指向Y数据通道1指向UV数据 */
typedef struct
{
    unsigned short usWidth;        /* 图像宽度 */
    unsigned short usHeight;       /* 图像高度 */
    unsigned short usBitCnt;       /* 像素Bit数 */
    XM_IA_IMG_TYPE_E eImgType;     /* 图像格式 */

    void *pvImgData;               /* 图像数据通道0 */
    void *pvImgData1;              /* 图像数据通道1 */
    void *pvImgData2;              /* 图像数据通道2 */
    unsigned int u32PhyAddr;       /* ARM平台物理地址0 */
    unsigned int u32PhyAddr1;      /* ARM平台物理地址1 */
    unsigned int u32PhyAddr2;      /* ARM平台物理地址2 */
    unsigned short usStride;       /* 图像跨距0 */
    unsigned short usStride1;      /* 图像跨距1 */
    unsigned short usStride2;      /* 图像跨距2 */

    unsigned int uiReserved[16];   /* 保留字节 */
}XM_IA_IMAGE_S;

/* 目标跟踪状态 */
typedef enum
{
    XM_IA_TRACK_LAST,              /* 持续目标 */
    XM_IA_TRACK_LOSS               /* 丢失目标 */

}XM_IA_TRACK_STATUS_E;

/* 算法灵敏度 */
typedef enum
{
    IA_SENSE_LOW,                  /* 低灵敏度 */
    IA_SENSE_MID,                  /* 中灵敏度 */
    IA_SENSE_HIGH,                 /* 高灵敏度 */
    IA_SENSE_NUM,                  /* 灵敏度数量 */
}XM_IA_SENSE_E;

/* 算法图像尺寸握手结构体 */
typedef struct
{
    int iDevImgWid;                 /* 设备图像宽/XM510为算法确认过的图像宽 */
    int iDevImgHgt;                 /* 设备图像高/XM510为算法确认过的图像高 */
    int iAlgImgWid;                 /* 算法图像宽 */
    int iAlgImgHgt;                 /* 算法图像高 */
}XM_IA_IMAGESIZE_EXCHANGE_S;

/* 方向 */
typedef enum
{
    XM_IA_DIRECT_FORWARD,               /* 正向 */
    XM_IA_DIRECT_BACKWARD,              /* 逆向 */
    XM_IA_BIDIRECTION,                  /* 双向 */
    XM_IA_DIRECT_NUM
}XM_IA_DIRECTION_E;

/* 旋转角度 */
typedef struct
{
    int iYaw;                       /* 偏航角(-90, 90)正左转；负右转 */
    int iRoll;                      /* 横滚角(-90, 90)正：左低右高；负：左高右低 */
    int iPitch;                     /* 俯仰角(-90, 90)正：仰视；负：俯视*/

}XM_IA_ANGLE_S;

/* 智能IA算法类型 */
typedef enum
{
    /* 人脸算法占用 0～99 */
    XM_FACE_DET_IA = 1,                     /* 人脸检测算法 */
    XM_FACE_DET_REC_IA = 2,                 /* 人脸检测识别算法 */
    XM_FACE_DET_SSH_IA = 3,                 /* 人脸检测SSH算法 */
    XM_FACE_DET_REC_LOW_MEM_IA = 4,         /* 人脸检测识别算法(识别低存储低精度) */

    /* 人形算法占用 100～199 */
    XM_PEDETRIAN_DETECT_SSH_RULE_IA = 101,  /* 规则SSH人形检测 */
    XM_PEDETRIAN_FACE_DETECT_RULE_IA = 102, /* 规则人形人脸检测算法 */
    XM_PEDETRIAN_FACE_DET_REC_RULE_IA = 103, /* 规则人形人脸检测识别算法 */

    /* 车辆算法占用 200～299 */
    XM_PGS_IA = 200,                         /* 停车诱导系统，包含车辆检测、车牌检测、车牌识别 */
    XM_PV_DETECT_SSH_RULE_IA = 201,  /* 规则SSH机非人检测 */
    /* 摇头机算法占用 300~400 */
    XM_SC_IA = 300,
    /*物品盗遗算法占用400~500*/
    XM_OBJTL_IA = 400,

    XM_MAX_SIZE_IA,
}XM_IA_TYPE_E;

typedef XM_IA_RECT_S XM_IA_LINE_S;
typedef XM_IA_POINT_S XM_IA_IMG_SIZE_S;

/*************************************************
Author: WangZelang
Date: 2018-11-28
Description: 雄迈智能算法初始化
INPUT:  eIAtype       算法类型
        initParams    初始化参数结构体
        Handle        算法句柄
*************************************************/
int XM_IA_Create(XM_IA_TYPE_E eIAtype, void* initParams, void **Handle);

/*************************************************
Author: WangZelang
Date: 2018-11-28
Description: 雄迈智能算法工作函数
INPUT:  Handle        算法句柄
        inParams      算法工作入参
OUTPUT: Result        算法结果结构体
*************************************************/
int XM_IA_Run(void* Handle, void* inParams, void* Result);

/*************************************************
Author: WangZelang
Date: 2018-11-28
Description: 配置雄迈智能算法
INPUT:  configParams  算法配置结构体
        Handle        算法句柄
*************************************************/
int XM_IA_Configuration(void* configParams, void* Handle);

/*************************************************
Author: WangZelang
Date: 2018-11-28
Description: 销毁雄迈智能算法
INPUT:  Handle        算法句柄
*************************************************/
int XM_IA_Destruction(void **Handle);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif

