#include <os/platform.h>
#include <crc/crc.h>
#include "ib.h"

int ib_create_information_block(information_container_t *info, information_content_params_t *param, int len)
{
    if (!info || !param)
        return -2;

    if (len != sizeof(information_content_params_t)) {
        return -3;
    }

    return ib_create_information_block_s3(info, param, len);
}

int ib_set_information_block_data_ptr(information_container_t *info, char *data, int len, int flag)
{
    information_header_t * head;
    unsigned crc8;

    if (!info || !data)
        return -2;

    head = (information_header_t *)data;
    if (head->data.size < IB_HEAD_AND_TAIL_SIZE || head->data.size > len)
        return -3;

    crc8 = calc_crc8((unsigned char *)data, head->data.size);
    if (crc8 != 0)
        return -4;

    switch (head->data.version.value) {
    case INFO_BLOCK_S1_VERSION:
        return ib_set_infoblock_data_ptr_s1(info, data, head->data.size, flag);
    case INFO_BLOCK_S2_VERSION:
        return ib_set_infoblock_data_ptr_s2(info, data, head->data.size, flag);
    case INFO_BLOCK_S3_VERSION:
        return ib_set_infoblock_data_ptr_s3(info, data, head->data.size, flag);
    default:
        return -5;
    }
}

void ib_destory(information_container_t *info)
{
    information_header_t * head;
    
    if (!info || !info->head)
        return;

    head = head = info->head;
    switch (head->data.version.value) {
    case INFO_BLOCK_S1_VERSION:
        ib_destory_s1(info);
        break;
    case INFO_BLOCK_S2_VERSION:
        ib_destory_s2(info);
        break;
    case INFO_BLOCK_S3_VERSION:
        ib_destory_s3(info);
        break;
    default:
        ;
    }
}

int ib_max_block_size(void)
{
    int size;

    size = ib_max_block_size_s1();
    size = max(size, ib_max_block_size_s2());
    size = max(size, ib_max_block_size_s3());

    return size;
}

bool ib_test(information_container_t *info, IB_DTYPE type)
{
    if (!info || !info->intf.test)
        return false;

    return info->intf.test(type);
}

int ib_get(information_container_t *info, IB_DTYPE type)
{
    if (!info || !info->intf.get)
        return 0;

    return info->intf.get(info->head, type);
}

void ib_show(information_container_t *info)
{
    if (!info || !info->intf.show)
        return;

    info->intf.show(info->head);
}