
//
//  "$Id: osd.c 435 2015-06-17 10:57:16Z zcl $"
//
//  Copyright (c)2008-2010, ZheJiang JuFeng Technology Stock CO.LTD.
//  All Rights Reserved.
//
//	Description:	
//	Revisions:		Year-Month-Day  SVN-Author  Modification
//
//#include "xmsdk/XMSDK.h" 
#include "hal_osd.h"
#include <pthread.h>
//#include "Capture.h"
//#include "localtime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "Unicode.h"

#define BITMSK(bit)	(int)(1 << (bit))
#define MakeRGB_16(r,g,b)       ((ushort)((((r) >> 3) << 10) | (((g) >> 3) << 5) | (((b) >> 3) << 0)))
//勾边使用的颜色
#define BOUNDRY_COLOR ((ushort)((0x07 << 10) | (0x07 << 5) | 0x07))

#define tracepoint() printf("%s %s   line:%d\n", __FILE__, __FUNCTION__, __LINE__)
#define trace printf

pthread_t g_threadTimeOSD;

FILE* g_FileFont = NULL; //字库文件读写句柄
UCS_FONT_HEADER g_UFH;	//字库文件头信息
UCS_FONT_BLOCK *g_pUFB = NULL;	//各个区的字符信息
Size g_sizeFont;
int g_nFontBytes;	//一个字符的点阵信息占的字节数(24*24/8)
unsigned char* g_pASCIIFont = NULL;	//ASCII字符的点阵数据字符信息

//---------------------------------------------------------------------------------------------------

int osdInit(const char* pFontPath)
{
	//读取字库头信息, 和ASCII点阵, 先不读取其他字符点阵
	int readLen = 0;

	g_FileFont = fopen(pFontPath, "r");

	if (g_FileFont)
	{
		readLen = fread(&g_UFH, 1, sizeof(UCS_FONT_HEADER), g_FileFont);
		if (readLen == sizeof(UCS_FONT_HEADER) && g_UFH.blocks > 0)
		{
			g_pUFB = (UCS_FONT_BLOCK*)malloc(sizeof(UCS_FONT_BLOCK)*g_UFH.blocks);
			fread(g_pUFB, sizeof(UCS_FONT_BLOCK), g_UFH.blocks, g_FileFont);

			g_sizeFont.w = g_pUFB[0].width;	//暂时只取第一个区点阵的宽度和高度
			g_sizeFont.h = g_pUFB[0].height;
			g_nFontBytes = g_sizeFont.w * g_sizeFont.h / 8;

			//所有ASCII字体拷到缓冲
			g_pASCIIFont = (unsigned char*)malloc(g_nFontBytes * 128 + 128);
			memset(g_pASCIIFont, 0, g_nFontBytes * 128 + 128);
			unsigned int i;
			for(i = 0; i < g_UFH.blocks; i++)
			{
				if(g_pUFB[i].end <= 0x80)
				{
					fseek(g_FileFont, g_pUFB[i].roffs, SEEK_SET);
					fread(&g_pASCIIFont[g_pUFB[i].start * g_nFontBytes], g_nFontBytes, (g_pUFB[i].end - g_pUFB[i].start), g_FileFont);
					fseek(g_FileFont, g_pUFB[i].xoffs, SEEK_SET);
					fread(&g_pASCIIFont[ 128 * g_nFontBytes + g_pUFB[i].start], 1, (g_pUFB[i].end - g_pUFB[i].start), g_FileFont);
				}
			}
		}
	} else {
		printf("Error: %s not exist!\n", pFontPath);
		return -1;
	}

	return 0;
}

int GetBitMap(const char* pTitle,BIT_MAP_INFO *map_info)
{
	char pTitleUtf8[1024];
	unsigned char*  pFontData;
	LogFont fontAttri;
	int dotHeight = 0;
	int dotWidth = 0;
	int i,j;
	int lineNum = 0; //行数
#if 0
    Gb2312TUtf8(pTitle, (unsigned char*)pTitleUtf8, sizeof(pTitleUtf8));
#else
    memcpy(pTitleUtf8, pTitle, strlen(pTitle)+1);
#endif
    memset(&fontAttri, 0, sizeof(fontAttri));
	fontAttri.lfWidth = 24;
	fontAttri.lfHeight = 24;
	if (strlen(pTitleUtf8) > 0)
	{
		//计算各行数据的点阵表示中最宽的宽度
		char pLineTitle[16][128]; //存储要叠加的各行信息
		int len;
		int nMaxLine = 12;

		//判断要叠加信息的行数，并将各行信息保存到pLineTitle[][]中
		len = strlen(pTitleUtf8);
		for(i = 0; i < len && lineNum < nMaxLine; i++)
		{
			for(j = 0; j+i < len && pTitleUtf8[j+i] != '\n'; j++)
			{
				pLineTitle[lineNum][j] = pTitleUtf8[i+j];
			}
			pLineTitle[lineNum][j] = '\0';
			lineNum++;
			i += j;
		}


		//计算最长那行的点阵宽度
		for (i = 0; i < lineNum; i++)
		{
			len = osdGetTextExtent(pLineTitle[i], &fontAttri);
			if (len > dotWidth)
			{
				dotWidth = len;
			}
		}
		dotWidth = (dotWidth + 7) / 8 * 8;
		dotHeight = fontAttri.lfHeight * lineNum;
		pFontData = (unsigned char*)malloc(dotWidth * dotHeight / 8);
		if (!pFontData)
		{
			return -1;
		}
		memset(pFontData, 0, dotWidth * dotHeight / 8);

		//转换为点阵数据
		int i;
		for ( i = 0; i < lineNum; i++)
		{
			osdGetTextRaster(pFontData + dotWidth * dotHeight / lineNum / 8 * i, dotWidth, pLineTitle[i], &fontAttri);
		}
		map_info->width = dotWidth;
		map_info->height = dotHeight;
		map_info->raster = pFontData;
	}

	return 0;
}

int osdGetTextExtent(const char* pText, const LogFont *fontAttri)
{
	if (!pText || strlen(pText) == 0)
	{
		return 0;
	}

	unsigned short code;//字符unicode
	int n;
	int w;//字符宽度
	int l;//字符字节数
	int width1 = 0;//字符串宽度
	int len = strlen(pText);

	for (n = 0; n < len; n += l, width1 += w)
	{
		code = osdGetCharCode(&pText[n], &l);
		if (l == 0)
		{
			break;
		}

		w = osdGetCharGlyph(code, fontAttri, NULL);
	}
	if (width1 % 8)
	{
		width1 += 8 - (width1 % 8);
	}
	return width1;
}

