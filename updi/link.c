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
#include "link.h"
#include "constants.h"

/*
    LINK level memory struct
    @mgwd: magicword
    @phy: pointer to phy object
*/
typedef struct _upd_datalink {
#define UPD_DATALINK_MAGIC_WORD 0xC3C3 //'ulin'
    unsigned int mgwd;  //magic word
    void *phy;
}upd_datalink_t;

/*
    Macro definition of APP level
    @VALID_LINK(): check whether valid LINK object
    @PHY(): get phy object ptr
*/
#define VALID_LINK(_link) ((_link) && ((_link)->mgwd == UPD_DATALINK_MAGIC_WORD))
#define PHY(_link) ((_link)->phy)

/*
    LINK object init
    @port: serial port name of Window or Linux
    @baud: baudrate
	@guard: guard time of when the transmission direction switches
    @return LINK ptr, NULL if failed
*/
void *updi_datalink_init(const char *port, int baud, int guard)
{
    upd_datalink_t *link = NULL;
    void *phy;
	size_t size;
    int result, retry = 3;

    DBG_INFO(LINK_DEBUG, "<LINK> init link");
    
    phy = updi_physical_init(port, UPDI_BAUTRATE_DEFAULT /*dummy*/);
    if (phy) {
		size = sizeof(*link);
        link = (upd_datalink_t *)malloc(size);
		if (!link) {
			DBG_INFO(LINK_DEBUG, "<LINK> link malloc memory(%d) failed", size);
			updi_physical_deinit(phy);
			return NULL;
		}

		memset(link, 0, size);
        link->mgwd = UPD_DATALINK_MAGIC_WORD;
        link->phy = (void *)phy;

        do {
          result = link_set_init(link, baud, guard);
          if (result) {
              DBG_INFO(LINK_DEBUG, "link_set_init failed %d, retry=%d", result, retry);
              phy_send_double_break(phy);
              continue;
          }

          result = link_check(link);
          if (result) {
              DBG_INFO(LINK_DEBUG, "link_check failed %d, retry=%d", result, retry);
              phy_send_double_break(phy);
              continue;
          }
        } while(retry-- && result);

        if (result) {
          updi_datalink_deinit(link);
          return NULL;
        }
    }

    return link;
}

/*
    LINK object destroy
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @no return
*/
void updi_datalink_deinit(void *link_ptr)
{
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    if (VALID_LINK(link)) {
        DBG_INFO(LINK_DEBUG, "<LINK> deinit link");

        updi_physical_deinit(PHY(link));
        free(link);
    }
}

