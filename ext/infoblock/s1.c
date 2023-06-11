#include <os/platform.h>
#include <crc/crc.h>

#include "ib.h"

/*
InfoBlock:  This is infomation block of storage in eeprom or userdata
address:    `INFO_BLOCK_ADDRESS_OFFSET` offset from storage
Layout:
    [header]
        0-3:    Major='s'(1)    Minor='1'(1)    info block size(2)
    [firmware version]
        4-7:    Fw version(3)   Fw Build number(1)
    [firmware size]
        8-11:   Fw size(4)
    [crc]
        12-15:  Fw crc(3)   Info crc(1)
*/

#pragma pack(1)
    typedef struct _information_block_s1 {
    information_header_t header;
    firmware_version_t fw_version;
    firmware_size_t fw_size;
    information_crc_t crc;
}information_block_s1_t;
#pragma pack()

bool ib_test_element_s1(int type)
{
    short major = TO_OP(type);
    bool result = true;

    switch (major) {
    case IB_HEAD:
    case IB_FW_VER:
    case IB_FW_SIZE:
    case IB_CRC:
        break;
    default:
        result = false;
    }

    return result;
}

static int get_header_value(const information_header_t *head, int type)
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

static int get_fw_version_value(const firmware_version_t *fw_ver, int type)
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

static int get_crc_value(const information_crc_t *crc, int type)
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

uint32_t ib_get_element_s1(information_header_t *head, int type)
{
    information_block_s1_t *ib= (information_block_s1_t *)head;
    int major = TO_OP(type);

    if (!head || head->data.version.value != INFO_BLOCK_S1_VERSION)
        return 0;

    switch (major) {
    case IB_HEAD:
        return get_header_value(&ib->header, type);
    case IB_FW_VER:
        return get_fw_version_value(&ib->fw_version, type);
    case IB_FW_SIZE:
        return get_fw_size_value(&ib->fw_size, type);
    case IB_CRC:
        return get_crc_value(&ib->crc, type);
    default:
        return 0;
    }
}

void ib_show_element_s1(information_header_t *head)
{
    information_block_s1_t *ib = (information_block_s1_t *)head;
    
    if (!head || head->data.version.value != INFO_BLOCK_S1_VERSION)
        return;

    DBG(UPDI_DEBUG, "Information Block Content(s1):", (u8 *)ib, sizeof(*ib), "%02X ");

    DBG_INFO(UPDI_DEBUG, "fw_version: %c%c%c %hhx.%hhx",
        (char)ib_get_element_s1(head, IB_FW_VER_NAME_N0),
        (char)ib_get_element_s1(head, IB_FW_VER_NAME_N1),
        (char)ib_get_element_s1(head, IB_FW_VER_NAME_N2),
        (unsigned char)ib_get_element_s1(head, IB_FW_VER_NAME_BUILD_MAJOR) & 0xF,
        (unsigned char)ib_get_element_s1(head, IB_FW_VER_NAME_BUILD_MINOR) & 0xF);

    DBG_INFO(UPDI_DEBUG, "fw_size: %d bytes(0x%x)",
        ib_get_element_s1(head, IB_FW_SIZE),
        ib_get_element_s1(head, IB_FW_SIZE));

    DBG_INFO(UPDI_DEBUG, "fw_crc: 0x%06x",
        ib_get_element_s1(head, IB_CRC_FW));
}

int ib_create_information_block_s1(information_container_t *info, int fw_crc24, int fw_size, int fw_version)
{
    //information_container_t *info = (information_container_t *)info_ptr;
    information_block_s1_t *ib;
    unsigned short size;

    if (!info)
        return -2;

    size = (unsigned short)sizeof(information_block_s1_t);
    ib = (information_block_s1_t *)malloc(size);
    if (!ib)
        return -3;

    memset(ib, 0, size);

    ib->header.data.version.ver[0] = INFO_BLOCK_S1_VER_MAJOR;
    ib->header.data.version.ver[1] = INFO_BLOCK_S1_VER_MINOR;
    ib->header.data.size = size;

    ib->fw_version.value = fw_version;
    ib->fw_size.value = fw_size;//len;

    ib->crc.data.fw = fw_crc24;//calc_crc24(data, len);
    ib->crc.data.info = calc_crc8((unsigned char *)ib, sizeof(*ib) - 1);

    info->head = (information_header_t *)ib;
    info->type = BIT_MASK(MEM_ALLOC);
    info->intf.test = ib_test_element_s1;
    info->intf.get = ib_get_element_s1;
    info->intf.show = ib_show_element_s1;

    return 0;
}

int ib_set_infoblock_data_ptr_s1(information_container_t *info, char *data, int len, int flag)
{
    //information_container_t *info = (information_container_t *)info_ptr;
    information_header_t * head = (information_header_t *)data;
    unsigned short size;

    if (!info || !data)
        return -2;

    size = (unsigned short)sizeof(information_block_s1_t);
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
    info->intf.test = ib_test_element_s1;
    info->intf.get = ib_get_element_s1;
    info->intf.show = ib_show_element_s1;

    return 0;
}

void ib_destory_s1(void *info_ptr)
{
    information_container_t *info = (information_container_t *)info_ptr;
    information_header_t * head;

    if (!info || !info->head)
        return;
    
    head = (information_header_t *)info->head;
    if (head->data.version.value != INFO_BLOCK_S1_VERSION)
        return;
   
    if (TEST_BIT(info->type, MEM_ALLOC) || TEST_BIT(info->type, MEM_SHARE_RELEASE))
        free(info->head);

    memset(info, 0, sizeof(info));
}

int ib_max_block_size_s1(void)
{
    return (int)sizeof(information_block_s1_t);
}