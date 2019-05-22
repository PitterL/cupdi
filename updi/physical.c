/*
Copyright(c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""
*/

#include "os/platform.h"
#include "physical.h"
#include "constants.h"

/*
    PHY level memory struct
    @mgwd: magicword
    @ser: pointer to sercom object
    @stat: store sercom parameter
    @ibdly: interval between each transfer action
*/
typedef struct _upd_physical{
#define UPD_PHYSICAL_MAGIC_WORD 0xE1E1 //'uphy'
    unsigned int mgwd;  //magic word
    void *ser;
    SER_PORT_STATE_T stat;
    int ibdly;  //delay ms for updi bus transfer switch
}upd_physical_t;

#define VALID_PHY(_phy) ((_phy) && ((_phy)->mgwd == UPD_PHYSICAL_MAGIC_WORD))
#define SER(_phy) ((HANDLE)_phy->ser)

/*
    PHY object init
    @port: serial port name of Window or Linux
    @baud: baudrate
    @return LINK ptr, NULL if failed
*/
void *updi_physical_init(const char *port, int baud)
{
    void *ser;
    upd_physical_t *phy = NULL;
    SER_PORT_STATE_T stat;
    //u8 data[] = { UPDI_BREAK };
    //int result;

    DBG_INFO(PHY_DEBUG, "<PHY> Opening port %s, baudrate %d", port, baud);

    stat.baudRate = baud;
    stat.byteSize = 8;
    stat.stopBits = TWOSTOPBITS;
    stat.parity = EVENPARITY;
    ser = (void *)OpenPort(port, &stat);
    if (ser) {
        phy = (upd_physical_t *)malloc(sizeof(*phy));
        phy->mgwd = UPD_PHYSICAL_MAGIC_WORD;
        phy->ser = ser;
        phy->ibdly = 0;
        stat.baudRate = baud;
        memcpy(&phy->stat, &stat, sizeof(stat));
        
        //send an initial break as handshake
        /*
        result = phy_send(phy, data, 1);
        if (result) {
            DBG_INFO(PHY_DEBUG, "<PHY> Init: phy_send failed %d", result);
            return NULL;
        }
        
        // send an initial double break as handshake
        result = phy_send_double_break(phy);
        if (result) {
            DBG_INFO(PHY_DEBUG, "phy_send_double_break failed %d", result);
            updi_physical_deinit(phy);
            return NULL;
        }
		*/
    }
    else {
        DBG_INFO(PHY_DEBUG, "<PHY> Init: OpenPort %s failed ", port);
    }
    
    return phy;
}

/*
    PHY object destroy
    @ptr_phy: APP object pointer, acquired from updi_physical_init()
    @no return
*/
void updi_physical_deinit(void *ptr_phy)
{
    upd_physical_t *phy = (upd_physical_t *)ptr_phy;
    if (!VALID_PHY(phy))
        return;

    DBG_INFO(PHY_DEBUG, "<PHY> Deinit");

    if (phy->ser) {
        ClosePort(SER(phy));
    }
    free(phy);
}

/*
PHY set Sercom baudrate
@ptr_phy: APP object pointer, acquired from updi_physical_init()
@return 0 successful, other value if failed
*/
int phy_set_baudrate(void *ptr_phy, int baud)
{
    upd_physical_t *phy = (upd_physical_t *)ptr_phy;
    SER_PORT_STATE_T stat;
    int result;

    if (!VALID_PHY(phy))
        return ERROR_PTR;

    DBG_INFO(PHY_DEBUG, "<PHY> Set Baudrate");

    memcpy(&stat, &phy->stat, sizeof(stat));

    stat.baudRate = baud;
    result = SetPortState(SER(phy), &stat);
    if (result) {
        DBG_INFO(PHY_DEBUG, "<PHY> set Baud %d failed %d", baud, result);
        return -2;
    }

    memcpy(&phy->stat, &stat, sizeof(stat));
    
    return 0;
}

/*
PHY send break
@ptr_phy: APP object pointer, acquired from updi_physical_init()
@return 0 successful, other value if failed
*/
int phy_send_break(void *ptr_phy)
{
    upd_physical_t * phy = (upd_physical_t *)ptr_phy;
    u8 data[] = { UPDI_BREAK };
    int result;

    if (!VALID_PHY(phy))
        return ERROR_PTR;

    DBG_INFO(PHY_DEBUG, "<PHY> Break: Sending break");

    result = phy_send(phy, data, 1);
    if (result) {
        DBG_INFO(PHY_DEBUG, "<PHY> Send Break: phy_send failed %d", result);
        return -2;
    }

    return 0;
}

