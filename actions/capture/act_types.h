/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009 Actions Semi Inc.
*/
/******************************************************************************/

/******************************************************************************/
#ifndef __ACT_TYPES_H__
#define __ACT_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

#ifndef __cplusclus
typedef int BOOL;
#ifdef TRUE
#undef TRUE
#endif
#define TRUE   1

#ifdef FALSE
#undef FALSE
#endif
#define FALSE  0

#endif

/******************************************************************************/
#ifdef __cplusplus
}
#endif

#endif
