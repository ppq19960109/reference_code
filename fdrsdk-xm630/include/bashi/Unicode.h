//
//  "$Id: Unicode.h 131 2009-11-14 06:12:32Z liwj $"
//
//  Copyright (c)2008-2010, ZheJiang JuFeng Technology Stock CO.LTD.
//  All Rights Reserved.
//
//	Description:	
//	Revisions:		Year-Month-Day  SVN-Author  Modification
//

#ifndef __INFRA_UNICODE_H__
#define __INFRA_UNICODE_H__

/*
 * linux/fs/nls_cp936.c
 *
 * Charset cp936 translation tables.
 * This translation table was generated automatically, the
 * original table can be download from the Microsoft website.
 * (http://www.microsoft.com/typography/unicode/unicodecp.htm)
 */
//#include "Types.h"


int Gb2312ToUni(const char* pSource,unsigned short* pTag,int nLenTag);

int Gb2312TUtf8(const char* pSource,unsigned char* pUtf8,int nUtf8Len);

int uni2char(const unsigned short uni,unsigned char *out, int boundlen);

int char2uni(const unsigned char *rawstring, int boundlen,unsigned short *uni);

int utf8_mbtowc(unsigned short *p, const unsigned char *s, int n);

int utf8_mbstowcs(unsigned short *pwcs, const unsigned char *s, int n);

int utf8_wctomb(unsigned char *s, unsigned short wc, int maxlen);

int utf8_wcstombs(unsigned char *s, const unsigned short *pwcs, int maxlen);

#endif
