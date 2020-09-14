/**
 * @filename serial.c
 * @author Darryl Pogue
 * @designer Darryl Pogue
 * @date 2010 10 20
 * @project Terminal Emulator
 *
 * 2018/09/10 Modified to adapt to linux system
 * 
 * This file contains the function implementations for serial port
 * communication.
 */
#ifdef CUPDI

#include "serial.h"
#include "error.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <hpl_sercom_config.h>

/*/////////////////////////////////////////////////////////////////////////////////////////////////////
//////// Need to modify the header file and the USART function according to the specific MCU //////////
/////////////////////////////////////////////////////////////////////////////////////////////////////*/
#include "driver_init.h"



/*/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////   The end of modification      //////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////*/

typedef struct _upd_sercom {
#define UPD_SERCOM_MAGIC_WORD 0xA5A5//'user'
    unsigned int mgwd;
    struct io_descriptor *io;
}upd_sercom_t;

#define VALID_SER(_ser) ((_ser) && (((upd_sercom_t *)(_ser))->mgwd == UPD_SERCOM_MAGIC_WORD)/* && ((upd_sercom_t *)(_ser))->io*/)
#define USART_BAUD_RATE(baud)                                                                                  \
65536 - ((65536 * 16.0f * baud) / CONF_GCLK_SERCOM4_CORE_FREQUENCY)

struct io_descriptor *iodes;
upd_sercom_t sercom;

static void tx_cb_USART_0(const struct usart_async_descriptor *const io_descr)
{
	/* Transfer completed */
}

static void rx_cb_USART_0(const struct usart_async_descriptor *const io_descr)
{
	/* Transfer completed */
}
static void err_cb_USART_0(const struct usart_async_descriptor *const io_descr)
{
	/* Transfer completed */
}
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
    upd_sercom_t* ser = &sercom;
//    int fd = 0;
	
	usart_async_register_callback(&USART_0, USART_ASYNC_TXC_CB, tx_cb_USART_0);
	usart_async_register_callback(&USART_0, USART_ASYNC_RXC_CB, rx_cb_USART_0);
	usart_async_register_callback(&USART_0, USART_ASYNC_ERROR_CB, err_cb_USART_0);
	usart_async_get_io_descriptor(&USART_0, &iodes);
	usart_async_enable(&USART_0);

    ser->mgwd = UPD_SERCOM_MAGIC_WORD;
    ser->io = iodes;

    if (SetPortState(ser, st) != 0) {
        ClosePort(ser);
    }

    return (HANDLE)ser;
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
    //upd_sercom_t *ser = (upd_sercom_t *)ptr_ser;
    //struct io_descriptor * io = ser->io;

    //if (!VALID_SER(ser))
    //    return ERROR_PTR;
			
	// Set baund rate
	usart_async_set_baud_rate(&USART_0, USART_BAUD_RATE(st->baudRate));

    /* Set databits */
	enum usart_character_size charSize;
    switch (st->byteSize) {
        case 5:
            charSize = USART_CHARACTER_SIZE_5BITS;
            break;
        case 6:
            charSize = USART_CHARACTER_SIZE_6BITS;
            break;
        case 7:
            charSize = USART_CHARACTER_SIZE_7BITS;
            break;
        case 8:
            charSize = USART_CHARACTER_SIZE_8BITS;
            break;
        default:
            charSize = USART_CHARACTER_SIZE_9BITS;
            return -6;
    }
	usart_async_set_character_size(&USART_0, charSize);

    /* Set stopbits */
	enum usart_stop_bits stopBits;
    switch (st->stopBits) {
        case ONESTOPBIT:
            stopBits = USART_STOP_BITS_ONE;
            break;
        case TWOSTOPBITS:
            stopBits = USART_STOP_BITS_TWO;
            break;
        default:
            stopBits = USART_STOP_BITS_ONE;
            return -7;
    }
	usart_async_set_stopbits(&USART_0, stopBits);

    /* Set parity */
	enum usart_parity parity;
    switch (st->parity) {
        case NOPARITY:
            parity = USART_PARITY_NONE;
            break;
        case ODDPARITY:
            parity = USART_PARITY_ODD;
            break;
        case EVENPARITY:
            parity = USART_PARITY_EVEN;
            break;
        default:
            parity = USART_PARITY_NONE;
            return -8;
    }
	usart_async_set_parity(&USART_0, parity);

    return 0;
}

int FlushPort(void *ptr_ser) 
{
    upd_sercom_t *ser = (upd_sercom_t *)ptr_ser;

    if (!VALID_SER(ser))
        return ERROR_PTR;

    usart_async_flush_rx_buffer(&USART_0);

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
int SendData(void *ptr_ser, const /*LPVOID*/u8 *tx, DWORD len) {
    upd_sercom_t *ser = (upd_sercom_t *)ptr_ser;
    DWORD written = 0;
    
    if (!VALID_SER(ser))
        return ERROR_PTR;

    /* Write to the port handle */
	written = io_write(ser->io, tx, len);
    if (written < 0) {
        return -2;
    }

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
    DWORD reading = 0;    

    reading = io_read(ser->io, rx, len);
    if (reading < 0) {
        return -2;
    }

    return reading;
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

    usart_async_disable(&USART_0);
}

#endif