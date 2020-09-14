#ifndef __UD_PLATFORM_H
#define __UD_PLATFORM_H

#ifdef CUPDI

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#include <stdbool.h>

#define ARRAY_SIZE(_) (sizeof (_) / sizeof (*_))
#define VALID_PTR(_ptr) ((_ptr) && (size_t)(_ptr) != ERROR_PTR)

#if defined(UTILS_COMPILER_H_INCLUDED)
    #define SET_BIT(_x, _bit) Set_bits((_x), (1 << (_bit)))
    #define CLR_BIT(_x, _bit) Clr_bits((_x), (1 << (_bit)))
    #define TEST_BIT(_x, _bit) Tst_bits((_x), (1 << (_bit)))
#else
    #define SET_BIT(_x, _bit) ((_x) |= (1 << (_bit)))
    #define CLR_BIT(_x, _bit) ((_x) &= ~(1 << (_bit)))
    #define TEST_BIT(_x, _bit) ((_x) & (1 << (_bit)))
#endif
#define BIT_MASK(_bit) (1 << (_bit))

#define SET_AND_CLR_BIT(_x, _sbit, _cbit) (SET_BIT((_x), (_sbit)), CLR_BIT((_x), (_cbit)))

#define L8_TO_LT16(__v0, __v1) ((((short)(__v1)) << 8) | ((short)(__v0)))
#define L16_TO_LT32(__v0, __v1) ((((short)(__v1)) << 16) | ((short)(__v0)))

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
typedef void               *LPVOID;

typedef void                *HANDLE;
typedef void                *LPCTSTR;

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "serial.h"
#include "delay.h"
#include "swap.h"
#include "logging.h"
#include "error.h"

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))

#endif

#endif