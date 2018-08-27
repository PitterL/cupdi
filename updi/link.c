#include "os/platform.h"
#include "physical.h"
#include "link.h"
#include "constants.h"

typedef struct _upd_datalink {
#define UPD_DATALINK_MAGIC_WORD 'ulin'
    unsigned int mgwd;  //magic word
    void *phy;
}upd_datalink_t;

#define VALID_LINK(_link) ((_link) && ((_link)->mgwd == UPD_DATALINK_MAGIC_WORD))
#define PHY(_link) ((_link)->phy)

void *updi_datalink_init(const char *port, int baud)
{
    upd_datalink_t *link = NULL;
    void *phy;
    int result;

    _loginfo_i("<LINK> init link");
    
    phy = updi_physical_init(port, baud);
    if (phy) {
        link = (upd_datalink_t *)malloc(sizeof(*link));
        link->mgwd = UPD_DATALINK_MAGIC_WORD;
        link->phy = (void *)phy;

        // Set the inter-byte delay bit and disable collision detection
        result = link_stcs(link, UPDI_CS_CTRLB, 1 << UPDI_CTRLB_CCDETDIS_BIT);
        if (result) {
            _loginfo_i("link_stcs UPDI_CS_CTRLB failed %d", result);
            updi_datalink_deinit(link);
            return NULL;
        }

        result = link_stcs(link, UPDI_CS_CTRLA, 1 << UPDI_CTRLA_IBDLY_BIT);
        if (result) {
            _loginfo_i("link_stcs UPDI_CS_CTRLA failed %d", result);
            return NULL;
        }

        result = link_check(link);
        if (result) {
            _loginfo_i("link_check failed %d", result);
            updi_datalink_deinit(link);
            return NULL;
        }
    }

    return link;
}

void updi_datalink_deinit(void *link_ptr)
{
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    if (VALID_LINK(link)) {
        _loginfo_i("<LINK> deinit link");

        updi_physical_deinit(PHY(link));
        free(link);
    }
}

