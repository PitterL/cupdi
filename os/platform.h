#ifndef __UD_PLATFORM_H
#define __UD_PLATFORM_H

#if defined(_WIN32) || defined(_WIN64) 
#include "win32/serial.h"
#include "win32/time.h"
#include "win32/swap.h"
#include "win32/logging.h"
#include "win32/error.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef int bool;
#define false 0
#define true 1

#define ARRAY_SIZE(_) (sizeof (_) / sizeof (*_))

#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )

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

#else
typedef void * HANDLE;
typedef void * LPCTSTR;

#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#define VALID_PTR(_ptr) ((_ptr) && (long)(_ptr) != ERROR_PTR)

#ifndef EVENPARITY
#define NOPARITY            0
#define ODDPARITY           1
#define EVENPARITY          2
#define MARKPARITY          3
#define SPACEPARITY         4
#endif

#ifndef TWOSTOPBITS
#define ONESTOPBIT          0
#define ONE5STOPBITS        1
#define TWOSTOPBITS         2
#endif

#endif