int osdGetTextRaster(unsigned char * pBuffer, int dotRectWidth, const char* pText, const LogFont *fontAttri)
{
	unsigned short code;//字符unicode
	int n;//字符计数
	int cw;//字符宽度
	int cl;//字符字节数
	int ox, ox_start, ox_end;//点阵偏移
	int oy, oy_start, oy_end;//点阵偏移
	int xoffset =0;//x坐标
	unsigned char * p;//点阵缓冲
	FontGlyph glyph;
	unsigned char raster[128];
	Size fontsize; 
	int x,y; //起点坐标
	int len = strlen(pText);

	if (!pBuffer)
	{
		return -1;
	}
	if (fontAttri)
	{
		memset(pBuffer, 0, dotRectWidth / 8 * fontAttri->lfHeight);
	}

	x = 0;
	y = 0;
	glyph.data = raster;
	if (x%8)
	{
		//trace("the X offset  is not match\n");
		return 0;
	}
	if (fontAttri)
	{
		fontsize.w = fontAttri->lfWidth;
		fontsize.h = fontAttri->lfHeight;
	}
	else
	{
		fontsize.w = 24;
		fontsize.h = 24;
	}

	//这里可能统一使用大字体会好点，需要测试
	oy_start = 0;
	oy_end = fontsize.h;

	for(n = 0; n < len; n += cl/*, x += cw*/)
	{
		code = osdGetCharCode(&pText[n], &cl);
		if(cl == 0)
		{
			break;
		}
		if(code == '\t')
		{
			xoffset = 96;
		}

		cw = osdGetCharGlyph(code, fontAttri, &glyph);
		p = glyph.data;

		ox_start = 0;
		ox_end = cw;
		if(xoffset + cw >dotRectWidth * 8)
		{
			ox_end = dotRectWidth * 8 - xoffset;
		}

		for (oy = oy_start; oy < oy_end; oy++)
		{
			for (ox = ox_start; ox < ox_end; ox++)
			{
				if (*(p + ox / 8) & (128 >> (ox % 8)))
				{
					pBuffer[(y+oy)*dotRectWidth/8+(x+xoffset+ox)/8] |= BITMSK(7-(xoffset+ox)%8);
				}
			}
			p += fontsize.w/8;
		}
		xoffset += cw;
	}

	return 1;
}
unsigned short osdGetCharCode(const char* pch, int *pn)
{
	unsigned char ch; //sigle char
	unsigned short code = 0; //unicode
	int flag = 0; //0 - empty, 1 - 1 char to finish unicode, 2 - 2 chars to finish unicode, -1 - error

	*pn = 0;
	while((ch = (unsigned char)*pch))
	{
		pch++;
		if(ch & 0x80)
		{
			if((ch & 0xc0) == 0xc0)
			{
				if((ch & 0xe0) == 0xe0)
				{
					if((ch & 0xf0) == 0xf0) //ucs-4?
					{
						break;
					}
					if(flag)
					{
						break;
					}
					*pn = 3;
					flag = 2;
					code |= ((ch & 0x0f) << 12);
				}
				else
				{
					if(flag)
					{
						break;
					}
					*pn = 2;
					flag = 1;
					code |= ((ch & 0x1f) << 6);
				}
			}
			else
			{
				if(flag == 0)
				{
					break;
				}
				else if(flag == 1) //unicode finished
				{
					code |= (ch & 0x3f);
					break;
				}
				else
				{
					code |= ((ch & 0x3f) << 6);
				}
				flag--;
			}
		}
		else //ASCII
		{
			if(flag)
			{
				break;
			}
			*pn = 1;
			code = ch;
			break;
		}
	}
	if(ch == 0)
	{
		code = 0;
	}

	return code;
}
//得到字符的宽度, 如果p不为NULL, 则用点阵填充
unsigned char osdGetCharGlyph(unsigned short code, const LogFont *fontAttri, FontGlyph* p)
{
	if (p)
	{
		p->x = 0;
		p->y = 0;
	}

	if(code < 0x80)//ASCII字符
	{
		if(p && p->data)
		{
			memcpy(p->data, g_pASCIIFont + code * g_nFontBytes, g_nFontBytes);
			p->showW = p->w = g_pASCIIFont[128 * g_nFontBytes + code];
		}
		return g_pASCIIFont[128 * g_nFontBytes + code];
	}

	if (p)
	{
		p->showW = p->w = osdGetOneFontFromFile(code, p->data);
		return p->w;
	}
	return osdGetOneFontFromFile(code, NULL);

}

//装载一个字符的字体, 传入参数为UCS-2编码, 返回值为字体宽度
unsigned char osdGetOneFontFromFile(unsigned short code, unsigned char *p)
{
	int start = 0;
	int end = g_UFH.blocks;
	int mid;
	unsigned char wx;

	while(1)
	{
		mid = (start + end)/2;
		if(mid == start)break;
		if(g_pUFB[mid].start <= code)
		{
			start = mid;
		}
		else
		{
			end = mid;
		}
	}
	if(code >= g_pUFB[mid].end) //字库中也没有, 显示未知字符.
	{
		trace("Unknown code = %04x\n", code);
		if(p)
		{
			memset(p, 0xff, g_nFontBytes);
		}
		return g_sizeFont.w / 2;
	}

	if(p)
	{
		fseek(g_FileFont, g_pUFB[mid].roffs + (code - g_pUFB[mid].start) * g_nFontBytes, SEEK_SET);
		fread(p, 1, g_nFontBytes, g_FileFont);
	}
	fseek(g_FileFont, g_pUFB[mid].xoffs + (code - g_pUFB[mid].start), SEEK_SET);
	fread(&wx, 1, 1, g_FileFont);
	return wx;
}


/*
* 作  用：将点阵转换成RGB(格式为1555，透明度忽略)格式的像素颜色值
* 参  数：[in] pRaster 要转换的点阵，每一位表示一个点，1表示显示，0表示不显示
*         [in] width   点阵宽带
*         [in] height  点阵高度
*         [in] fgColor 前景色，即要显示的点对应的颜色
*         [in] bgColor 背景色，即不用显示的点对应的颜色
*         [out] pPixel 存储转换好后的像素颜色值的缓冲区
* 返回值：true 转换成功    false 转换失败
* 说  明：因为转换成像素颜色值后会占用更多的空间，所以要求数组pPixel的大小至少是(width * height * 2)字节
*/
int osdRasterToPixel(const unsigned char* pRaster, int width, int height, unsigned int fgColor, unsigned int bgColor, unsigned char* pPixel)
{
	if (pRaster == NULL || pPixel == NULL || width <= 0 || height <= 0)
	{
		return 0;
	}

	unsigned short* pDst = (unsigned short*)pPixel;
	unsigned short newFgColor, newBgColor;
	unsigned char r, g, b;

	r = (unsigned char)((fgColor >> 16) & 0xFF);
	g = (unsigned char)((fgColor >> 8) & 0xFF);
	b = (unsigned char)((fgColor >> 0) & 0xFF);
	newFgColor  = MakeRGB_16(r, g, b);

	r = (unsigned char)((bgColor >> 16) & 0xFF);
	g = (unsigned char)((bgColor >> 8) & 0xFF);
	b = (unsigned char)((bgColor >> 0) & 0xFF);
	newBgColor = MakeRGB_16(r, g, b);
	int i,j;
	for(i = 0; i < width / 8 * height; i++)
	{
		for(j = 0; j < 8; j++)
		{
			if(pRaster[i] & (BITMSK(j)))
			{
				pDst[i * 8 + (8 - j)] = newFgColor | 0x8000;
			}
			else
			{
				pDst[i * 8 + (8 - j)] = newBgColor;
			}
		}
	}

	return 1;
}


/*
* 作  用：字符缩放，为了让效果更好，使用像素颜色值进行缩放
* 参  数：[in] pOldPixel  要缩放的原始像素值
*         [in] oldWidth   缩放前字符一行的像素个数
*         [in] oldHeigth  缩放前字符一列的像素个数
*         [in] newWidth   缩放后字符一行的像素个数
*         [in] newHeigth  缩放后字符一列的像素个数
*         [out] pNewPixel 存储缩放后像素颜色值的缓冲区
* 返回值：true 缩放成功   false 缩放失败
* 说  明：数组pPixel的大小至少是(newWidth * newHeight * 2)字节。宽和高不能一个放大，一个缩小。
*/
int osdZoomPixel(const unsigned char* pOldPixel, int oldWidth, int oldHeight, int newWidth, int newHeight, unsigned char* pNewPixel, int bAddBoundry)
{
	if (pOldPixel == NULL || pNewPixel == NULL || oldWidth <= 0 || oldHeight <= 0 || newWidth <= 0 || newHeight <= 0)
	{
		return 0;
	}

	if (oldHeight == newHeight)
	{
		memcpy(pNewPixel, pOldPixel, oldWidth * oldHeight * 2);
	}
	else if ((newWidth > oldWidth) || (newHeight > oldHeight))
	{//放大
		if (newWidth*2 / oldWidth == 3) //放大1.5倍
		{
			//先放大3倍
			unsigned char* pBig3 = (unsigned char*)malloc(oldHeight*3*oldWidth*3*2);
			osdZoomIn(pOldPixel, oldWidth, oldHeight, oldWidth*3, oldHeight*3, pBig3);

			//再缩小2倍
			osdZoomOut(pBig3, oldWidth*3, oldHeight*3, oldWidth*3/2, oldHeight*3/2, pNewPixel);
			free(pBig3);
		}
		else //放大整数倍
		{
			osdZoomIn(pOldPixel, oldWidth, oldHeight, newWidth , newHeight, pNewPixel);
		}
	}
	else
	{//缩小
		osdZoomOut(pOldPixel, oldWidth, oldHeight, newWidth , newHeight, pNewPixel);
	}
	if(bAddBoundry == 1)
	{
		osdAddBoundry((unsigned short*)pNewPixel, newWidth, newHeight);
	}

	return 1;
}


/*
* 作  用：字符放大
* 参  数：[in] pOldPixel  要放大的像素颜色值(格式为1555)
*         [in] oldWidth   原始字符一行的像素个数
*         [in] oldHeight  原始字符一列的像素个数
*         [in] newWidth   字符放大后一行的像素个数
*         [in] newHeight  字符放大后一列的像素个数
*         [out] pNewPixel 保存放大后字符像素颜色值的缓冲区 
* 返回值：无
*/
void osdZoomIn(const unsigned char* pOldPixel, int oldWidth, int oldHeight, int newWidth, int newHeight, unsigned char* pNewPixel)
{
	int widthZoomDegree = newWidth / oldWidth;
	int heightZoomDegree = newHeight / oldHeight;
	const unsigned short* pSrc = (unsigned short*)pOldPixel;
	unsigned short* pDst = (unsigned short*)pNewPixel;
	unsigned short  pixel = 0;
	int h,w,i,j;
	for( h = 0; h < oldHeight; h++)
	{
		for( w = 0; w < oldWidth; w++)
		{
			pixel = pSrc[h * oldWidth + w];
			for( i = 0; i < heightZoomDegree; i++)
			{
				for( j = 0; j < widthZoomDegree; j++)
				{
					pDst[((h * heightZoomDegree + i) * oldWidth + w)  * widthZoomDegree + j] = pixel;
				}
			}
		}
	}
}


/*
* 作  用：字符缩小
* 参  数：[in] pOldPixel  要缩小的像素颜色值(格式为1555)
*         [in] oldWidth   原始字符一行的像素个数
*         [in] oldHeight  原始字符一列的像素个数
*         [in] newWidth   字符缩小后一行的像素个数
*         [in] newHeight  字符缩小后一列的像素个数
*         [out] pNewPixel 保存缩小后字符像素颜色值的缓冲区 
* 返回值：无
*/
void osdZoomOut(const unsigned char* pOldPixel, int oldWidth, int oldHeight, int newWidth, int newHeight, unsigned char* pNewPixel)
{
	int widthZoomDegree = oldWidth / newWidth;
	int heightZoomDegree = oldHeight / newHeight;
	const unsigned short* pSrc = (unsigned short*)pOldPixel;
	unsigned short* pDst = (unsigned short*)pNewPixel;
	unsigned short  pBoundPixel[4*4]; //最多能存放16个点，等比例缩放时最多能支持4级缩小
	int h,w,i,j;
	for( h = 0; h < newHeight; h++)
	{
		for( w = 0; w < newWidth; w++)
		{
			memset(pBoundPixel, 0, sizeof(pBoundPixel));
			for( i = 0; i < heightZoomDegree; i++)
			{
				for( j = 0; j < widthZoomDegree; j++)
				{
					pBoundPixel[i * widthZoomDegree + j] = pSrc[((h * heightZoomDegree + i) * newWidth + w)  * widthZoomDegree + j];
				}
			}
			pDst[h * newWidth + w] = osdGetAveragePixel(pBoundPixel, widthZoomDegree * heightZoomDegree);
		}
	}
}


/*
* 作  用：得到几个像素混合后的颜色值
* 参  数：[in] pPixels  要混合的像素颜色值，颜色格式为1555
*         [in] pixelNum 要混合的像素个数
* 返回值：混合后的颜色值
*/
//前景色增强算法，通过增加前景色在混色中的比重来增强前景色
unsigned short osdGetAveragePixel(const unsigned short* pPixels, int pixelNum)
{
	unsigned int r = 0;
	unsigned int g = 0;
	unsigned int b = 0;
	int fgCnt = 0; //前景位置点的个数
	int bkCnt = 0;//背景色点的个数
	int Degree = 1;//放大倍数
	int i;
	for( i = 0; i < pixelNum; i++)
	{
		if(pPixels[i] & 0x8000)
		{
			fgCnt ++;
		}
	}
	bkCnt = pixelNum - fgCnt;//背景色点的个数
	Degree = 3;//放大倍数

	if( (pixelNum == 4 && fgCnt)	// 2*2倍缩放所有前景色都增强,目前只有2*2倍缩放
		|| (fgCnt >= pixelNum /2)) //其他倍数前景色超过 1/2才增强
	{
		for( i = 0; i < pixelNum; i++)
		{
			if(pPixels[i] & 0x8000)//前景色
			{
				r += Degree * (((pPixels[i] >> 10) & 0x1F) << 3);
				g += Degree * (((pPixels[i] >> 5) & 0x1F)  << 3);
				b += Degree * (((pPixels[i] >> 0) & 0x1F)  << 3);
			}
			else//背景色
			{
				r += ((pPixels[i] >> 10) & 0x1F) << 3;
				g += ((pPixels[i] >> 5) & 0x1F)  << 3;
				b += ((pPixels[i] >> 0) & 0x1F)  << 3;
			}
		}
		r /= (Degree * fgCnt + bkCnt);
		g /= (Degree * fgCnt + bkCnt);
		b /= (Degree * fgCnt + bkCnt);
		return (MakeRGB_16(r, g, b) | 0x8000);
	}
	else//背景色不增强
	{
		for(i = 0; i < pixelNum; i++)
		{
			r += (((pPixels[i] >> 10) & 0x1F) << 3);
			g += (((pPixels[i] >> 5) & 0x1F)  << 3);
			b += (((pPixels[i] >> 0) & 0x1F)  << 3);
		}
		r /= pixelNum;
		g /= pixelNum;
		b /= pixelNum;
		return MakeRGB_16(r, g, b);
	}
}

void osdAddBoundry(unsigned short* pPixels, int width, int height)
{
	unsigned short* pTmp = (unsigned short*)malloc(width * height * 2);
	memset(pTmp, 0x00, width * height * 2);
	int h,w;
	for( h = 1; h < height - 1; h++)
	{
		for( w = 1; w < width - 1; w++)
		{
			if(pPixels[h * width + w] & 0x8000)
			{
				pTmp[(h-1) * width + w] = 0x8000 | BOUNDRY_COLOR;
				pTmp[(h+1) * width + w] = 0x8000 | BOUNDRY_COLOR;
				pTmp[h * width + w - 1] = 0x8000 | BOUNDRY_COLOR;
				pTmp[h * width + w + 1] = 0x8000 | BOUNDRY_COLOR;
			}
		}
	}
	for( h = 1; h < height - 1; h++)
	{
		for( w = 1; w < width - 1; w++)
		{
			if(pPixels[h * width + w] & 0x8000)
			{
				pTmp[h * width + w] = pPixels[h * width + w];
			}
		}
	}

	memcpy(pPixels, pTmp, width * height * 2);

	free(pTmp);
}



