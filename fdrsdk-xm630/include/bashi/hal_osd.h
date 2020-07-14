
//
//  "$Id: osd.h 435 2015-06-17 10:57:16Z zcl $"
//
//  Copyright (c)2008-2010, ZheJiang JuFeng Technology Stock CO.LTD.
//  All Rights Reserved.
//
//	Description:	
//	Revisions:		Year-Month-Day  SVN-Author  Modification
//

#ifndef _FR_OSD_H_
#define _FR_OSD_H_

#if 0
#ifndef _SDK_STRUCT_OSD_PARAM_
#define _SDK_STRUCT_OSD_PARAM_
typedef struct
{
	int left; //0 - 100
	int top; //0 - 100
	int fontSize; //0:16  1:24  2:32  3:48  pixel
	int fgTrans; //0 - 100   (0表示完全透明)
	int bgTrans;//0 - 100   (0表示完全透明)
	int fgColor; //0xAABBGGRR   AA=00
	int bgColor; //0xAABBGGRR  AA=00
	char index;		//用于标记要修改哪个OSD，时间标题不用设置这个值，其它标题取值范围1~5
	char res[3];	//
} XMSDK_OSD_PARAM;
#endif
#endif

typedef struct tagUCS_FONT_HEADER
{
	char  magic[16];	//标志
	unsigned int size;		//字库总大小
	unsigned int blocks;	//块数
}UCS_FONT_HEADER;

typedef struct tagUCS_FONT_BLOCK
{
	unsigned short start;		//编码起始值
	unsigned short end;		//编码结束值
	unsigned short width;		//点阵宽度
	unsigned short height;	//点阵高度
	unsigned int roffs;	//字体点阵数据偏移
	unsigned int xoffs;	//字体扩展数据偏移
}UCS_FONT_BLOCK;
typedef struct {
	int left; 
	int top; 
	int fontSize; //0:16  1:24  2:32  3:48  pixel
	int fgTrans; //0 - 100   (0表示完全透明)
	int bgTrans;//0 - 100   (0表示完全透明)
	int fgColor; //0xAABBGGRR   AA=00
	int bgColor; //0xAABBGGRR  AA=00
	char index;		//用于标记要修改哪个OSD，时间标题不用设置这个值，其它标题取值范围1~5
	char res[3];	//
} XMSDK_OSD_PARAM;
/// 捕获标题叠加参数结构
typedef struct CAPTURE_TITLE_PARAM
{
	/// 标题序号，最多支持的标题数从CaptureGetCaps可以取得。序号0对应的是
	/// 时间标题。时间标题点阵由底层内部产生，width height raster参数被忽略。
	int		index;

	/// 是否显示。0-不显示，其他参数忽略，1-显示。
	int		enable;

	/// 标题左上角x坐标，取8的整数倍，采用相对坐标体系(CIF)。
	unsigned short	x;		

	/// 标题左上角y坐标，采用相对坐标体系(CIF)。
	unsigned short	y;
	
	/// 标题宽度，取8的整数倍。					
	unsigned short	width;		

	/// 标题高度。
	unsigned short	height;		

	/// 前景颜色，16进制表示为0xAABBGGRR，A(Alpha)表示透明度。
	/// text color, rgba 8:8:8:8	
	unsigned int	fg_color;
	
	/// 背景颜色，16进制表示为0xAABBGGRR，A(Alpha)表示透明度。
	/// background color, rgba 8:8:8:8	
	unsigned int	bg_color;

	/// 标题单色点阵数据。0表示该点无效，1表示该点有效。每个字节包含8个象素的
	/// 数据，最左边的象素在最高位，紧接的下一个字节对应右边8个象素，直到一行
	/// 结束，接下来是下一行象素的点阵数据。
	unsigned char	*raster;
}CAPTURE_TITLE_PARAM;



/// 字体属性描述，128字节对齐
typedef struct tagLogFont 
{ 
	unsigned long lfWidth; 
	unsigned long lfHeight; 
	unsigned char lfItalic; 
	unsigned char lfUnderline; 
	unsigned char lfStrikeOut; 
	unsigned char lfBold; 
	unsigned char lfScaling;
	unsigned char Reserved[3];
	unsigned int resv[32-4];
} LogFont; 

/// 字体轮廓描述结构体,共128字节
typedef struct tagFontGlyph
{
	int x;		///< 起始x坐标
	int y;		///< 起始y坐标
	int w;		///< 点阵宽度
	int h;		///< 点整高度
	int showW;	///< 显示的宽度
	unsigned short code;	///< 字体的unicode编码
	unsigned short resv;
	unsigned char* data;		///< 点阵数据二维数组，每个像素暂用一个字节，0-255等级反走样
	int resv2[32-7];	///< 保留字节
} FontGlyph;

typedef enum
{
	FS_NORMAL = 0x0000,
	FS_BOLD = 0x0001,
	FS_SMALL = 0x0002,
}FontStyle;

typedef struct tagSize 
{
	int w;
	int h;
}Size;
typedef struct xmBIT_MAP_INFO{
	unsigned char *raster;
	int width;
	int height;
}BIT_MAP_INFO;



int showTimeOSD(int nChannel, int nStream, XMSDK_OSD_PARAM* pOsdParm);
int shutTimeOSD(int nChannel, int nStream);

int GetBitMap(const char* pTitle,BIT_MAP_INFO *map_info);

int osdInit(const char* pFontFile);
int osdSetTitle(int nChannel, int nStream, XMSDK_OSD_PARAM* pOsdParm, const char* pTitle);

int osdGetTextExtent(const char* pText, const LogFont *fontAttri);
int osdGetTextRaster(unsigned char * pBuffer, int dotRectWidth, const char* pText, const LogFont *fontAttri);
unsigned short osdGetCharCode(const char* pch, int *pn);
unsigned char osdGetCharGlyph(unsigned short code, const LogFont *fontAttri, FontGlyph* p);
unsigned char osdGetOneFontFromFile(unsigned short code, unsigned char *p);

int osdRasterToPixel(const unsigned char* pRaster, int width, int height, unsigned int fgColor, unsigned int bgColor, unsigned char* pPixel);
int osdZoomPixel(const unsigned char* pOldPixel, int oldWidth, int oldHeight, int newWidth, int newHeight, unsigned char* pNewPixel, int bAddBoundry);
void osdZoomIn(const unsigned char* pOldPixel, int oldWidth, int oldHeight, int newWidth, int newHeight, unsigned char* pNewPixel);
void osdZoomOut(const unsigned char* pOldPixel, int oldWidth, int oldHeight, int newWidth, int newHeight, unsigned char* pNewPixel);
unsigned short osdGetAveragePixel(const unsigned short* pPixels, int pixelNum);
void osdAddBoundry(unsigned short* pPixels, int width, int height);


#endif //__FR_OSD_H__