int link_check(void *link_ptr)
{
    /*
        Check UPDI by loading CS STATUSA
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    u8 resp;

    if (!VALID_LINK(link))
        return ERROR_PTR;
    
    _loginfo_i("<LINK> link check");

    resp = link_ldcs(link_ptr, UPDI_CS_STATUSA);
    if (resp != 0) {
        _loginfo_i("UPDI init OK (%02x)", resp);
        return 0;
    }
    
    _loginfo_i("UPDI not OK - reinitialisation required");
    
    return -1;
}

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

    _loginfo_i("<LINK> LDCS from 0x02X", address);
    result = phy_transfer(PHY(link), cmd, sizeof(cmd), &resp, sizeof(resp));
    if (result != sizeof(resp)) {
        _loginfo_i("phy_transfer failed %d", result);
        return -2;
    }

    
    *data = resp;

    return 0;
}

u8 link_ldcs(void *link_ptr, u8 address)
{
    u8 resp = 0;
    
    _link_ldcs(link_ptr, address, &resp);

    return resp;
}

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

    _loginfo_i("<LINK> STCS to 0x02x", address);

    result = phy_send(PHY(link), cmd, sizeof(cmd));
    if (result) {
        _loginfo_i("phy_send failed %d", result);
        return -2;
    }

    return 0;
}

int _link_ld(void *link_ptr, u16 address, u8 *data)
{
    /*
        Load a single byte direct from a 16 - bit address
        return 0 if error
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    const u8 cmd[] = { UPDI_PHY_SYNC, UPDI_LDS | UPDI_ADDRESS_16 | UPDI_DATA_8, address & 0xFF, (address >> 8) & 0xFF};
    u8 resp;
    int result;

    if (!VALID_LINK(link) || !data)
        return ERROR_PTR;

    _loginfo_i("<LINK> LD from %04X}", address);
  
    result = phy_transfer(PHY(link), cmd, sizeof(cmd), &resp, sizeof(resp));
    if (result != sizeof(resp)) {
        _loginfo_i("phy_transfer failed %d", result);
        return -2;
    }

    *data = resp;

    return 0;
}

u8 link_ld(void *link_ptr, u16 address)
{
    u8 resp = 0;

    _link_ld(link_ptr, address, &resp);

    return resp;
}

u16 link_ld16(void *link_ptr, u16 address)
{
    /*
    Load a 2 byte direct from a 16 - bit address
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    const u8 cmd[] = { UPDI_PHY_SYNC , UPDI_LDS | UPDI_ADDRESS_16 | UPDI_DATA_16, address & 0xFF, (address >> 8) & 0xFF};
    u8 resp[2] = {0xff, 0xff};
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    _loginfo_i("<LINK> LD from %04X}", address);

    result = phy_transfer(PHY(link), cmd, sizeof(cmd), resp, sizeof(resp));
    if (result != sizeof(resp)) {
        _loginfo_i("phy_transfer failed %d", result);
    }

    return resp[0] | (resp[1] << 8);
}

int link_st(void *link_ptr, u16 address, u8 value)
{
    /*
        Store a single byte value directly to a 16 - bit address
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    const u8 cmd[] = { UPDI_PHY_SYNC, UPDI_STS | UPDI_ADDRESS_16 | UPDI_DATA_8, address & 0xFF, (address >> 8) & 0xFF};
    const u8 val[] = { value };
    u8 resp = 0xff;
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    _loginfo_i("<LINK> ST to 0x04X: %02x", address, value);

    result = phy_transfer(PHY(link), cmd, sizeof(cmd), &resp, sizeof(resp));
    if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
        _loginfo_i("phy_transfer failed %d ack %02x", result, resp);
        return -2;
    }

    result = phy_transfer(PHY(link), val, sizeof(val), &resp, sizeof(resp));
    if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
        _loginfo_i("phy_transfer #2 failed %d ack %02x", result, resp);
        return -2;
    }

    return 0;
}

int link_st16(void *link_ptr, u16 address, u16 value)
{
    /*
        Store a 16 - bit word value directly to a 16 - bit address
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    const u8 cmd[] = { UPDI_PHY_SYNC, UPDI_STS | UPDI_ADDRESS_16 | UPDI_DATA_16, address & 0xFF, (address >> 8) & 0xFF };
    const u8 val[] = { value & 0xFF, (value >> 8) & 0xFF };
    u8 resp = 0xff;
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    _loginfo_i("<LINK> ST16 to 0x04X: %04x", address, value);

    result = phy_transfer(PHY(link), cmd, sizeof(cmd), &resp, sizeof(resp));
    if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
        _loginfo_i("phy_transfer failed %d ack %02x", result, resp);
        return -2;
    }

    result = phy_transfer(PHY(link), val, sizeof(val), &resp, sizeof(resp));
    if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
        _loginfo_i("phy_transfer #2 failed %d ack %02x", result, resp);
        return -2;
    }

    return 0;
}

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

    _loginfo_i("<LINK> LD8 from ptr++");
 
    result = phy_transfer(PHY(link), cmd, sizeof(cmd), data, len);
    if (result != len) {
        _loginfo_i("phy_transfer failed %d", result);
        return -2;
    }

    return 0;
}

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

    _loginfo_i("<LINK> LD16 from ptr++");

    result = phy_transfer(PHY(link), cmd, sizeof(cmd), data, len);
    if (result != len) {
        _loginfo_i("phy_transfer failed %d", result);
        return -2;
    }

    return 0;
}

int link_st_ptr(void *link_ptr, u16 address)
{
    /*
        Set the pointer location
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    const u8 cmd[] = { UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_ADDRESS | UPDI_DATA_16, address & 0xFF, (address >> 8) & 0xFF };
    u8 resp = 0xFF;
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    _loginfo_i("<LINK> ST to ptr");

    result = phy_transfer(PHY(link), cmd, sizeof(cmd), &resp, sizeof(resp));
    if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
        _loginfo_i("phy_transfer failed %d resp = 0x%02x", result, resp);
        return -2;
    }

    return 0;
}

int link_st_ptr_inc(void *link_ptr, u8 *data, int len)
{
    /*
        Store data to the pointer location with pointer post - increment
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    const u8 cmd[] = { UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_INC | UPDI_DATA_8, data[0] };
    u8 resp = 0xFF;
    int i;
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    _loginfo_i("<LINK> ST8 to *ptr++");

    result = phy_transfer(PHY(link), cmd, sizeof(cmd), &resp, sizeof(resp));
    if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
        _loginfo_i("phy_transfer failed %d resp 0x%02x", result, resp);
        return -2;
    }

    for (i = 1; i < len; i++) {
        result = phy_transfer(PHY(link), &data[i], 1, &resp, sizeof(resp));
        if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
            _loginfo_i("phy_transfer failed %d i %d resp 0x%02x", result, i, resp);
            return -2;
        }
    }

    return 0;
}

int link_st_ptr_inc16(void *link_ptr, u8 *data, int len)
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

    _loginfo_i("<LINK> ST16 to *ptr++");

    result = phy_transfer(PHY(link), cmd, sizeof(cmd), &resp, sizeof(resp));
    if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
        _loginfo_i("phy_transfer failed %d resp 0x%02x", result, resp);
        return -2;
    }

    for (i = 2; i < len; i += 2) {
        result = phy_transfer(PHY(link), &data[i], 2, &resp, sizeof(resp));
        if (result != sizeof(resp) || resp != UPDI_PHY_ACK) {
            _loginfo_i("phy_transfer failed %d i %d resp 0x%02x", result, i, resp);
            return -2;
        }
    }

    return 0;
}

int link_repeat(void *link_ptr, u8 repeats)
{
    /*
        Store a value to the repeat counter
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    u8 cmd[] = { UPDI_PHY_SYNC, UPDI_REPEAT | UPDI_REPEAT_WORD, (repeats - 1) & 0xFF, ((repeats - 1) >> 8) & 0xFF };
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    _loginfo_i("<LINK> Repeat %d", repeats);

    result = phy_send(PHY(link), cmd, sizeof(cmd));
    if (result) {
        _loginfo_i("phy_send failed %d", result);
        return -2;
    }

    return 0;
}

int link_read_sib(void *link_ptr, u8 *data, int len)
{
    /*
        Read the SIB
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    _loginfo_i("<LINK> Read SIB len %d", len);

    return phy_sib(PHY(link), data, len);
}

int link_key(void *link_ptr, u8 size_k, const char *key)
{
    /*
        Read the SIB
    */
    upd_datalink_t *link = (upd_datalink_t *)link_ptr;
    const u8 cmd[] = { UPDI_PHY_SYNC, UPDI_KEY | UPDI_KEY_KEY | size_k };
    u8 len = 8 << size_k;
    int i;
    int result;

    if (!VALID_LINK(link))
        return ERROR_PTR;

    _loginfo_i("<LINK> Key %x", size_k);

    result = phy_send(PHY(link), key, len);
    if (result) {
        _loginfo_i("phy_send failed %d", result);
        return -2;
    }

    for (i = 0; i < len; i++) {
        result = phy_send_byte(PHY(link), (u8)key[len - i - 1]); //Reserse the string
        if (result) {
            _loginfo_i("phy_send byte %d failed %d", i, result);
            return -3;
        }
    }

    return 0;
}
