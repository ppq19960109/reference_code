#ifndef __SMART_FDR_H__
#define __SMART_FDR_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

#include "xm_ia_fdr_interface.h"
#include "smart_comm.h"
typedef struct xmFDR_Para_S//参数设置
{
	unsigned char Enable;//使能 0:不使能 1:使能
	XM_IA_FACE_ALG_E eFaceAlg;      /* 人脸算法 */
	int iFaceMinWid;                /*人脸检测最小人脸宽度 */
	int iFaceMaxWid;                /* 人脸检测最大人脸宽度 */
	XM_IA_RECT_S stFaceRecRegion;   /* 人脸识别区域 */
	XM_IA_SENSE_E eAlgSense;	/* 算法灵敏度设置 */
	unsigned char reserved[16];
}FDR_Para_S;

typedef struct tagfaceDetect
{
	char sex;//性别
	char age;//年龄
	char unused[2];//填充数据
	int iFaceId;				/* 人脸ID */
	float fClearness;			/* 人脸清晰评估值,值越高越清晰 */
	char resv[16];
}FACE_DETECT;


typedef struct
{
    int s32X;
    int s32Y;
    unsigned int u32Width;
    unsigned int u32Height;
}FDR_RECT_S;

typedef struct tagFrTargetInfo
{
	FDR_RECT_S stRect;  
	FACE_DETECT face;
	char resv[12];
}FR_TARGET_INFO;

typedef struct tagFrFeatrueInfo
{
	XM_IA_FACE_REC_INFO_S astFaceRecInfo;
	int reserved[16];
}FR_FEATURE_INFO;

typedef struct xm_IA_JPEG_INFO 
{
	unsigned char *pBuf;  //存放jpeg码流的起始地址，内存由调用者申请释放
	unsigned int bufLen;  //调用者申请buf的内存空间大小
	unsigned int streamLen; //实际jpeg码流长度 无人脸或者编码出错时，长度为0
	unsigned int reserved[8];
}IA_JPEG_INFO_S;
typedef struct
{
	/*
	usWidth要求16对齐
	usHeight要求2对齐
	96<=usWidth<=1280
	32<=usHeight<=720
	usWidth:usHeight要与原图的宽高比保持一致
	*/
    unsigned short usWidth;        /* 图像宽度 */
    unsigned short usHeight;       /* 图像高度 */
    void *pvImgData;               /* 图像数据通道0*/
    unsigned int uiReserved[16];   /* 保留字节 */
}XM_FDR_IMAGE_S;

/**
 @returns 0 on success.
 */
int FDR_Creat(int nChannel, FDR_Para_S *pstParam);

/**
 @returns 0 on success.
 */
int FDR_SetParam(int nChannel,FDR_Para_S *pstParam);

/**
 @已创建返回1，未创建返回0
 */
int FDR_QueryStatus(int nChannel);

/**
 @returns 0 on success.
 */
int FDR_Destory(int nChannel);

/**
返回值: <0:算法失败  >=0:检出的人脸数(人数目前只有0和1)
pFdrImage: 输入图像参数，需要8bit的NV12的图片
XM_IA_FACE_REC_INFO_S:人脸检测相关结果
IA_JPEG_INFO_S: 人脸jpeg编码buf相关参数
 */
int FDR_Feature(XM_FDR_IMAGE_S *pFdrImage, XM_IA_FACE_REC_INFO_S *pstFaceRecInfo, IA_JPEG_INFO_S *pstJpegInfo);

/// 人脸特征比对
/// \param [in] pfFaceFeat      人脸特征
/// \param [in] pstFeatureList  人脸特征列表
/// \param [in] fThresh         比对阈值下限
/// \param [out] pstMatchRslt   人脸匹配结果
/// \return 0  成功
/// \return !0 失败
int FR_MatchFace(XM_IA_FR_FEATRUES *pfFaceFeat, XM_IA_FR_FEATRUES_LIST *pstFeatureList, float fThresh, XM_IA_FR_MATCH_RSLT_S *pstMatchRslt);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif //


