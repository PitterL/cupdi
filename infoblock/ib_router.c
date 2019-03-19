#include <os/platform.h>
#include <crc/crc.h>
#include "ib.h"

int infoblock_set_data_ptr(information_container_t *info, char *data, int len, int flag)
{
    information_header_t * head;
    unsigned crc8;

    head = (information_header_t *)data;
    if (head->data.size != len)
        return -3;

    crc8 = calc_crc8((unsigned char *)data, len);
    if (crc8 != 0)
        return -2;

    switch (head->data.version.value) {
    case INFO_BLOCK_S1_VERSION:
        return set_information_block_ptr_s1(info, data, len, flag);
    case INFO_BLOCK_S2_VERSION:
        return set_information_block_ptr_s2(info, data, len, flag);
    default:
        return -3;
    }
}

void ib_destory(information_container_t *info)
{
    information_header_t * head = info->head;
    
    if (!head)
        return;

    switch (head->data.version.value) {
    case INFO_BLOCK_S1_VERSION:
        destory_information_block_s1(info);
        break;
    case INFO_BLOCK_S2_VERSION:
        destory_information_block_s2(info);
        break;
    default:
        ;
    }
}

int ib_max_block_size(void)
{
    int size;

    size = infomation_block_size_s1();
    size = max(size, infomation_block_size_s2());

    return size;
}

bool ib_test(information_container_t *info, IB_DTYPE type)
{
    if (!info->intf.test)
        return false;

    return info->intf.test(type);
}

int ib_get(information_container_t *info, IB_DTYPE type)
{
    if (!info->intf.get)
        return ERROR_PTR;

    return info->intf.get(info->head, type);
}

void ib_show(information_container_t *info)
{
    if (!info->intf.show)
        return;

    info->intf.show(info->head);
}