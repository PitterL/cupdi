#include <os/platform.h>
#include <crc/crc.h>
#include "ib.h"

/*
InfoBlock:  This is infomation block of storage in eeprom or userdata
address:    `INFO_BLOCK_ADDRESS_OFFSET` offset from storage
Layout:
    [header]
        0-3:    Major='s'(1)    Minor='2'(1)    info block size(2)
    [firmware version]
        4-7:    Fw version(3)   Fw Build number(1)
    [firmware size]
        8-11:   Fw size(4)
    [dsdr]:
        12-15:  ds pointer addr(2)   dr pointer addr(2)  
    [acqnode]:
        16-19   acq pointer addr(2)  node pointer addr(2)
    [crc]
        20-23:  Fw crc(3)   Info crc(1)
*/

PACK(
    typedef struct _information_block_s3 {
    information_header_t header;
    firmware_version_t fw_version;
    firmware_size_t fw_size;
    varible_address_t var_addr;
    information_crc_t crc;
})information_block_s3_t;

bool ib_test_element_s3(int type)
{
    short major = TO_OP(type);
    bool result = true;

    switch (major) {
    case IB_HEAD:
    case IB_FW_VER:
    case IB_FW_SIZE:
    case IB_REG:
    case IB_CRC:
        break;
    default:
        result = false;
    }

    return result;
}

static uint32_t get_header_value(const information_header_t *head, int type)
{
    switch (type) {
    case B_HEAD_VER_N0:
        return head->data.version.ver[0];
    case B_HEAD_VER_N1:
        return head->data.version.ver[1];
    case B_HEAD_SIZE:
        return head->data.size;
    default:
        return head->value;
    }
}

static uint32_t get_fw_version_value(const firmware_version_t *fw_ver, int type)
{
    switch (type) {
    case IB_FW_VER_NAME_N0:
        return fw_ver->data.ver[0];
    case IB_FW_VER_NAME_N1:
        return fw_ver->data.ver[1];
    case IB_FW_VER_NAME_N2:
        return fw_ver->data.ver[2];
    case IB_FW_VER_NAME_BUILD_MAJOR:
        return fw_ver->data.build.major;
    case IB_FW_VER_NAME_BUILD_MINOR:
        return fw_ver->data.build.minor;
    default:
        return fw_ver->value;
    }
}

static uint32_t get_fw_size_value(const firmware_size_t *fw_size, int type)
{
    return fw_size->value;
}

static uint32_t get_reg_addr_value(const varible_address_t *vaddr, int type)
{
    switch (type) {
    case IB_REG_SR_SIGNAL:
        return vaddr->dsdr.data.ds;
    case IB_REG_SR_REF:
        return vaddr->dsdr.data.dr;
    case IB_REG_AN_ACQ:
        return vaddr->acqnd.data.acq;
    case IB_REG_AN_NODE:
        return vaddr->acqnd.data.node;
    default:
        return 0;
    }
}

static uint32_t get_crc_value(const information_crc_t *crc, int type)
{
    switch (type) {
    case IB_CRC_FW:
        return crc->data.fw;
    case IB_CRC_INFO:
        return crc->data.info;
    default:
        return crc->value;
    }
}

uint32_t ib_get_element_s3(information_header_t *head, int type)
{
    information_block_s3_t *ib = (information_block_s3_t *)head;
    int major = TO_OP(type);

    if (!head || head->data.version.value != INFO_BLOCK_S3_VERSION)
        return 0;

    switch (major) {
    case IB_HEAD:
        return get_header_value(&ib->header, type);
    case IB_FW_VER:
        return get_fw_version_value(&ib->fw_version, type);
    case IB_FW_SIZE:
        return get_fw_size_value(&ib->fw_size, type);
    case IB_REG:
        return get_reg_addr_value(&ib->var_addr, type);
    case IB_CRC:
        return get_crc_value(&ib->crc, type);
    default:
        return 0;
    }
}

