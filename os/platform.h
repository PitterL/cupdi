#ifndef __UD_PLATFORM_H
#define __UD_PLATFORM_H

//#define _LINUX

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef int bool;
#define false 0
#define true 1

#define ARRAY_SIZE(_) (sizeof (_) / sizeof (*_))

#if defined(UTILS_COMPILER_H_INCLUDED)
    #define SET_BIT(_x, _bit) Set_bits((_x), (1 << (_bit)))
    #define CLR_BIT(_x, _bit) Clr_bits((_x), (1 << (_bit)))
    #define TEST_BIT(_x, _bit) Tst_bits((_x), (1 << (_bit)))
#else
    #define SET_BIT(_x, _bit) ((_x) |= (1 << (_bit)))
    #define CLR_BIT(_x, _bit) ((_x) &= ~(1 << (_bit)))
    #define TEST_BIT(_x, _bit) ((_x) & (1 << (_bit)))
#endif
#define SET_AND_CLR_BIT(_x, _sbit, _cbit) (SET_BIT((_x), (_sbit)), CLR_BIT((_x), (_cbit)))

#if defined(_WIN32) || defined(_WIN64) 

#include "win32/serial.h"
#include "win32/delay.h"
#include "win32/swap.h"
#include "win32/logging.h"
#include "win32/error.h"

#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )

#elif defined(_LINUX)

typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef float               FLOAT;
typedef FLOAT               *PFLOAT;
typedef char                CHAR;
typedef unsigned char       UCHAR;
typedef unsigned char       *PUCHAR;
typedef short               SHORT;
typedef unsigned short      USHORT;
typedef unsigned short      *PUSHORT;
typedef long                LONG;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef long long           LONGLONG;
typedef unsigned long long  ULONGLONG;
typedef ULONGLONG           *PULONGLONG;
typedef unsigned long       ULONG;
typedef int                 INT;
typedef unsigned int        UINT;
typedef unsigned int        *PUINT;
typedef void                VOID;
typedef void                *LPVOID;

typedef void                *HANDLE;
typedef void                *LPCTSTR;

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "linux/serial.h"
#include "linux/delay.h"
#include "linux/swap.h"
#include "linux/logging.h"
#include "linux/error.h"

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))

#else
#error "No OS defined in platform.h !"
#endif

#define VALID_PTR(_ptr) ((_ptr) && (size_t)(_ptr) != ERROR_PTR)

#endif
