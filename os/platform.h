#ifndef __UD_PLATFORM_H
#define __UD_PLATFORM_H

#if defined(_WIN32) || defined(_WIN64) 
#include "win32/serial.h"
#include "win32/time.h"
#include "win32/logging.h"
#include "win32/error.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef int bool;
#define false 0
#define true 1

#define VALID_PTR(_ptr) (!!(_ptr))
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

/*! Byte swap short, 'tiny' chip is little endian, if not match of OS cpu, switch it*/
inline int16_t swap_int16(int16_t val)
{
#if (BYTE_ORDER == BIG_ENDIAN)
    return (val << 8) | ((val >> 8) & 0xFF);
#else
    return val;
#endif
}

inline int32_t swap_int32(int32_t val)
{
#if (BYTE_ORDER == BIG_ENDIAN)
    return ((val >> 24) & 0xff) | // move byte 3 to byte 0
        ((val << 8) & 0xff0000) | // move byte 1 to byte 2
        ((val >> 8) & 0xff00) | // move byte 2 to byte 1
        ((val << 24) & 0xff000000); // byte 0 to byte 3
#else
    return val;
#endif
}

#define VALID_PTR(_ptr) ((_ptr) && (_ptr) != (typeof(_ptr))ERROR_PTR)

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