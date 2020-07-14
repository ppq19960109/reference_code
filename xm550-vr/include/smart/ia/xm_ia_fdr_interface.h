/*****************************************************************************

------------------------------------------------------------------------------
        xm_ia_fdr_interface.h
产品名:    人脸检测识别
模块名:    人脸检测识别
生成日期:  2018-09-10
作者:     王则浪
文件描述:  人脸检测识别对外头文件
*****************************************************************************/
#ifndef _XM_IA_FDR_INTERFACE_H_
#define _XM_IA_FDR_INTERFACE_H_

#include "xm_ia_comm.h"
#include "xm_fr_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FACE_MAX_NUM                    (6)             /* 最大人脸数量 */
#define FACE_DETECT_MAX_NUM             (32)            /* 最大人脸数量 */
#define FACE_POINT_NUM                  (68)            /* 人脸定位点数 */
#define FACE_REC_FEATURE_NUM            (512)           /* 人脸识别的特征数量 */
#define FACE_REC_VERSION_LEN            (10)            /* 人脸识别版本号长度 */

/* 人脸检测识别算法模式 */
typedef enum
{
    XM_IA_FDR_MODE_FDR,             /* 人脸检测识别 */
    XM_IA_FDR_MODE_EXTRACT_FEATURE, /* 录人脸图像特征 */
    XM_IA_FDR_MODE_NUM              /* 人脸检测识别算法模式类型 */

}XM_IA_FDR_MODE_E;

/* 红外非红外结构体 */
typedef enum
{
    XM_IA_ALIVE_MODE_INFRARED_INACTIVE,           /* 关闭红外模式 */
    XM_IA_ALIVE_MODE_INFRARED_ACTIVE              /* 启动红外模式 */
}XM_IA_ALIVE_MODE_E;

/* 人脸特征点结构体 */
typedef enum
{
    XM_IA_FP_MODE_INACTIVE,           /* 关闭特征点模式 */
    XM_IA_FP_MODE_ACTIVE              /* 启动特征点模式 */
}XM_IA_FP_MODE_E;

/* 人脸检测结果结构体 */
typedef enum
{
    XM_IA_ALIVE_INACTIVE,           /* 无活体 */
    XM_IA_ALIVE_ACTIVE              /* 有活体 */
}XM_IA_ALIVE_S;

typedef struct
{
    int iFaceMinWid;                /* 图像中最小人脸宽度：与图像宽高窄边相对坐标(0~8192) */
    int iFaceMaxWid;                /* 图像中最大人脸宽度：与图像宽高窄边相对坐标(0~8192) */

    unsigned int uiReserved[16];    /* 保留字节 */
}XM_IA_FD_CONFIG_S;

/* 人脸识别配置结构体 */
typedef struct
{
    int iClearThr;                  /* 人脸清晰度识别阈值 */
    XM_IA_RECT_S stFaceRecRegion;   /* 人脸识别区域：相对坐标(0~8191) */

    unsigned int uiReserved[16];    /* 保留字节 */
}XM_IA_FR_CONFIG_S;

/* 人脸检测识别配置参数 */
typedef struct
{
    XM_IA_SENSE_E eAlgSense;                /* 算法灵敏度 */

    XM_IA_FD_CONFIG_S stFaceDetConfig;      /* 人脸检测配置 */
    XM_IA_FR_CONFIG_S stFaceRecConfig;      /* 人脸识别配置 */

    XM_IA_ROTATE_E eRotate;                 /* 旋转方向选择 */

    unsigned int uiReserved[15];            /* 保留字节 */
}XM_IA_FDR_CONFIG_S;

/* 人脸检测识别初始化参数 */
typedef struct
{
    /* INPUT */
    char acClassBinPath[XM_IA_PATH_MAX_LEN];  /* 分类器Bin文件路径 */
    unsigned int uiInStructSize;    /* Run入参参数结构体检查Size */
    unsigned int uiInitStructSize;  /* 初始化参数结构体检查Size */
    unsigned int uiConfigStructSize;/* 配置参数结构体检查Size */
    unsigned int uiRsltStructSize;  /* 算法结果参数结构体检查Size */

    int iRotateMode;                /* 是否启用旋转模式：1启用；0不启用。启用旋转模式后支持0度不旋转、90度旋转和270度旋转 */
    XM_IA_IMG_TYPE_E eDevImgType;   /* 设备图像类型 */
    XM_IA_IMG_SIZE_S stDevImgSize;  /* 设备图像尺寸/XM510为算法确认过的图像尺寸 */
    XM_IA_FDR_CONFIG_S stFdrConfig; /* 人脸检测识别配置 */
    XM_IA_ALIVE_MODE_E eInfraredMode;     /* 红外模式 */
    XM_IA_FP_MODE_E eFaceAngleMode;                 /*人脸角度检测开关，人脸识别默认开启*/

    /* OUTPUT */
    char acFrAlgVersion[FACE_REC_VERSION_LEN];  /* 人脸识别算法版本号 */
    XM_IA_IMG_SIZE_S stAlgImgSize;  /* 算法图像尺寸 */
    XM_IA_PLATFORM_E ePlatForm; /* 芯片平台选择 */

}XM_IA_FDR_INIT_S;

