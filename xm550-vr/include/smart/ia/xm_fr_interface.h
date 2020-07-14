/*--------------------------------------------------------------------------------
-- Copyright (C),2018, xmsilicon                                                --
-- File name:       xm_fr_interface.h                                           --
-- Author:          wuchunxuan                                                	--
-- Description:     xm  face recognize interface 								--
-- Date:            2018-09-13                                                	--
-- Modification:                                                              	--
--  <author>     <time>    <version>   <desc>                                 	--
--                                                                            	--
--------------------------------------------------------------------------------*/

#ifndef _XM_FR_INTERFACE_H_
#define _XM_FR_INTERFACE_H_
/*--------------------------------------------------------------------------------
	1. include headers
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	2. constant defines
--------------------------------------------------------------------------------*/
#define FR_INPUT_WIDTH                  (112)       /* FR 网络Input层网络输入图像宽度 */
#define FR_INPUT_HEIGHT                 (112)       /* FR 网络Input层网络输入图像高度 */
#define FACE_REC_FEATURE_NUM            (512)       /* 人脸识别的特征数量 */

/*--------------------------------------------------------------------------------
	3. data struct
--------------------------------------------------------------------------------*/

/* 智能IA算法类型 */
typedef enum
{
    CLASS_ANGELA = 0,                       /* 40MB人脸识别网络 */
    CLASS_BELLA = 1,                        /* 20MB人脸识别网络 */
    CLASS_CAROL = 2                        /* 25MB人脸识别网络 */
    
}XM_IA_FR_CLASS_E;

/* 人脸识别初始化参数 */
typedef struct
{
    /* INPUT */
    char acClassBinPath[XM_IA_PATH_MAX_LEN];  /* 分类器Bin文件路径 */
    XM_IA_FR_CLASS_E eFrClass;                /* 人脸识别分类器 */

    unsigned int uiReserved[15];    /* 保留字节 */

}XM_IA_FR_INIT_S;

/* 人脸特征,固定结构体 */
typedef struct
{
    float afFaceFeat[FACE_REC_FEATURE_NUM];     /* 512维人脸特征 */

}XM_IA_FR_FEATRUES;

/* 人脸特征列表 */
typedef struct
{
    int iFaceNumInList;                 /* 人脸特征库中人脸数 */
    XM_IA_FR_FEATRUES *pstFeatureItem;  /* 人脸特征值数组列表的首地址 */

    unsigned int uiReserved[16];        /* 保留字节 */
}XM_IA_FR_FEATRUES_LIST;

/* 人脸特征比对结果 */
typedef struct
{
    int iFaceRecIdx;                    /* 识别人在特征列表中的序号 */
    float fMatchingRate;                /* 配对率(0~100%) */

    unsigned int uiReserved[16];        /* 保留字节 */
}XM_IA_FR_MATCH_RSLT_S;

/// 人脸特征比对
/// \param [in] pfFaceFeat      人脸特征
/// \param [in] pstFeatureList  人脸特征列表
/// \param [in] fThresh         比对阈值(0~100%),大于阈值结果才匹配成功,高灵敏度0.440，中灵敏度0.515，低灵敏度0.570
/// \param [out] pstMatchRslt   人脸匹配结果
/// \return 0  成功
/// \return !0 失败
int XM_IA_FR_MatchFace(XM_IA_FR_FEATRUES *pstFaceFeat, XM_IA_FR_FEATRUES_LIST *pstFeatureList, float fThresh, XM_IA_FR_MATCH_RSLT_S *pstMatchRslt);


#endif