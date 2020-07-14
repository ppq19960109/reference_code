/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

#include "lvgui-yuv.h"
#include "logger.h"

#include <string.h>
#include <stdio.h>

#define NV12_NEAREST_SCALE      1

static void nv12_nearest_scale(uint8_t* src, uint8_t* dst,
                        int srcWidth, int srcHeight, int dstWidth, int dstHeight)
{
    register int sw = srcWidth;  //register keyword is for local var to accelorate 
    register int sh = srcHeight;
    register int dw = dstWidth;
    register int dh = dstHeight;
    register int y, x;
    unsigned long int srcy, srcx, src_index;//, dst_index;
    unsigned long int xrIntFloat_16 = (sw << 16) / dw + 1; //better than float division
    unsigned long int yrIntFloat_16 = (sh << 16) / dh + 1;

    uint8_t* dst_uv = dst + dh * dw; //memory start pointer of dest uv
    uint8_t* src_uv = src + sh * sw; //memory start pointer of source uv
    uint8_t* dst_uv_yScanline;
    uint8_t* src_uv_yScanline;
    uint8_t* dst_y_slice = dst; //memory start pointer of dest y
    uint8_t* src_y_slice;
    uint8_t* sp;
    uint8_t* dp;
 
    for (y = 0; y < (dh & ~7); ++y)  //'dh & ~7' is to generate faster assembly code
    {
        srcy = (y * yrIntFloat_16) >> 16;
        src_y_slice = src + srcy * sw;

        if((y & 1) == 0)
        {
            dst_uv_yScanline = dst_uv + (y / 2) * dw;
            src_uv_yScanline = src_uv + (srcy / 2) * sw;
        }

        for(x = 0; x < (dw & ~7); ++x)
        {
            srcx = (x * xrIntFloat_16) >> 16;
            dst_y_slice[x] = src_y_slice[srcx];

            if((y & 1) == 0) //y is even
            {
                if((x & 1) == 0) //x is even
                {
                    src_index = (srcx / 2) * 2;
            
                    sp = dst_uv_yScanline + x;
                    dp = src_uv_yScanline + src_index;
                    *sp = *dp;
                    ++sp;
                    ++dp;
                    *sp = *dp;
                }
             }
         }
         dst_y_slice += dw;
    }
}

#ifndef NV12_NEAREST_SCALE

static void nv12_bilinear_scale (uint8_t* src, uint8_t* dst, int srcWidth, int srcHeight, int dstWidth,int dstHeight)
{
    int x, y;
    int ox, oy;
    int tmpx, tmpy;
    int xratio = (srcWidth << 8)/dstWidth;
    int yratio = (srcHeight << 8)/dstHeight;
    uint8_t* dst_y = dst;
    uint8_t* dst_uv = dst + dstHeight * dstWidth;
    //uint8_t* src_y = src;
    //uint8_t* src_uv = src + srcHeight * srcWidth;

    uint8_t y_plane_color[2][2];
    uint8_t u_plane_color[2][2];
    uint8_t v_plane_color[2][2];
    int j,i;
    int size = srcWidth * srcHeight;
    int offsetY;
    
    /*
    int y_final, u_final, v_final; 
    int u_final1 = 0;
    int v_final1 = 0;
    int u_final2 = 0;
    int v_final2 = 0;
    int u_final3 = 0;
    int v_final3 = 0;
    int u_final4 = 0;
    int v_final4 = 0;
    */
    int u_sum = 0;
    int v_sum = 0;


    tmpy = 0;
    for (j = 0; j < (dstHeight & ~7); ++j)
    {
        //tmpy = j * yratio;
    oy = tmpy >> 8;
    y = tmpy & 0xFF;

    tmpx = 0;
    for (i = 0; i < (dstWidth & ~7); ++i)
    {
        // tmpx = i * xratio;
        ox = tmpx >> 8;
        x = tmpx & 0xFF;
    
        offsetY = oy * srcWidth;
            //YYYYYYYYYYYYYYYY
        y_plane_color[0][0] = src[ offsetY + ox ];
        y_plane_color[1][0] = src[ offsetY + ox + 1 ];
        y_plane_color[0][1] = src[ offsetY + srcWidth + ox ];
        y_plane_color[1][1] = src[ offsetY + srcWidth + ox + 1 ];
            
        int y_final = (0x100 - x) * (0x100 - y) * y_plane_color[0][0]
            + x * (0x100 - y) * y_plane_color[1][0]
            + (0x100 - x) * y * y_plane_color[0][1]
            + x * y * y_plane_color[1][1];
        y_final = y_final >> 16;
        if (y_final>255)
            y_final = 255;
        if (y_final<0)
            y_final = 0;
        dst_y[ j * dstWidth + i] = (uint8_t)y_final; //set Y in dest array 
            //UVUVUVUVUVUV
        if((j & 1) == 0) //j is even
        {
            if((i & 1) == 0) //i is even
            {
                u_plane_color[0][0] = src[ size + offsetY + ox ];
                u_plane_color[1][0] = src[ size + offsetY + ox ];
                u_plane_color[0][1] = src[ size + offsetY + ox ];
                u_plane_color[1][1] = src[ size + offsetY + ox ];

                v_plane_color[0][0] = src[ size + offsetY + ox + 1];
                v_plane_color[1][0] = src[ size + offsetY + ox + 1];
                v_plane_color[0][1] = src[ size + offsetY + ox + 1];
                v_plane_color[1][1] = src[ size + offsetY + ox + 1];
            }
            else //i is odd
            {
                u_plane_color[0][0] = src[ size + offsetY + ox - 1 ];
                u_plane_color[1][0] = src[ size + offsetY + ox + 1 ];
                u_plane_color[0][1] = src[ size + offsetY + ox - 1 ];
                u_plane_color[1][1] = src[ size + offsetY + ox + 1 ];

                v_plane_color[0][0] = src[ size + offsetY + ox ];
                v_plane_color[1][0] = src[ size + offsetY + ox + 1 ];
                v_plane_color[0][1] = src[ size + offsetY + ox ];
                v_plane_color[1][1] = src[ size + offsetY + ox + 1 ];
            }
        }
        else // j is odd
        {
            if((i & 1) == 0) //i is even
            {
                u_plane_color[0][0] = src[ size + offsetY + ox ];
                u_plane_color[1][0] = src[ size + offsetY + ox ];
                u_plane_color[0][1] = src[ size + offsetY + srcWidth + ox ];
                u_plane_color[1][1] = src[ size + offsetY + srcWidth + ox ];
                    
                v_plane_color[0][0] = src[ size + offsetY + ox + 1];
                v_plane_color[1][0] = src[ size + offsetY + ox + 1];
                v_plane_color[0][1] = src[ size + offsetY + srcWidth + ox + 1];
                v_plane_color[1][1] = src[ size + offsetY + srcWidth + ox + 1];                                                    
            }
            else //i is odd
            {
                u_plane_color[0][0] = src[ size + offsetY + ox - 1 ];
                u_plane_color[1][0] = src[ size + offsetY + srcWidth + ox - 1 ];
                u_plane_color[0][1] = src[ size + offsetY + ox + 1];
                u_plane_color[1][1] = src[ size + offsetY + srcWidth + ox + 1];

                v_plane_color[0][0] = src[ size + offsetY + ox ];
                v_plane_color[1][0] = src[ size + offsetY + srcWidth + ox ];
                v_plane_color[0][1] = src[ size + offsetY + ox + 2 ];
                v_plane_color[1][1] = src[ size + offsetY + srcWidth + ox + 2 ];
            }
        }

       int u_final = (0x100 - x) * (0x100 - y) * u_plane_color[0][0]
                     + x * (0x100 - y) * u_plane_color[1][0]
                     + (0x100 - x) * y * u_plane_color[0][1]
                     + x * y * u_plane_color[1][1];
       u_final = u_final >> 16;

       int v_final = (0x100 - x) * (0x100 - y) * v_plane_color[0][0]
                      + x * (0x100 - y) * v_plane_color[1][0]
                      + (0x100 - x) * y * v_plane_color[0][1]
                      + x * y * v_plane_color[1][1];
       v_final = v_final >> 16;
       if((j & 1) == 0)
       {
           if((i & 1) == 0)
           {    
               //set U in dest array  
               dst_uv[(j / 2) * dstWidth + i ] = (uint8_t)(u_sum / 4);
               //set V in dest array
               dst_uv[(j / 2) * dstWidth + i + 1] = (uint8_t)(v_sum / 4);
               u_sum = 0;
               v_sum = 0;
           }
       }
       else
       {
           u_sum += u_final;
           v_sum += v_final;
       }
       tmpx += xratio;
    }
    tmpy += yratio;
    }
}
#endif

int lvgui_yuv_nv12_scale(uint8_t *src, int src_width, int src_heigh,
                uint8_t *scale, int width, int heigh){

#ifdef NV12_NEAREST_SCALE
    nv12_nearest_scale(src, scale, src_width, src_heigh, width, heigh);
#else
    nv12_bilinear_scale(src, scale, src_width, src_heigh, width, heigh);
#endif

    return 0;
}


int lvgui_yuv_nv12_crop(uint8_t *src, int src_width, int src_heigh, 
                uint8_t *crop, int top, int left, int width, int heigh){
    uint8_t *Y_src, *Y_crop;
    uint8_t *UV_src, *UV_start, *UV_crop;
    int  i;

    if((top < 0) || (left < 0) || (width > src_width) || (heigh > src_heigh)){
        return -1;
    }

    if(((top % 2) != 0) || ((left % 2) != 0) || ((width % 2 ) != 0) || ((heigh %2) != 0)){
        return -1;
    }

    if(((width + left) > src_width) || ((heigh  + top) > src_heigh)){
        return -1;
    }

    // copy Y
    Y_crop = crop;
    for(i = 0; i < heigh; i++){
        Y_src = src + src_width * (top + i) + left;
        memcpy(Y_crop, Y_src, width);
        Y_crop += width;
    }

    // copy uv
    UV_start = src + (src_width * src_heigh);
    UV_crop = crop + (width * heigh);
    for(i = 0; i < heigh/2; i++){
        UV_src = UV_start  + src_width * (top/2 + i) + left;
        memcpy(UV_crop, UV_src, width);
        UV_crop += width;
    }

    return 0;
}
                                           
int lvgui_yuv_nv12_to_rgb1555(const uint8_t *yuv, int width, int heigh, uint8_t *rgb){
    const int nv_start = width * heigh ;
    int  i, j, rgb_index = 0;
    uint8_t y, u, v;
    int r, g, b, nv_index = 0;
    uint16_t d;
	
    for(i = 0; i <  heigh ; i++){
		for(j = 0; j < width; j ++){
			nv_index = i / 2  * width + j - j % 2;

			y = yuv[rgb_index];
			u = yuv[nv_start + nv_index ];
			v = yuv[nv_start + nv_index + 1];

			r = y + (140 * (v-128))/100;  //r
			g = y - (34 * (u-128))/100 - (71 * (v-128))/100; //g
			b = y + (177 * (u-128))/100; //b
				
			if(r > 255) r = 255;
			if(g > 255) g = 255;
			if(b > 255) b = 255;
       		if(r < 0) r = 0;
			if(g < 0) g = 0;
			if(b < 0) b = 0;
            
            d = ((uint16_t)(r >> 3) << 10) | ((uint16_t)(g >> 3) << 5) | (uint16_t)(b >> 3);

            rgb[rgb_index*2] = d & 0xFF;
            rgb[rgb_index*2 + 1] = d >> 8;

			rgb_index++;
		}
    }

    return 0;
}

int lvgui_yuv_nv12_to_rgb24(const uint8_t *yuv, int width, int heigh, uint8_t *rgb){
    const int nv_start = width * heigh ;
    int  i, j, rgb_index = 0;
    uint8_t y, u, v;
    int r, g, b, nv_index = 0;
	
    for(i = 0; i <  heigh ; i++){
		for(j = 0; j < width; j ++){
			nv_index = i / 2  * width + j - j % 2;

			y = yuv[rgb_index];
			u = yuv[nv_start + nv_index ];
			v = yuv[nv_start + nv_index + 1];

			r = y + (140 * (v-128))/100;  //r
			g = y - (34 * (u-128))/100 - (71 * (v-128))/100; //g
			b = y + (177 * (u-128))/100; //b
				
			if(r > 255) r = 255;
			if(g > 255) g = 255;
			if(b > 255) b = 255;
       		if(r < 0) r = 0;
			if(g < 0) g = 0;
			if(b < 0) b = 0;
            
			rgb[rgb_index*3 + 0] = b;
			rgb[rgb_index*3 + 1] = g;
			rgb[rgb_index*3 + 2] = r;

			rgb_index++;
		}
    }

    return 0;
}

int lvgui_save_to_bmp(const char  *filepath, uint8_t *rgb, int width, int heigh, int chs){
    lvgui_bmp_header_t header = {};
    lvgui_bmp_info_t   info = {};
    FILE *fj;
    size_t wlen;

    header.bfType = ('M' << 8) | 'B';
    header.bfSize = width * heigh * chs + sizeof(lvgui_bmp_header_t) + sizeof(lvgui_bmp_info_t);
    header.bfOffBits = sizeof(lvgui_bmp_header_t) + sizeof(lvgui_bmp_info_t);

    info.biSize = sizeof(lvgui_bmp_info_t);
    info.biWidth = width;
    info.biHeight = -heigh;
    info.biPlanes = 1;
    info.biBitCount = 8*chs;
    info.biCompression = 0;
    info.biSizeImage = 0;
    info.biXPelsPerMeter = 2835;
    info.biYPelsPerMeter = 2835;
    info.biClrUsed = 0;
    info.biClrImportant = 0;


    fj = fopen(filepath, "w+");
    if(fj == NULL){
        printf("save_bmp_file => open file  %s fail\n", filepath);
        return -1;
    }

    wlen = fwrite(&header, 1, sizeof(header), fj);
    if(wlen  != sizeof(header)){
        printf("save_bmp_file => write header fail:%zu\n", wlen);
        fclose(fj);
        return -1;
    }

    wlen = fwrite(&info, 1, sizeof(info), fj);
    if(wlen  != sizeof(info)){
        printf("save_bmp_file => write info fail:%zu\n", wlen);
        fclose(fj);
        return -1;
    }
    
    wlen = fwrite(rgb, 1, width*heigh*chs, fj);
    if(wlen  != width*heigh*chs){
        printf("save_bmp_file => write rgb fail:%zu\n", wlen);
        fclose(fj);
        return -1;
    }

    fclose(fj);

    return 0;
}
