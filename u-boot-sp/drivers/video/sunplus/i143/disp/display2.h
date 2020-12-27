// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Sunplus - All Rights Reserved
 *
 * Author(s): Hammer Hsieh <hammer.hsieh@sunplus.com>
 */

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

typedef unsigned long long  u64;
typedef unsigned int        u32;
typedef unsigned short      u16;
typedef unsigned char       u8;
typedef unsigned char       u08;

typedef long long           s64;
typedef int                 s32;
typedef short               s16;
typedef signed char         s8;

typedef unsigned char		BYTE;
typedef unsigned short		WORD;
typedef unsigned long		DWORD;

typedef unsigned long long 	UINT64;
typedef unsigned int   		UINT32;
typedef unsigned short 		UINT16;
typedef unsigned char 		UINT8;

typedef long long 			INT64;
typedef int   				INT32;
typedef short 				INT16;
typedef signed char			INT8;

typedef int					SINT32;
typedef short               SINT16;
typedef char                SINT8;

#define ALIGNED(x, n)			((x) & (~(n - 1)))
#define DISP_ALIGN(x, n)		(((x) + ((n)-1)) & ~((n)-1))
#define EXTENDED_ALIGNED(x, n)	(((x) + ((n) - 1)) & (~(n - 1)))

#define SWAP32(x)	((((UINT32)(x)) & 0x000000ff) << 24 \
					| (((UINT32)(x)) & 0x0000ff00) << 8 \
					| (((UINT32)(x)) & 0x00ff0000) >> 8 \
					| (((UINT32)(x)) & 0xff000000) >> 24)
#define SWAP16(x)	(((x) & 0x00ff) << 8 | ((x) >> 8))

#endif	//__DISPLAY_H__

