#ifndef __WIN32_ENUM_H_
#define __WIN32_ENUM_H_

#include <Windows.h>
#include <tchar.h>

BYTE ListSerialPorts(WORD vid, WORD pid);

#define USBSER_VID 0x03EB
#define USBSER_PID 0x6123

BYTE ListSerialPorts(WORD vid, WORD pid);

#endif