/*
    LINK Set the inter-byte delay bit and disable collision detection
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @baud: baudrate to set
	@guard: guard time of when the transmission direction switches
    @return 0 successful, other value if failed
*/
int link_set_init(void *link_ptr, int baud, int guard)
{
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    u8 clksel, resp, val;
	int baud_first = baud;
	int i, result;
	
    if (!VALID_LINK(link))
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> link set init");

	if (baud > UPDI_BAUTRATE_IN_CLK_4M_MAX) {
		baud_first = UPDI_BAUTRATE_DEFAULT;
	}

    result = phy_set_baudrate(PHY(link), baud_first);
    if (result) {
        DBG_INFO(LINK_DEBUG, "phy_set_baudrate default failed %d", result);
        return -2;
    }

	// Disable collision detection
	DBG_INFO(LINK_DEBUG, "<LINK> Disable collision detection");
	result = link_stcs(link, UPDI_CS_CTRLB, 1 << UPDI_CTRLB_CCDETDIS_BIT);
	if (result) {
		DBG_INFO(LINK_DEBUG, "link_stcs UPDI_CS_CTRLB failed %d", result);
		return -3;
	}
    
    // Set the inter-byte delay bit and Gaurd Delay time 
	DBG_INFO(LINK_DEBUG, "<LINK> Set the inter-byte delay bit and Gaurd Delay time");
	val = UPDI_CTRLA_GTVAL_16_CYCLES;
	for (i = 0; i < UPDI_CTRLA_GTVAL_TYPES; i++) {
		if (guard >= (1 << (UPDI_CTRLA_GTVAL_TYPES - i))) {
			val = i;
			break;
		}
	}

	// Set baudrate and clock
	if (baud <= UPDI_BAUTRATE_IN_CLK_4M_MAX) {
		clksel = UPDI_ASI_CTRLA_CLKSEL_4M;
	}
	else if (baud <= UPDI_BAUTRATE_IN_CLK_8M_MAX) {
		clksel = UPDI_ASI_CTRLA_CLKSEL_8M;
	}
	else if (baud <= UPDI_BAUTRATE_IN_CLK_16M_MAX) {
		clksel = UPDI_ASI_CTRLA_CLKSEL_16M;
#ifdef UPDI_CTRLA_ENABLE_INTER_BYTE
		// Enable inter byte
		val |= (1 << UPDI_CTRLA_IBDLY_BIT);
#endif
	}
	else {
		DBG_INFO(LINK_DEBUG, "Unsupported baudrate for UPDI clk %d, max 0.9Mhz", baud);
		return -6;
	}

    result = link_stcs(link, UPDI_CS_CTRLA, val);
    if (result) {
        DBG_INFO(LINK_DEBUG, "link_stcs UPDI_CS_CTRLA failed %d", result);
        return -4;
    }

    // Set clock source
    DBG_INFO(LINK_DEBUG, "<LINK> Check and set clock source to %d", clksel);
    result = _link_ldcs(link_ptr, UPDI_ASI_CTRLA, &resp);
    if (result) {
        DBG_INFO(LINK_DEBUG, "_link_ldcs failed %d", result);
        return -7;
    } 
    else if (resp != clksel) {
        result = link_stcs(link, UPDI_ASI_CTRLA, clksel);
        if (result) {
            DBG_INFO(LINK_DEBUG, "link_stcs UPDI_ASI_CTRLA failed %d", result);
            return -8;
        }
    }

    // Baudrate
    result = phy_set_baudrate(PHY(link), baud);
    if (result) {
        DBG_INFO(LINK_DEBUG, "phy_set_baudrate failed %d", result);
        return -9;
    }

    return 0;
}