void ib_show_element_s3(information_header_t *head)
{
    information_block_s3_t *ib = (information_block_s3_t *)head;
	char n, n0, n1, n2;

    if (!head || head->data.version.value != INFO_BLOCK_S3_VERSION)
        return;

	DBG_INFO(UPDI_DEBUG, "");
	DBG_INFO(UPDI_DEBUG, "==========================");

    DBG(UPDI_DEBUG, "Information Block Content(s3):", (u8 *)ib, sizeof(*ib), "%02X ");

	n = (char)ib_get_element_s3(head, IB_FW_VER_NAME_N0);
	n0 = n >= ' ' && n <= '~' ? n : ' ';
	n = (char)ib_get_element_s3(head, IB_FW_VER_NAME_N1);
	n1 = n >= ' ' && n <= '~' ? n : ' ';
	n2 = (char)ib_get_element_s3(head, IB_FW_VER_NAME_N2);
	
    DBG_INFO(UPDI_DEBUG, "fw_version: <%c%c%02hhX> %hhX.%hhX",
        n0,
        n1,
        n2,
        (unsigned char)ib_get_element_s3(head, IB_FW_VER_NAME_BUILD_MAJOR) & 0xF,
        (unsigned char)ib_get_element_s3(head, IB_FW_VER_NAME_BUILD_MINOR) & 0xF);

    DBG_INFO(UPDI_DEBUG, "fw_size: %d bytes(0x%x)",
        ib_get_element_s3(head, IB_FW_SIZE),
        ib_get_element_s3(head, IB_FW_SIZE));

    DBG_INFO(UPDI_DEBUG, "reg addr: ds 0x%04x dr 0x%04x",
        ib_get_element_s3(head, IB_REG_SR_SIGNAL),
        ib_get_element_s3(head, IB_REG_SR_REF));

    DBG_INFO(UPDI_DEBUG, "reg addr: acq 0x%04x node 0x%04x",
        ib_get_element_s3(head, IB_REG_AN_ACQ),
        ib_get_element_s3(head, IB_REG_AN_NODE));

    DBG_INFO(UPDI_DEBUG, "fw_crc: 0x%06X",
        ib_get_element_s3(head, IB_CRC_FW));

	DBG_INFO(UPDI_DEBUG, "info_crc: 0x%06X",
		ib_get_element_s3(head, IB_CRC_INFO));

	DBG_INFO(UPDI_DEBUG, "==========================");
}

int ib_create_information_block_s3(information_container_t *info, information_content_params_t *param, int len)
{
    information_block_s3_t *ib;
    unsigned short size;

    if (!info || !param)
        return -2;

    size = (unsigned short)sizeof(information_block_s3_t);
    ib = (information_block_s3_t *)malloc(size);
    if (!ib)
        return -3;

    memset(ib, 0, size);

    ib->header.data.version.ver[0] = INFO_BLOCK_S3_VER_MAJOR;
    ib->header.data.version.ver[1] = INFO_BLOCK_S3_VER_MINOR;
    ib->header.data.size = size;

    ib->fw_version.value = param->fw_crc24;
    ib->fw_size.value = param->fw_size;//len;
    memcpy(&ib->var_addr, &param->var_addr, sizeof(ib->var_addr));

    ib->crc.data.fw = param->fw_crc24;//calc_crc24(data, len);
    ib->crc.data.info = calc_crc8((unsigned char *)ib, sizeof(*ib) - 1);

    info->head = (information_header_t *)ib;
    info->type = BIT_MASK(MEM_ALLOC);
    info->intf.test = ib_test_element_s3;
    info->intf.get = ib_get_element_s3;
    info->intf.show = ib_show_element_s3;

    return 0;
}

int ib_set_infoblock_data_ptr_s3(information_container_t *info, char *data, int len, int flag)
{
    // information_container_t *info = (information_container_t *)info_ptr;
    information_header_t * head = (information_header_t *)data;
    unsigned short size;

    if (!info || !data)
        return -2;

    size = (unsigned short)sizeof(information_block_s3_t);
    if (len != size)
        return -3;

    if (TEST_BIT(flag, MEM_ALLOC)) {
        info->head = (information_header_t *)malloc(size);
        if (!info->head)
            return -4;
        memcpy(info->head, data, size);
    }
    else if (TEST_BIT(flag, MEM_SHARE) || TEST_BIT(flag, MEM_SHARE_RELEASE))
        info->head = head;
    else
        return -5;

    info->type = flag;
    info->intf.test = ib_test_element_s3;
    info->intf.get = ib_get_element_s3;
    info->intf.show = ib_show_element_s3;

    return 0;
}

void ib_destory_s3(void *info_ptr)
{
    information_container_t *info = (information_container_t *)info_ptr;
    information_header_t * head;

    if (!info || info->head)
        return;
    
    head = (information_header_t *)info->head;
    if (head->data.version.value != INFO_BLOCK_S3_VERSION)
        return;

    if (TEST_BIT(info->type, MEM_ALLOC) || TEST_BIT(info->type, MEM_SHARE_RELEASE))
        free(info->head);

    memset(info, 0, sizeof(info));
}

int ib_max_block_size_s3(void)
{
    return (int)sizeof(information_block_s3_t);
}