/* 人脸检测识别工作入参 */
typedef struct
{
    unsigned char ucIsFirstFrm;     /* 首帧非首帧标志：1首帧/0非首帧 */
    XM_IA_FDR_MODE_E eFdrMode;      /* 人脸检测识别算法模式 */
    XM_IA_IMAGE_S *pstImg[2];          /* 图像结构体,0:彩图,1:红外图 */
}XM_IA_FDR_IN_S;

/* 人脸检测面部信息 */
typedef struct
{
    int iFaceId;                /* 人脸ID */

    float fScore;               /* 人脸置信概率 */
    float fClearness;           /* 人脸清晰评估值,值越高越清晰 */

    /* 以相对坐标方式提供(0~8191) */
    XM_IA_RECT_S stFace;        /* 人脸位置 */
    XM_IA_POINT_S stNose;       /* 鼻子位置 */
    XM_IA_POINT_S stLftEye;     /* 左眼位置 */
    XM_IA_POINT_S stRgtEye;     /* 右眼位置 */
    XM_IA_POINT_S stMouthLft;   /* 左嘴角位置 */
    XM_IA_POINT_S stMouthRgt;   /* 右嘴角位置 */
    XM_IA_POINT_S astFacePoint[FACE_POINT_NUM]; /* 人脸64个特征位置点 */
    XM_IA_ALIVE_S iAliveFace;                   /* 人脸活体检测 */
    int iAliveScore;                            /* 人脸活体分数[0～100],分数越高活体概率越高 */

    unsigned int uiReserved[14];/* 保留字节 */
}XM_IA_FACE_DET_INFO_S;

/* 待识别人脸状态 */
typedef enum
{
    FACE_STATUS_APPROPRIATE,        /* 恰当人脸 */
    FACE_STATUS_LEAN,               /* 倾斜人脸 */
    FACE_STATUS_OUTDROP,            /* 界外人脸 */
    FACE_STATUS_ANALYSISING,        /* 尚在分析中人脸 */
    FACE_STATUS_LOW_RELIABLE,       /* 低置信度人脸 */
    FACE_STATUS_NUM                 /* 人脸状态数 */
}APPROPRIATE_FACE_STATUS_E;

/* 人脸角度结构体 方向均以本人方向*/
typedef struct
{
    int iYaw;               /* 偏航角(-90, 90)正左转；负右转 */
    int iRoll;              /* 横滚角(-90, 90)正：左低右高；负：左高右低 */
    int iPitch;             /* 俯仰角(-90, 90)正：仰视；负：俯视*/

    APPROPRIATE_FACE_STATUS_E eFaceAngleStatus;     /* 人脸角度状态 */

}XM_IA_FDR_ANGLE_S;

/* 人脸检测结果 */
typedef struct
{
    int iFaceNum;               /* 人脸检测数 */
    XM_IA_FACE_DET_INFO_S astFaceRect[FACE_DETECT_MAX_NUM];   /* 人脸信息 */
    XM_IA_FDR_ANGLE_S astFaceAngle[FACE_DETECT_MAX_NUM];/*人脸角度信息*/

    unsigned int uiReserved[16];/* 保留字节 */
}XM_IA_FD_RESULTS_S;

/* 人脸识别信息 */
typedef struct
{
    int iFaceId;                                /* 识别人脸ID号 */
    XM_IA_RECT_S stFaceRect;                    /* 识别位置:相对坐标(0~8191) */
    XM_IA_FR_FEATRUES stFrFeatures;             /* 人脸特征 */

    unsigned int uiReserved[16];                /* 保留字节 */
}XM_IA_FACE_REC_INFO_S;

/* 人脸识别结果 */
typedef struct
{
    int iFaceNum;                   /* 人脸识别数 */
    XM_IA_FACE_REC_INFO_S astFaceRecInfo[FACE_MAX_NUM];   /* 人脸识别信息 */

    unsigned int uiReserved[16];                /* 保留字节 */
}XM_IA_FR_RESULTS_S;

/* 人脸检测识别结果 */
typedef struct
{
    XM_IA_FD_RESULTS_S stFdResult;      /* 人脸检测结果 */
    XM_IA_FR_RESULTS_S stFrResult;      /* 人脸识别结果 */

    unsigned int uiReserved[16];                /* 保留字节 */
}XM_IA_FDR_RESULTS_S;

#ifdef __cplusplus
}
#endif

#endif