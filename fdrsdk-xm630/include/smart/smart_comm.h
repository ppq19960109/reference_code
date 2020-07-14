#ifndef __SMART_COMM_H__
#define __SMART_COMM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/* 人脸算法类型 */
typedef enum
{
    XM_IA_FACE_DET,                 /* 人脸检测 */
    XM_IA_FACE_DET_REC,             /* 人脸检测识别 */
    XM_IA_FACE_ALG_NUM              /* 人脸相关算法数量 */
}XM_IA_FACE_ALG_E;
typedef enum
{
	SMART_CAR_PLATE = 0, //表示车牌类型
	SMART_FACE_DET = 1, //表示人脸检测
	SMART_PGS = 2, //车位检测
	SMART_FACE_REC = 3, //人脸识别
	SMART_ALL = 255,//表示全部类型
}SMART_JPEG_TYPE_E;

typedef struct
{
	char resultType; //识别类型类型  SMART_JPEG_TYPE_E
	char picSubType; //0:大图  1:小图
	char picFormat;  //0:jpg  1:bmp   2:yuv
	char tagNum;//目标个数
	char reserved[16];
}IA_COMM_RES_S;

/*
	说明：
	SMART_IA_RES_S结构体后面紧跟着comm.tagNum个对应算法结果信息结构体
	例如：人脸检测算法，结果结构体中的comm.tagNum = 2
	那么jpeg码流中结果信息的布局如下
	|-------------------|-----------------|-----------------|
	|  SMART_IA_RES_S   |  FR_TARGET_INFO |  FR_TARGET_INFO |
	
	不同算法结果对应的结果信息结构体：
	SMART_FACE_DET:人脸检测 FR_TARGET_INFO 
	SMART_PGS:车位 PGS_Result_S
	SMART_FACE_REC:人脸特征 FR_FEATURE_INFO
*/
typedef struct
{
	IA_COMM_RES_S comm;
	int index;//当前图片的索引号 [0,7]
	void *pTargetInfo;//目前此参数对应用层无实际意义
}SMART_IA_RES_S;
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif //__SMART_COMM_H__