/*
    LINK check whether device is connected 
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @return 0 successful, other value if failed
*/
int link_check(void *link_ptr)
{
    /*
        Check UPDI by loading CS STATUSA
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    u8 resp;
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;
    
    DBG_INFO(LINK_DEBUG, "<LINK> link check");

    DBG_INFO(LINK_DEBUG, "<LINK> link get StatusB");
    result = _link_ldcs(link_ptr, UPDI_CS_STATUSB, &resp);
    if (result) {
        DBG_INFO(LINK_DEBUG, "UPDI not ready");
        return -2;
    }

    if (resp) {
        DBG_INFO(LINK_DEBUG, "UPDI response status error %d at StatusB, may send a BREAK", resp);
        switch (resp) {
        case 0x1:
            DBG_INFO(LINK_ERROR, "[Parity Error] Wrong sampling of the parity bit");
            break;
        case 0x2:
            DBG_INFO(LINK_ERROR, "[Frame Error] Wrong sampling of frame Stop bits");
            break;
        case 0x3:
            DBG_INFO(LINK_ERROR, "[Access Layer Time-out Error] UPDI can get no data or response from the Access layer.   \
                Examples of error cases are system domain in Sleep or   \
                system domain Reset.");
            break;
        case 0x4:
            DBG_INFO(LINK_ERROR, "[Clock Recovery Error] Wrong sampling of frame Start bit");
            break;
        case 0x7:
            DBG_INFO(LINK_ERROR, "[Contention Error] Signalize Driving Contention on the UPDI RXD/TXD line");
            break;
        default:
            DBG_INFO(LINK_ERROR, "[Unknown] UPDI Error Signature unknown");
        }
        //phy_send_break(PHY(link));

        //FIXME: how to handle this?
    }

    DBG_INFO(LINK_DEBUG, "<LINK> link get StatusA");
    result = _link_ldcs(link_ptr, UPDI_CS_STATUSA, &resp);
    if (result) {
        DBG_INFO(LINK_DEBUG, "UPDI not ready");
        return -2;
    }
    
    if (resp == 0) {
        DBG_INFO(LINK_DEBUG, "UPDI not OK - reinitialisation required");
        return -3;
    }
    
    DBG_INFO(LINK_DEBUG, "UPDI init OK (%02x)", resp);
    return 0;
}

/*
    LINK read udpi control register
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @address: reg address
    @data: output 8bit buffer
    @return 0 successful, other value if failed
*/
int _link_ldcs(void *link_ptr, u8 address, u8 *data)
{
    /*
        Load data from Control / Status space
        default return 0 if error
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    u8 cmd[] = { UPDI_PHY_SYNC, UPDI_LDCS | (address & 0x0F) };
    u8 resp;
    int result;

    if (!VALID_LINK(link) || !data)
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> LDCS from 0x%02x", address);
    result = phy_transfer(PHY(link), cmd, sizeof(cmd), &resp, sizeof(resp));
    if (result != sizeof(resp)) {
        DBG_INFO(LINK_DEBUG, "phy_transfer failed %d", result);
        return -2;
    }

    
    *data = resp;

    return 0;
}

/*
    LINK read udpi control register capsule
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @address: reg address
    @return reg value if successful, zero if not accessiable(this will confict with reg value zero)
*/
u8 link_ldcs(void *link_ptr, u8 address)
{
    u8 resp;
    int result;

    result = _link_ldcs(link_ptr, address, &resp);
    if (result) {
        DBG_INFO(LINK_DEBUG, "_link_ldcs failed %d", result);
        return 0;
    }

    return resp;
}

/*
    LINK write udpi control register
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @address: reg address
    @value: reg value
    @return 0 successful, other value if failed
*/
int link_stcs(void *link_ptr, u8 address, u8 value)
{
    /*
        Store a value to Control / Status space
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    u8 cmd[] = { UPDI_PHY_SYNC, UPDI_STCS | (address & 0x0F), value };
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> STCS to 0x02x", address);

    result = phy_send(PHY(link), cmd, sizeof(cmd));
    if (result) {
        DBG_INFO(LINK_DEBUG, "phy_send failed %d", result);
        return -2;
    }

    return 0;
}

/*
    LINK read 8bit target register by direct mode
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @address: target address
    @val: output buffer
	@is24bit: whether 24bit address mode
    @return 0 successful, other value if failed
*/
int _link_ld(void *link_ptr, u32 address, u8 *val, bool is24bit)
{
    /*
        Load a single byte direct from a 16 - bit address
        return 0 if error
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
	const u8 *cmd, command24[] = { UPDI_PHY_SYNC, UPDI_LDS | UPDI_ADDRESS_24 | UPDI_DATA_8, address & 0xFF, (address >> 8) & 0xFF, (address >> 16) & 0xFF };
    const u8 command16[] = { UPDI_PHY_SYNC, UPDI_LDS | UPDI_ADDRESS_16 | UPDI_DATA_8, address & 0xFF, (address >> 8) & 0xFF};
    u8 resp;
    int cmdlen, result;

    if (!VALID_LINK(link) || !val)
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> LD from {%04X}", address);
  
	if (is24bit) {
		cmd = command24;
		cmdlen = sizeof(command24);
	}
	else {
		cmd = command16;
		cmdlen = sizeof(command16);
	}

    result = phy_transfer(PHY(link), cmd, cmdlen, &resp, sizeof(resp));
    if (result != sizeof(resp)) {
        DBG_INFO(LINK_DEBUG, "phy_transfer failed %d", result);
        return -2;
    }

    *val = resp;

    return 0;
}

/*
    LINK read 8bit data register by direct mode, capsule
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @address: target address
	@is24bit: whether 24bit address mode
    @return target data if successful, zero if not accessiable(this will confict with target data zero)
*/
u8 link_ld(void *link_ptr, u32 address, bool is24bit)
{
    u8 resp = 0;

    _link_ld(link_ptr, address, &resp, is24bit);

    return resp;
}

/*
    LINK read 16bit data by direct mode
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @address: target address
    @val: output buffer
	@is24bit: whether 24bit address mode
    @return 0 successful, other value if failed
*/
int _link_ld16(void *link_ptr, u32 address, u16 *val, bool is24bit)
{
    /*
    Load a 2 byte direct from a 16 - bit address
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    const u8 *cmd, command24[] = { UPDI_PHY_SYNC , UPDI_LDS | UPDI_ADDRESS_24 | UPDI_DATA_16, address & 0xFF, (address >> 8) & 0xFF, (address >> 16) & 0xFF };
	const u8 command16[] = { UPDI_PHY_SYNC , UPDI_LDS | UPDI_ADDRESS_16 | UPDI_DATA_16, address & 0xFF, (address >> 8) & 0xFF };
    u8 resp[2];
    int cmdlen, result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> LD from %04X}", address);

	if (is24bit) {
		cmd = command24;
		cmdlen = sizeof(command24);
	}
	else {
		cmd = command16;
		cmdlen = sizeof(command16);
	}

    result = phy_transfer(PHY(link), cmd, cmdlen, resp, sizeof(resp));
    if (result != sizeof(resp)) {
        DBG_INFO(LINK_DEBUG, "phy_transfer failed %d", result);
        return -2;
    }

    *val = resp[0] | (resp[1] << 8);
    return 0;
}

/*
    LINK read 16bit data by direct mode, capsule
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @address: reg address
    @val: output buffer
	@is24bit: whether 24bit address mode
    @return 0 successful, other value if failed
*/
u16 link_ld16(void *link_ptr, u32 address, bool is24bit)
{
    u16 val = 0;
    int result;

    result = _link_ld16(link_ptr, address, &val, is24bit);
    if (result){
        DBG_INFO(LINK_DEBUG, "_link_ld16 failed %d", result);
    }

    return val;
}

/*
    LINK write 8bit data by direct mode
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @address: target address
    @value: target value
	@is24bit: whether 24bit address mode
    @return 0 successful, other value if failed
*/
int link_st(void *link_ptr, u32 address, u8 value, bool is24bit)
{
    /*
        Store a single byte value directly to a 16 - bit address
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
	const u8 *cmd, command24[] = { UPDI_PHY_SYNC, UPDI_STS | UPDI_ADDRESS_24 | UPDI_DATA_8, address & 0xFF, (address >> 8) & 0xFF, (address >> 16) & 0xFF };
    const u8 command16[] = { UPDI_PHY_SYNC, UPDI_STS | UPDI_ADDRESS_16 | UPDI_DATA_8, address & 0xFF, (address >> 8) & 0xFF};
    const u8 val[] = { value };
    u8 resp = 0xff;
    int cmdlen, result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> ST to 0x04X: %02x", address, value);

	if (is24bit) {
		cmd = command24;
		cmdlen = sizeof(command24);
	}
	else {
		cmd = command16;
		cmdlen = sizeof(command16);
	}

    result = phy_transfer(PHY(link), cmd, cmdlen, &resp, sizeof(resp));
    if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
        DBG_INFO(LINK_DEBUG, "phy_transfer failed %d ack %02x", result, resp);
        return -2;
    }

    result = phy_transfer(PHY(link), val, sizeof(val), &resp, sizeof(resp));
    if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
        DBG_INFO(LINK_DEBUG, "phy_transfer #2 failed %d ack %02x", result, resp);
        return -2;
    }

    return 0;
}

/*
    LINK write 16bit data by direct mode
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @address: target address
    @value: target value
	@is24bit: whether 24bit address mode
    @return 0 successful, other value if failed
*/
int link_st16(void *link_ptr, u32 address, u16 value, bool is24bit)
{
    /*
        Store a 16 - bit word value directly to a 16 - bit address
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    const u8 *cmd, command24[] = { UPDI_PHY_SYNC, UPDI_STS | UPDI_ADDRESS_24 | UPDI_DATA_16, address & 0xFF, (address >> 8) & 0xFF, (address >> 16) & 0xFF };
	const u8 command16[] = { UPDI_PHY_SYNC, UPDI_STS | UPDI_ADDRESS_16 | UPDI_DATA_16, address & 0xFF, (address >> 8) & 0xFF };
	const u8 val[] = { value & 0xFF, (value >> 8) & 0xFF };
	u8 resp = 0xff;
    int cmdlen, result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> ST16 to 0x04X: %04x", address, value);

	if (is24bit) {
		cmd = command24;
		cmdlen = sizeof(command24);
	}
	else {
		cmd = command16;
		cmdlen = sizeof(command16);
	}

    result = phy_transfer(PHY(link), cmd, cmdlen, &resp, sizeof(resp));
    if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
        DBG_INFO(LINK_DEBUG, "phy_transfer failed %d ack %02x", result, resp);
        return -2;
    }

    result = phy_transfer(PHY(link), val, sizeof(val), &resp, sizeof(resp));
    if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
        DBG_INFO(LINK_DEBUG, "phy_transfer #2 failed %d ack %02x", result, resp);
        return -2;
    }

    return 0;
}

/*
    LINK read 8bit data by indirect mode, 
        the address is set by link_st_ptr() first. After the operation, the ptr in increase 1
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @data: data output buffer
    @len: data length to be read
    @return 0 successful, other value if failed
*/
int link_ld_ptr_inc(void *link_ptr, u8 *data, int len)
{
    /*
        Loads a number of bytes from the pointer location with pointer post - increment
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    const u8 cmd[] = { UPDI_PHY_SYNC, UPDI_LD | UPDI_PTR_INC | UPDI_DATA_8 };
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> LD8 from ptr++");
 
    result = phy_transfer(PHY(link), cmd, sizeof(cmd), data, len);
    if (result != len) {
        DBG_INFO(LINK_DEBUG, "phy_transfer failed %d", result);
        return -2;
    }

    return 0;
}

/*
    LINK read 16bit data by indirect mode,
    the address is set by link_st_ptr() first. After the operation, the ptr in increase 2
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @data: data output buffer
    @len: data length to be read
    @return 0 successful, other value if failed
*/
int link_ld_ptr_inc16(void *link_ptr, u8 *data, int len)
{
    /*
        Load a 16-bit word value from the pointer location with pointer post-increment
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    const u8 cmd[] = { UPDI_PHY_SYNC, UPDI_LD | UPDI_PTR_INC | UPDI_DATA_16 };
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> LD16 from ptr++");

    result = phy_transfer(PHY(link), cmd, sizeof(cmd), data, len);
    if (result != len) {
        DBG_INFO(LINK_DEBUG, "phy_transfer failed %d", result);
        return -2;
    }

    return 0;
}

/*
    LINK set st/ld command address
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @address: the address to be set
	@is24bit: whether 24bit address mode
    @return 0 successful, other value if failed
*/
int link_st_ptr(void *link_ptr, u32 address, bool is24bit)
{
    /*
        Set the pointer location
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
	const u8 *cmd, command24[] = { UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_ADDRESS | UPDI_DATA_24, address & 0xFF, (address >> 8) & 0xFF, (address >> 16) & 0xFF };
	const u8 command16[] = { UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_ADDRESS | UPDI_DATA_16, address & 0xFF, (address >> 8) & 0xFF };
	u8 resp = 0xFF;
    int cmdlen, result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> ST ptr %x", address);

	if (is24bit) {
		cmd = command24;
		cmdlen = sizeof(command24);
	}
	else {
		cmd = command16;
		cmdlen = sizeof(command16);
	}

    result = phy_transfer(PHY(link), cmd, cmdlen, &resp, sizeof(resp));
    if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
        DBG_INFO(LINK_DEBUG, "phy_transfer failed %d resp = 0x%02x", result, resp);
        return -2;
    }

    return 0;
}

/*
    LINK set 8bit data by indirect mode,
        the address is set by link_st_ptr() first. After the operation, the ptr in increase 1
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @data: data input buffer
    @len: data length
    @return 0 successful, other value if failed
*/
int link_st_ptr_inc(void *link_ptr, const u8 *data, int len)
{
    /*
        Store data to the pointer location with pointer post - increment
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    const u8 cmd[] = { UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_INC | UPDI_DATA_8, data[0] };
    u8 resp;
    int i;
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> ST8 to *ptr++");

    result = phy_transfer(PHY(link), cmd, sizeof(cmd), &resp, sizeof(resp));
    if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
        DBG_INFO(LINK_DEBUG, "phy_transfer failed %d resp 0x%02x", result, resp);
        return -2;
    }

    for (i = 1; i < len; i++) {
        result = phy_transfer(PHY(link), &data[i], 1, &resp, sizeof(resp));
        if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
            DBG_INFO(LINK_DEBUG, "phy_transfer failed %d i %d resp 0x%02x", result, i, resp);
            return -2;
        }
    }

    return 0;
}

/*
    LINK set 16bit data by indirect mode,
    the address is set by link_st_ptr() first. After the operation, the ptr in increase 2
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @data: data input buffer
    @len: data length
    @return 0 successful, other value if failed
*/
int link_st_ptr_inc16(void *link_ptr, const u8 *data, int len)
{
    /*
        Store a 16 - bit word value to the pointer location with pointer post - increment
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    const u8 cmd[] = { UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_INC | UPDI_DATA_16, data[0], data[1] };
    u8 resp = 0xFF;
    int i;
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> ST16 to *ptr++, len %d", len);

    result = phy_transfer(PHY(link), cmd, sizeof(cmd), &resp, sizeof(resp));
    if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
        DBG_INFO(LINK_DEBUG, "phy_transfer failed %d resp 0x%02x", result, resp);
        return -2;
    }

    for (i = 2; i < len; i += 2) {
        result = phy_transfer(PHY(link), &data[i], 2, &resp, sizeof(resp));
        if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
            DBG_INFO(LINK_DEBUG, "phy_transfer failed(%d) i %d resp 0x%02x", result, i, resp);
            return -3;
        }
    }

    return 0;
}

/*
    LINK repeat ST/LD operation by indirect mode,
        the address is set by link_st_ptr() first. After the operation, the ptr will increase by st/ld command dedicated
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @repeats: repeats count
    @return 0 successful, other value if failed
*/
int link_repeat(void *link_ptr, u8 repeats)
{
    /*
        Store a value to the 8bit repeat counter
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    u8 cmd[] = { UPDI_PHY_SYNC, UPDI_REPEAT | UPDI_REPEAT_BYTE, repeats};
    int result;
    
    if (!VALID_LINK(link))
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> Repeat %d", repeats);

    result = phy_send(PHY(link), cmd, sizeof(cmd));
    if (result) {
        DBG_INFO(LINK_DEBUG, "phy_send failed %d", result);
        return -2;
    }

    return 0;
}

/*
    LINK repeat ST/LD operation by indirect mode, repeats is 16bit width
    the address is set by link_st_ptr() first. After the operation, the ptr will increase by st/ld command dedicated
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @repeats: repeats count(16bit width)
    @return 0 successful, other value if failed
*/
int link_repeat16(void *link_ptr, u16 repeats)
{
    /*
    Store a value to the 16bit repeat counter
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    u8 cmd[] = { UPDI_PHY_SYNC, UPDI_REPEAT | UPDI_REPEAT_WORD, repeats & 0xFF, (repeats >> 8) & 0xFF };
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> Repeat16 %d", repeats);

    result = phy_send(PHY(link), cmd, sizeof(cmd));
    if (result) {
        DBG_INFO(LINK_DEBUG, "phy_send failed %d", result);
        return -2;
    }

    return 0;
}

/*
    LINK read SIB content
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @data: output data
    @return 0 successful, other value if failed
*/
int link_read_sib(void *link_ptr, u8 *data, int len)
{
    /*
        Read the SIB
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> Read SIB len %d", len);

    return phy_sib(PHY(link), data, len);
}

/*
    LINK key access operation
    @link_ptr: APP object pointer, acquired from updi_datalink_init()
    @size_k: key size in 8-bit unit mode, (2 ^ size_k) * 8
    @key: key data
    @return 0 successful, other value if failed
*/
int link_key(void *link_ptr, u8 size_k, const char *key)
{
    /*
        Write a key
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    const u8 cmd[] = { UPDI_PHY_SYNC, UPDI_KEY | UPDI_KEY_KEY | size_k };
    u8 len = 8 << size_k;
    int i;
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    DBG_INFO(LINK_DEBUG, "<LINK> Key %x", size_k);

    result = phy_send(PHY(link), cmd, sizeof(cmd));
    if (result) {
        DBG_INFO(LINK_DEBUG, "phy_send failed %d", result);
        return -2;
    }

    for (i = 0; i < len; i++) {
        result = phy_send_byte(PHY(link), (u8)key[len - i - 1]); //Reserse the string
        if (result) {
            DBG_INFO(LINK_DEBUG, "phy_send byte %d failed %d", i, result);
            return -3;
        }
    }

    return 0;
}
