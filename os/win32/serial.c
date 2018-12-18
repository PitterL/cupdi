/**
 * @filename serial.c
 * @author Darryl Pogue
 * @designer Darryl Pogue
 * @date 2010 10 20
 * @project Terminal Emulator
 *
 * This file contains the function implementations for serial port
 * communication.
 */
#include "serial.h"
#include "error.h"

typedef struct _upd_sercom {
#define UPD_SERCOM_MAGIC_WORD 0xA5A5 //'user'
    unsigned int mgwd;
    HANDLE fd;
}upd_sercom_t;

#define VALID_SER(_ser) ((_ser) && (((upd_sercom_t *)(_ser))->mgwd == UPD_SERCOM_MAGIC_WORD) && ((upd_sercom_t *)(_ser))->fd)
#define FD(_ser) ((HANDLE)(_ser)->fd)

/**
 * Initialises a serial port handle for reading and writing
 *
 * @param char *port  The name of the serial port to open.
 * @param DWORD baudRate The bautrate to commulication
 * @param BYTE byteSize The data size
 * @param BYTE stopBits The number of stop bits ONESTOPBIT|ONE5STOPBITS|TWOSTOPBITS
 * @param BYTE parity The partity checksum  NOPARITY|ODDPARITY|EVENPARITY
 * @returns HANDLE fd   The pointer to the handle which will be initialised to the open
 *                      serial port connection.
 */

HANDLE OpenPort(const void *port, const SER_PORT_STATE_T *st) {
    upd_sercom_t *ser;
    HANDLE fd = NULL;
    char comport[16] = "\\\\.\\";

    /* Create the file descriptor handle */
    strncat(comport, port, sizeof(comport) - 1);
    if ((fd = CreateFileA(comport, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                    OPEN_EXISTING, 0/*FILE_FLAG_OVERLAPPED*/, NULL))
        == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    ser = (upd_sercom_t *)malloc(sizeof(*ser));
    if (!ser) {
        CloseHandle(fd);
        return NULL;
    }

    ser->mgwd = UPD_SERCOM_MAGIC_WORD;
    ser->fd = fd;

    if (SetPortState(ser, st) != 0) {
        ClosePort(ser);
        ser = NULL;
    }

    return ser;
}

/**
* Set a serial port state
*
* @param char *ser  The port handle.
* @param DWORD baudRate The bautrate to commulication
* @param BYTE byteSize The data size
* @param BYTE stopBits The number of stop bits ONESTOPBIT|ONE5STOPBITS|TWOSTOPBITS
* @param BYTE parity The partity checksum  NOPARITY|ODDPARITY|EVENPARITY
* @returns 0 - success, other value failed code
*/
int SetPortState(void *ptr_ser, const SER_PORT_STATE_T *st) {
    upd_sercom_t *ser = (upd_sercom_t *)ptr_ser;
    COMMPROP cprops;
    DCB dcb;
    COMMCONFIG config;
    COMMTIMEOUTS timeouts = { 
        100, //interval timeout. 0 = not used, The maximum time allowed to elapse before the arrival of the next byte on the communications line, in milliseconds
        0, // read multiplier .For each read operation, this value is multiplied by the requested number of bytes to be read.
        1000, // read constant (milliseconds)
        0, // Write multiplier
        0  // Write Constant
    };

    if (!VALID_SER(ser))
        return ERROR_PTR;

    if (!GetCommProperties(FD(ser), &cprops)) {
        return -2;
    }

    /* Setup the port for sending and receiving data */
    if (!SetupComm(FD(ser), cprops.dwMaxRxQueue, cprops.dwMaxTxQueue)) {
        return -3;
    }

    /* Get the current DCB settings */
    if (!GetCommState(FD(ser), &dcb)) {
        return -4;
    }

    dcb.BaudRate = st->baudRate;
    dcb.ByteSize = st->byteSize;
    dcb.StopBits = st->stopBits;
    dcb.Parity = st->parity;

    config.dwSize = sizeof(COMMCONFIG);
    config.wVersion = 0x1;
    config.dcb = dcb;
    config.dwProviderSubType = cprops.dwProvSubType;
    config.dwProviderOffset = 0;
    config.dwProviderSize = 0;

    /* Set the DCB to the config settings */
    if (!SetCommState(FD(ser), &config.dcb)) {
        return -5;
    }

    /* Specify events to receive */
    if (!SetCommMask(FD(ser), EV_RXCHAR /*| EV_TXEMPTY*/)) {
        return -6;
    }

    if (!SetCommTimeouts(FD(ser), &timeouts)) {
        return -7;
    }

    return 0;
}

int FlushPort(void *ptr_ser) 
{
    upd_sercom_t *ser = (upd_sercom_t *)ptr_ser;

    if (!VALID_SER(ser))
        return ERROR_PTR;

    if (!PurgeComm(FD(ser),
        PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR)) {
        return -2;
    }

    return 0;
}

/**
 * Sends data out the serial port pointed to by the handle fd.
 *
 * @param HANDLE fd The handle to the serial port.
 * @param LPVOID tx The data to be transmitted.
 * @param DWORD len The length of the data.
 *
 * @returns 0 if successful, greater than 0 otherwise.
 */
int SendData(void *ptr_ser, const LPVOID tx, DWORD len) {
    upd_sercom_t *ser = (upd_sercom_t *)ptr_ser;
    DWORD written = 0;
    
    if (!VALID_SER(ser))
        return ERROR_PTR;

    /* Write to the port handle */
    if (!WriteFile(FD(ser), tx, len, &written, NULL)) {
        return -2;
    }

    if (written != len)
        return -3;

    return 0;
}

/**
 * Reads data from the serial port.
 *
 * @param HANDLE fd   The handle to the serial port
 * @param LPVOID rx The data buffer to be received.
 * @param DWORD len The length of the data.
 * @returns bytes received, negative value mean error code
 */
int ReadData(void *ptr_ser, LPVOID rx, DWORD len) {
    upd_sercom_t *ser = (upd_sercom_t *)ptr_ser;
    DWORD read = 0;
    //DWORD dwEvtMask;

    /*
    if (!WaitCommEvent(FD(ser), &dwEvtMask, NULL)) {
    
    }
    */

    if (!ReadFile(FD(ser), rx, 1, &read, NULL)) {
        return -2;
    }

    return read;

}

/**
 * Closes a serial port handle.
 *
 * @param HANDLE fd    The pointer to the handle of the serial port.
 * @no return  */
void ClosePort(void *ptr_ser) {
    upd_sercom_t *ser = (upd_sercom_t *)ptr_ser;

    if (!ser)
        return;

    if (ser->fd) {
        CloseHandle(ser->fd);
        free(ser);
    }
}
