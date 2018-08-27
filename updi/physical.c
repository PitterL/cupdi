#include "os/platform.h"
#include "physical.h"
#include "constants.h"

typedef struct _upd_physical{
#define UPD_PHYSICAL_MAGIC_WORD 'uphy'
    unsigned int mgwd;  //magic word
    void *ser;
    SER_PORT_STATE_T stat;
    int ibdly;  //delay ms for updi bus transfer switch
}upd_physical_t;

#define VALID_PHY(_phy) ((_phy) && ((_phy)->mgwd == UPD_PHYSICAL_MAGIC_WORD))
#define SER(_phy) ((HANDLE)_phy->ser)

/**
* Initialises a serial port handle for reading and writing
*
* @param const char * The name of the serial port to open.
* @param int baud the baudrate of the port
* @returns void *fd  The upd_physical_t pointer to the handle which will be initialised to the open
*   serial port connection.
*/
void *updi_physical_init(const char *port, int baud)
{
    void *ser;
    upd_physical_t *phy = NULL;
    SER_PORT_STATE_T stat;
    int result;

    _loginfo_i("<PHY> Opening port %s, baudrate %d", port, baud);

    stat.baudRate = baud;
    stat.byteSize = 8;
    stat.stopBits = TWOSTOPBITS;
    stat.parity = EVENPARITY;
    ser = (void *)OpenPort(port, &stat);
    if (ser) {
        phy = (upd_physical_t *)malloc(sizeof(*phy));
        phy->mgwd = UPD_PHYSICAL_MAGIC_WORD;
        phy->ser = ser;
        phy->ibdly = 1;
        memcpy(&phy->stat, &stat, sizeof(stat));
        
        // send an initial break as handshake
        result = phy_send_byte(phy, UPDI_BREAK);
        if (result) {
            _loginfo_i("phy_send_byte failed %d", result);
            updi_physical_deinit(phy);
            phy = NULL;
        }
    }
    
    return phy;
}

void updi_physical_deinit(void *ptr_phy)
{
    upd_physical_t *phy = (upd_physical_t *)ptr_phy;
    if (!VALID_PHY(phy))
        return;

    _loginfo_i("<PHY> Deinit");

    if (phy->ser) {
        ClosePort(SER(phy));
    }
    free(phy);
}

int phy_send_double_break(void *ptr_phy)
{
    /*
    Sends a double break to reset the UPDI port
    BREAK is actually just a slower zero frame
    A double break is guaranteed to push the UPDI state
    machine into a known state, albeit rather brutally
    */
    upd_physical_t * phy = (upd_physical_t *)ptr_phy;
    SER_PORT_STATE_T stat;
    u8 data[2];
    int result;

    if (!VALID_PHY(phy))
        return ERROR_PTR;

    _loginfo_i("<PHY> Sending double break");
    /*
        # Re - init at a lower baud
        # At 300 bauds, the break character will pull the line low for 30ms
        # Which is slightly above the recommended 24.6ms
    */  
    stat.baudRate = 300;
    stat.byteSize = 8;
    stat.stopBits = ONESTOPBIT;
    stat.parity = EVENPARITY;
    result = SetPortState(SER(phy), &stat);
    if (result) {
        _loginfo_i("SetPortState failed %d", result);
        return -2;
    }

    /*Send two break characters, with 1 stop bit in between */
    data[0] = UPDI_BREAK;
    data[1] = UPDI_BREAK;
    result = SendData(SER(phy), data, 2);
    if (result) {
        _loginfo_i("SendData failed %d", result);
        return -3;
    }
    /*Wait for the double break end*/
    result = ReadData(SER(phy), data, 2);
    if (result != 2) {
        _loginfo_i("ReadData failed %d", result);
        return -4;
    }

    /*Re - init at the real baud*/
    result = SetPortState(SER(phy), &phy->stat);
    if (result) {
        _loginfo_i("re-SetPortState failed %d", result);
        return -5;
    }

    return 0;
}

int phy_send(void *ptr_phy, const u8 *data, int len)
{
    /*
        Sends a char array to UPDI with inter - byte delay
        Note that the byte will echo back
     */
    upd_physical_t * phy = (upd_physical_t *)ptr_phy;
    u8 val;
    int result;

    if (!VALID_PHY(phy))
        return ERROR_PTR;

    _loginfo("<PHY> Send", data, len);

    result = FlushPort(SER(phy));
    if (result) {
        _loginfo_i("FlushPort failed %d", result);
    }

    for (int i = 0; i < len; i++) {
        /* Send */
        val = data[i];
        result = SendData(SER(phy), &val, 1);   //Todo: should check whether we could send all data once
        if (result) {
            _loginfo_i("SendData failed %d", result);
            return -2;
        }
        /* Echo */
        result = ReadData(SER(phy), &val, 1);
        if (result != 1) {
            _loginfo_i("ReadData failed %d", result);
            return -3;
        }

        if (data[i] != val) {
            _loginfo_i("ReadData mismatch %02x(%02x) located = %d", val, data[i], i);
            return -4;
        }

        msleep(phy->ibdly);
    }

    return 0;
}

int phy_send_byte(void *ptr_phy, u8 val)
{
    return phy_send(ptr_phy, &val, 1);
}

int phy_receive(void *ptr_phy, u8 *data, int len)
{
    /*
        Receives a frame of a known number of chars from UPDI
    */
    upd_physical_t * phy = (upd_physical_t *)ptr_phy;
    int result;
    int i = 0;
    int retry = 1;

    if (!VALID_PHY(phy))
        return ERROR_PTR;

    /* For each byte */
    while(i < len) {
        /* Read */
        result = ReadData(SER(phy), &data[i], 1);   //Todo: should check whether we could read all data once
        if (result == 1) {
            i++;
        }else {
            _loginfo_i("ReadData failed %d", result);
            retry--;
        }
        
        if (retry < 0) {
            _loginfo_i("ReadData timeout");
            break;
        }
    }

    _loginfo("<PHY> Receive", data, len);

    return i;
}

u8 phy_receive_byte(void *ptr_phy)
{
    u8 resp = 0xFF;    //default resp
    int result;

    result = phy_receive(ptr_phy, &resp, 1);
    if (result != 1) {
        _loginfo_i("phy_send failed", result);
    }

    return resp;
}

int phy_transfer(void *ptr_phy, const u8 *wdata, int wlen, u8 *rdata, int rlen)
{
    int result;

    _loginfo_i("<PHY> Transfer w %d, r %d", wlen, rlen);

    result = phy_send(ptr_phy, wdata, wlen);
    if (result) {
        _loginfo_i("phy_send failed %d", result);
        return -2;
    }

    result = phy_receive(ptr_phy, rdata, rlen);
    if (result != rlen) {
        _loginfo_i("phy_receive failed %d", result);
        return -3;
    }

    return rlen;
}

int phy_sib(void *ptr_phy, u8 *data, int len) 
{
    /*
        System information block is just a string coming back from a SIB command
    */

    upd_physical_t * phy = (upd_physical_t *)ptr_phy;
    const u8 val[2] = { UPDI_PHY_SYNC, UPDI_KEY | UPDI_KEY_SIB | UPDI_SIB_16BYTES};
    const int sib_size = 16;
    int result;

    if (!VALID_PHY(phy))
        return ERROR_PTR;

    _loginfo_i("<PHY> Sib");

    if (len > sib_size)
        len = sib_size;

    result = phy_transfer(phy, val, 2, data, len);
    if (result != len) {
        _loginfo_i("phy_transfer failed %d", result);
        return -3;
    }

    return 0;
}