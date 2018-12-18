/**
 * @filename serial.h
 * @author Darryl Pogue
 * @designer Darryl Pogue
 * @date 2010 10 20
 * @project Terminal Emulator
 *
 * 2018/09/10 Modified to adapt to linux system
 * 
 * This file contains the definitions and prototypes for communication
 * across a serial port.
 */
#ifndef __LINUX_SERIAL_H_
#define __LINUX_SERIAL_H_

#include "../platform.h"
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

typedef struct __SER_PORT_STATE{
    DWORD baudRate;
    BYTE byteSize;
    BYTE stopBits;
    BYTE parity;
}SER_PORT_STATE_T;

/**
 * Initialises a serial port handle for reading and writing
 * @implementation serial.c
 */
HANDLE OpenPort(const void *port, const SER_PORT_STATE_T *state);

/**
* configure a serial port 
* @implementation serial.c
*/
int SetPortState(void *ptr_ser, const SER_PORT_STATE_T *state);

/**
* clear the serial port
* @implementation serial.c
*/
int FlushPort(void *ptr_ser);

/**
 * Sends data out the serial port pointed to by the handle fd.
 * @implementation serial.c
 */
int SendData(void *ptr_ser, const LPVOID tx, DWORD len);

/**
 * Receives data from the serial port pointed to by the handle fd.
 * @implementation serial.c
 */
int ReadData(void *ptr_ser, LPVOID rx, DWORD len);

/**
 * Closes a serial port handle.
 * @implementation serial.c
 */
void ClosePort(void *ptr_ser);

#endif
