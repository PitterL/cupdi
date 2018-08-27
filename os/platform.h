#ifndef __UD_PLATFORM_H
#define __UD_PLATFORM_H

#if defined(_WIN32) || defined(_WIN64) 
#include "win32/serial.h"
#include "win32/time.h"
#include "win32/logging.h"
#include "win32/error.h"
#include <stdbool.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define VALID_PTR(_ptr) (!!(_ptr))
#define ARRAY_SIZE(_) (sizeof (_) / sizeof (*_))

//typedef enum {false, true} bool;

#else
typedef void * HANDLE;
typedef void * LPCTSTR;
#endif

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