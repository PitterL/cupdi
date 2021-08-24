#include <os/platform.h>
#include <crc/crc.h>

#include "ib.h"

PACK(
    typedef struct _information_block_s2 {
    information_header_t header;
    firmware_version_t fw_version;
    firmware_size_t fw_size;
    dsdr_address_t dsdr;
    information_crc_t crc;
})information_block_s2_t;

bool test_ib_element_s2(int type)
{
    short major = TO_OP(type);
    bool result = true;

    switch (major) {
    case IB_HEAD:
    case IB_FW_VER:
    case IB_FW_SIZE:
    case IB_REG_SR:
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
    case IB_HEAD_VER_N0:
        return head->data.version.ver[0];
    case IB_HEAD_VER_N1:
        return head->data.version.ver[1];
    case IB_HEAD_SIZE:
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

static uint32_t get_sr_reg_addr_value(const dsdr_address_t *sr_addr, int type)
{
    switch (type) {
    case IB_REG_SR_SIGNAL:
        return sr_addr->data.ds;
    case IB_REG_SR_REF:
        return sr_addr->data.dr;
    default:
        return sr_addr->value;
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

uint32_t get_ib_element_s2(information_header_t *head, int type)
{
    information_block_s2_t *ib = (information_block_s2_t *)head;
    int major = TO_OP(type);

    switch (major) {
    case IB_HEAD:
        return get_header_value(&ib->header, type);
    case IB_FW_VER:
        return get_fw_version_value(&ib->fw_version, type);
    case IB_FW_SIZE:
        return get_fw_size_value(&ib->fw_size, type);
    case IB_REG_SR:
        return get_sr_reg_addr_value(&ib->dsdr, type);
    case IB_CRC:
        return get_crc_value(&ib->crc, type);
    default:
        return 0;
    }
}

void show_ib_element_s2(information_header_t *head)
{
    information_block_s2_t *ib = (information_block_s2_t *)head;

	DBG_INFO(UPDI_DEBUG, "");
	DBG_INFO(UPDI_DEBUG, "==========================");

    DBG(UPDI_DEBUG, "Information Block Content:", (u8 *)ib, sizeof(*ib), "%02X ");

    DBG_INFO(UPDI_DEBUG, "fw_version: %c%c%c(%hhX) %hhX.%hhX",
        (char)get_ib_element_s2(head, IB_FW_VER_NAME_N0),
        (char)get_ib_element_s2(head, IB_FW_VER_NAME_N1),
        (char)get_ib_element_s2(head, IB_FW_VER_NAME_N2),
        (unsigned char)get_ib_element_s2(head, IB_FW_VER_NAME_N2) & 0xFF,
        (unsigned char)get_ib_element_s2(head, IB_FW_VER_NAME_BUILD_MAJOR) & 0xF,
        (unsigned char)get_ib_element_s2(head, IB_FW_VER_NAME_BUILD_MINOR) & 0xF);

    DBG_INFO(UPDI_DEBUG, "fw_size: %d bytes(0x%x)",
        get_ib_element_s2(head, IB_FW_SIZE),
        get_ib_element_s2(head, IB_FW_SIZE));

    DBG_INFO(UPDI_DEBUG, "reg addr: ds 0x%04x dr 0x%04x",
        get_ib_element_s2(head, IB_REG_SR_SIGNAL),
        get_ib_element_s2(head, IB_REG_SR_REF));

    DBG_INFO(UPDI_DEBUG, "fw_crc: 0x%06X",
        get_ib_element_s2(head, IB_CRC_FW));

	DBG_INFO(UPDI_DEBUG, "==========================");
}

int create_information_block_s2(/*information_container_t*/void *info_ptr, int fw_crc24, int fw_size, int fw_version, int dsdr_addr)
{
    information_container_t *info = (information_container_t *)info_ptr;
    information_block_s2_t *ib;
    unsigned short size;

    size = (unsigned short)sizeof(information_block_s2_t);
    ib = (information_block_s2_t *)malloc(size);
    if (!ib)
        return -2;

    memset(ib, 0, size);

    ib->header.data.version.ver[0] = INFO_BLOCK_S2_VER_MAJOR;
    ib->header.data.version.ver[1] = INFO_BLOCK_S2_VER_MINOR;
    ib->header.data.size = size;

    ib->fw_version.value = fw_version;
    ib->fw_size.value = fw_size;//len;

    ib->dsdr.value = dsdr_addr;

    ib->crc.data.fw = fw_crc24;//calc_crc24(data, len);
    ib->crc.data.info = calc_crc8((unsigned char *)ib, sizeof(*ib) - 1);

    info->head = (information_header_t *)ib;
    info->type = BIT_MASK(MEM_ALLOC);
    info->intf.test = test_ib_element_s2;
    info->intf.get = get_ib_element_s2;
    info->intf.show = show_ib_element_s2;

    return 0;
}

int set_information_block_ptr_s2(/*information_container_t*/ void *info_ptr, char *data, int len, int flag)
{
    information_container_t *info = (information_container_t *)info_ptr;
    information_header_t * head = (information_header_t *)data;
    unsigned short size;

    size = (unsigned short)sizeof(information_block_s2_t);

    if (len < size)
        return -2;

    if (TEST_BIT(flag, MEM_ALLOC)) {
        info->head = (information_header_t *)malloc(size);
        if (!info->head)
            return -3;
        memcpy(info->head, data, size);
    }
    else if (TEST_BIT(flag, MEM_SHARE) || TEST_BIT(flag, MEM_SHARE_RELEASE))
        info->head = head;
    else
        return -4;

    info->type = flag;
    info->intf.test = test_ib_element_s2;
    info->intf.get = get_ib_element_s2;
    info->intf.show = show_ib_element_s2;

    return 0;
}

void destory_information_block_s2(void *info_ptr)
{
    information_container_t *info = (information_container_t *)info_ptr;
    if (TEST_BIT(info->type, MEM_ALLOC) || TEST_BIT(info->type, MEM_SHARE_RELEASE))
        free(info->head);

    memset(info, 0, sizeof(info));
}

int infomation_block_size_s2(void)
{
    return (int)sizeof(information_block_s2_t);
}