/*
    PHY send doule break
    @ptr_phy: APP object pointer, acquired from updi_physical_init()
    @return 0 successful, other value if failed
*/
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
    u8 data[] = { UPDI_BREAK, UPDI_BREAK };
    int result;

    if (!VALID_PHY(phy))
        return ERROR_PTR;

    DBG_INFO(PHY_DEBUG, "<PHY> D-Break: Sending double break");
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
        DBG_INFO(PHY_DEBUG, "<PHY> D-Break: SetPortState failed %d", result);
        return -2;
    }

    /*Send two break characters, with 1 stop bit in between */
    result = phy_send(phy, data, 2);
    if (result) {
        DBG_INFO(PHY_DEBUG, "<PHY> D-Break: phy_send failed %d", result);
        return -3;
    }

    /*Re - init at the real baud*/
    result = SetPortState(SER(phy), &phy->stat);
    if (result) {
        DBG_INFO(PHY_DEBUG, "<PHY> D-Break: re-SetPortState failed %d", result);
        return -5;
    }

    return 0;
}

/*
    PHY send data by each byte
    @ptr_phy: APP object pointer, acquired from updi_physical_init()
    @data: data to be sent
    @len: data lenght
    @return 0 successful, other value if failed
*/
int phy_send_each(void *ptr_phy, const u8 *data, int len)
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

    DBG(PHY_DEBUG, "<PHY> Send:", data, len, "0x%02x ");

    result = FlushPort(SER(phy));
    if (result) {
        DBG_INFO(PHY_DEBUG, "<PHY> Send: FlushPort failed %d", result);
    }

    for (int i = 0; i < len; i++) {
        /* Send */
        val = data[i];
        result = SendData(SER(phy), &val, 1);   //Todo: should check whether we could send all data once
        if (result) {
            DBG_INFO(PHY_DEBUG, "<PHY> Send: SendData failed %d", result);
            return -2;
        }
        
        /* Echo */
        result = ReadData(SER(phy), &val, 1);
        if (result != 1) {
            DBG_INFO(PHY_DEBUG, "<PHY> Send: ReadData failed %d", result);
            return -3;
        }

        if (data[i] != val) {
            DBG_INFO(PHY_DEBUG, "<PHY> Send: ReadData mismatch %02x(%02x) located = %d", val, data[i], i);
            return -4;
        }

        if (phy->ibdly)
            msleep(phy->ibdly);
    }

    return 0;
}

/*
PHY send data
@ptr_phy: APP object pointer, acquired from updi_physical_init()
@data: data to be sent
@len: data lenght
@return 0 successful, other value if failed
*/
int phy_send(void *ptr_phy, const u8 *data, int len)
{
    /*
    Sends a char array to UPDI with inter - byte delay
    Note that the byte will echo back
    */
    upd_physical_t * phy = (upd_physical_t *)ptr_phy;
    int i, result;
    u8 *rbuf;

    if (!VALID_PHY(phy))
        return ERROR_PTR;

    DBG(PHY_DEBUG, "<PHY> Send:", data, len, "0x%02x ");

    rbuf = malloc(len);
    if (!rbuf) {
        DBG_INFO(PHY_DEBUG, "<PHY> Send: malloc rbuf(%d) failed", len);
        return -2;
    }

    result = FlushPort(SER(phy));
    if (result) {
        DBG_INFO(PHY_DEBUG, "<PHY> Send: FlushPort failed %d", result);
    }

    /* Send */
    result = SendData(SER(phy), data, len); 
    if (result) {
        DBG_INFO(PHY_DEBUG, "<PHY> Send: SendData (%d) failed %d", len, result);
        result = -3;
    }

    /* Echo */
    if (result == 0) {
        result = ReadData(SER(phy), rbuf, len);
        if (result != len) {
            DBG_INFO(PHY_DEBUG, "<PHY> Send: ReadData (%d) failed %d", len, result);
            result = -4;
        }
    }

    if (result == len) {
        for (i = 0; i < len; i++) {
            if (data[i] != rbuf[i]) {
                DBG_INFO(PHY_DEBUG, "<PHY> Send: ReadData mismatch %02x(%02x) located = %d", rbuf[i], data[i], i);
                result = -5;
                break;
            }
        }
    }

    if (phy->ibdly)
        msleep(phy->ibdly);

    free(rbuf);

    if (result == len)
        return 0;
    else
        return result;
}

/*
    PHY send data
    @ptr_phy: APP object pointer, acquired from updi_physical_init()
    @data: data to be sent
    @len: data lenght
    @return 0 successful, other value if failed
*/
int phy_send_byte(void *ptr_phy, u8 val)
{
    return phy_send_each(ptr_phy, &val, 1);
}

