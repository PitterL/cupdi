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
#include "serial.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>

typedef struct _upd_sercom {
#define UPD_SERCOM_MAGIC_WORD 0xA5A5//'user'
    unsigned int mgwd;
    int fd;
}upd_sercom_t;

#define VALID_SER(_ser) ((_ser) && (((upd_sercom_t *)(_ser))->mgwd == UPD_SERCOM_MAGIC_WORD) && ((upd_sercom_t *)(_ser))->fd)
#define FD(_ser) ((int)(_ser)->fd)

static speed_t speed_arr[] = {B0, B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800,
                              B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400};
static int name_arr[] = {0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800,
                        2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400};

static speed_t GetBaudRate(int baudrate)
{
    int i;

    for (i = sizeof(speed_arr) / sizeof(speed_t) - 1; i >= 0; i--) {
        if (baudrate == name_arr[i]) {
            return speed_arr[i];
        }
    }

    return B0;
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
    upd_sercom_t *ser;
    int fd = 0;

    /* open tty device */
    fd = open(port, O_RDWR | O_NOCTTY/* | O_NONBLOCK*/);
    if (fd < 0) {
        printf("Could not open tty device (%s)", strerror(errno));
    }

    ser = (upd_sercom_t *)malloc(sizeof(*ser));
    if (!ser) {
        flock(fd, LOCK_UN);
        close(fd);
        return 0;
    }

    ser->mgwd = UPD_SERCOM_MAGIC_WORD;
    ser->fd = fd;

    if (SetPortState(ser, st) != 0) {
        ClosePort(ser);
        ser = NULL;
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
    upd_sercom_t *ser = (upd_sercom_t *)ptr_ser;
    struct termios tio;
    int status;
    int fd = FD(ser);

    if (!VALID_SER(ser))
        return ERROR_PTR;

    if (!isatty(fd)) {
        printf("Not a tty device\n");
        return -2;
    }

    /* Lock device file */
    status = flock(fd, LOCK_EX | LOCK_NB);
    if ((status == -1) && (errno == EWOULDBLOCK))
    {
        printf("Device file is locked by another process\n");
        return -3;
    }

    /* Flush stale I/O data (if any) */
    tcflush(fd, TCIOFLUSH);

    memset(&tio, 0, sizeof(tio));
    status = cfsetispeed(&tio, GetBaudRate(st->baudRate));  // Set input speed
    if (status == -1) {
        printf("Could not configure input speed (%s)s\n", strerror(errno));
        return -4;
    }
    status = cfsetospeed(&tio, GetBaudRate(st->baudRate));  // Set output speed
    if (status == -1) {
        printf("Could not configure output speed (%s)s\n", strerror(errno));
        return -5;
    }

    /* Flush stale I/O data (if any) */
    tcflush(fd, TCIOFLUSH);

    /* Set databits */
    tio.c_cflag &= ~CSIZE;
    switch (st->byteSize) {
        case 5:
            tio.c_cflag |= CS5;
            break;
        case 6:
            tio.c_cflag |= CS6;
            break;
        case 7:
            tio.c_cflag |= CS7;
            break;
        case 8:
            tio.c_cflag |= CS8;
            break;
        default:
            printf("Invalid data bitss\n");
            return -6;
    }

    /* Set stopbits */
    switch (st->stopBits) {
        case ONESTOPBIT:
            tio.c_cflag &= ~CSTOPB;
            break;
        case TWOSTOPBITS:
            tio.c_cflag |= CSTOPB;
            break;
        default:
            printf("Invalid stop bitss\n");
            return -7;
    }

    /* Set parity */
    switch (st->parity) {
        case NOPARITY:
            tio.c_cflag &= ~PARENB;
            break;
        case ODDPARITY:
            tio.c_cflag |= PARENB;
            tio.c_cflag |= PARODD;
            break;
        case EVENPARITY:
            tio.c_cflag |= PARENB;
            tio.c_cflag &= ~PARODD;
            break;
        default:
            printf("Invalid paritys\n");
            return -8;
    }

    /* Set flow control -- none */   
    tio.c_cflag &= ~CRTSCTS;
    tio.c_iflag &= ~(IXON | IXOFF | IXANY);

    //tio.c_cflag |= CRTSCTS;
    //tio.c_iflag &= ~(IXON | IXOFF | IXANY);

     /* Control, input, output, local modes for tty device */
    tio.c_cflag |= CLOCAL | CREAD;
    //tio.c_oflag = 0;
    //tio.c_lflag = 0;
    tio.c_oflag &= ~OPOST; /*Output*/
    tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  /* Non Cannonical mode   

    /* Control characters */
    tio.c_cc[VTIME] = 10; // Inter-character timer unused 1/10s
    tio.c_cc[VMIN]  = 0; // Blocking read until 1 character received

    /* Flush stale I/O data (if any) */
    tcflush(fd, TCIFLUSH);

    /* Activate new port settings */
    status = tcsetattr(fd, TCSANOW, &tio);
    if (status == -1)
    {
        printf("Could not apply port settings (%s)s\n", strerror(errno));
        return -9;
    }

    return 0;
}

int FlushPort(void *ptr_ser) 
{
    upd_sercom_t *ser = (upd_sercom_t *)ptr_ser;

    if (!VALID_SER(ser))
        return ERROR_PTR;

    tcflush(FD(ser), TCIOFLUSH);

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
    written = write(FD(ser), tx, len);
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
/*    WORD fs_sel;
    fd_set fs_read;
    struct timeval time;

    FD_ZERO(&fs_read);
    FD_SET(FD(ser), &fs_read);

    time.tv_sec = 0;
    time.tv_usec = 10000;

    fs_sel = select(FD(ser) + 1, &fs_read, NULL, NULL, &time);
    if (fs_sel) {
        reading = read(FD(ser), rx, len);
        if (reading < 0) {
            return -2;
        }

        return reading;
    } else {
        return -3;
    }
*/    

    reading = read(FD(ser), rx, len);
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

    if (ser->fd) {
        flock(FD(ser), LOCK_UN);
        close(FD(ser));
        free(ser);
    }
}