/*
    PHY receive data by each byte
    @ptr_phy: APP object pointer, acquired from updi_physical_init()
    @data: data buffer to receive
    @len: data lenght
    @return 0 successful, other value if failed
*/
int phy_receive_each(void *ptr_phy, u8 *data, int len)
{
    /*
        Receives a frame of a known number of chars from UPDI
    */
    upd_physical_t * phy = (upd_physical_t *)ptr_phy;
    int result;
    int i = 0;
    int retry = 0;

    if (!VALID_PHY(phy))
        return ERROR_PTR;

    /* For each byte */
    while(i < len) {
        /* Read */
        result = ReadData(SER(phy), &data[i], 1);   //Todo: should check whether we could read all data once
        if (result == 1) {
            i++;
        }else {
            DBG_INFO(PHY_DEBUG, "<PHY> Recv: ReadData failed %d", result);
            retry--;
        }
        
        if (retry < 0) {
            DBG_INFO(PHY_DEBUG, "<PHY> Recv: ReadData timeout");
            break;
        }
    }

    if (i)
        DBG(PHY_DEBUG, "<PHY> Recv: Received(%d/%d): ", data, i, "0x%02x ", i, len);

    return i;
}

/*
PHY receive data
@ptr_phy: APP object pointer, acquired from updi_physical_init()
@data: data buffer to receive
@len: data lenght
@return 0 successful, other value if failed
*/
int phy_receive(void *ptr_phy, u8 *data, int len)
{
    /*
    Receives a frame of a known number of chars from UPDI
    */
    upd_physical_t * phy = (upd_physical_t *)ptr_phy;
    int result;

    if (!VALID_PHY(phy))
        return ERROR_PTR;

    /* Read */
    result = ReadData(SER(phy), data, len);
    if (result != len) {
        DBG(PHY_DEBUG, "<PHY> Recv: Received(%d/%d) failed: ", data, result, "0x%02x ", result, len);
    }

    DBG(PHY_DEBUG, "<PHY> Recv: Received(%d/%d): ", data, result, "0x%02x ", result, len);

    return result;
}

/*
    PHY receive one byte data
    @ptr_phy: APP object pointer, acquired from updi_physical_init()
    @return data received, oxff if failed(conflicted with the data content original 0xFF)
*/
u8 phy_receive_byte(void *ptr_phy)
{
    u8 resp = 0xFF;    //default resp
    int result;

    result = phy_receive_each(ptr_phy, &resp, 1);
    if (result != 1) {
        DBG_INFO(PHY_DEBUG, "<PHY> Recv one: phy_receive failed, Got %d bytes", result);
    }

    return resp;
}

/*
    PHY transfer data
    @ptr_phy: APP object pointer, acquired from updi_physical_init()
    @wdata: data buffer to send
    @wlen: send length
    @rdata: data buffer to receive
    @len: receiving lenght
    @return 0 successful, other value if failed
*/
int phy_transfer(void *ptr_phy, const u8 *wdata, int wlen, u8 *rdata, int rlen)
{
    int result;
    int retry = 0;  //determine retries in higher level by protocol used

    DBG_INFO(PHY_DEBUG, "<PHY> Transfer: Write %d bytes, Read %d bytes", wlen, rlen);

    do {
        result = phy_send(ptr_phy, wdata, wlen);
        if (result) {
            DBG_INFO(PHY_DEBUG, "<PHY> Transfer: phy_send failed %d", result);
            result = -2;
        }
        else {
            result = phy_receive(ptr_phy, rdata, rlen);
            if (result != rlen) {
                DBG_INFO(PHY_DEBUG, "<PHY> Transfer: phy_receive failed, Got %d bytes", result);
                result = -3;
            }
            else {
                break;
            }
        }

        if (result < 0)
            retry--;

    } while (retry >= 0);

    return result;
}

/*
    PHY transfer data
    @ptr_phy: APP object pointer, acquired from updi_physical_init()
    @data: data buffer to store SIB
    @len: SIB lenght
    @return 0 successful, other value if failed
*/
int phy_sib(void *ptr_phy, u8 *data, int len) 
{
    /*
        System information block is just a string coming back from a SIB command
    */

    upd_physical_t * phy = (upd_physical_t *)ptr_phy;
    const u8 val[] = { UPDI_PHY_SYNC, UPDI_KEY | UPDI_KEY_SIB | UPDI_SIB_16BYTES};
    const int sib_size = 16;
    int result;

    if (!VALID_PHY(phy))
        return ERROR_PTR;

    DBG_INFO(PHY_DEBUG, "<PHY> Sib");

    if (len > sib_size)
        len = sib_size;

    result = phy_transfer(phy, val, sizeof(val), data, len);
    if (result != len) {
        DBG_INFO(PHY_DEBUG, "<PHY> Sib: phy_transfer failed %d", result);
        return -3;
    }

    return 0;
}