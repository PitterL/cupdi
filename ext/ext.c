#include "os/platform.h"
#include "../device/device.h"
#include "ext.h"

int ext_create_data_block(void *contnr, void *data, int len, B_BLOCK_TYPE btype)
{
    int result = -2;

    if (btype == BLOCK_INFO) {
        result = ib_create_information_block(contnr, data, len);
    } else if (btype == BLOCK_CFG) {
        result = cb_create_configure_block(contnr, data, len);
    } else {
        /* Not support */
    }

    return result;
}

int ext_info_set_data_ptr(void *contnr, char *data, int len, unsigned short flag)
{
    ext_header_t *head = (ext_header_t *)data;
    int result = -2;

    if (!data || len < sizeof(ext_header_t)) {
        return result;
    }

    if (ib_is_head(head)) {
        result = ib_set_information_block_data_ptr(contnr, data, len, flag);
    } else if (cb_is_head(head)) {
        result = cb_set_configure_block_data_ptr(contnr, data, len, flag);
    } else {
        /* Not support */
    }

    return result;
}

void ext_destory(void *contnr)
{
    container_header_t *head = contnr;

    if (ib_is_container(head)) {
        ib_destory(contnr);
    } else if (cb_is_container(head)) {
        cb_destory(contnr);
    } else {
        /* Not support */
    }
}

int ext_info_max_size(B_BLOCK_TYPE btype)
{
    int result = 0;

    if (btype == BLOCK_INFO) {
        result = ib_max_block_size();
    } else if (btype == BLOCK_CFG) {
        result = cb_max_block_size();
    } else {
        /* Not support */
    }

    return result;
}

bool ext_is(void *contnr, B_BLOCK_TYPE btype)
{
    container_header_t *head = contnr;
    bool result = false;

    if (btype == BLOCK_INFO) {
        result = ib_is_container(head);
    } else if (btype == BLOCK_CFG) {
        result = cb_is_container(head);
    } else {
        /* Not support */
    }

    return result;
}

bool ext_test(void *contnr, int type)
{
    container_header_t *head = contnr;
    bool result = false;

    if (ib_is_container(head)) {
        result = ib_test(contnr, type);
    } else if (cb_is_container(head)) {
        result = cb_test(contnr, type);
    } else {
        /* Not support */
    } 

    return result;
}

int ext_get(void *contnr, int type)
{
    container_header_t *head = contnr;
    int result = 0;

    if (ib_is_container(head)) {
        result = ib_get(contnr, type);
    } else if (cb_is_container(head)) {
        result = cb_get(contnr, type);
    } else {
        /* Not support */
    } 

    return result;    
}

void *ext_read(void *contnr, int type, int param)
{
    container_header_t *head = contnr;
    void *result = NULL;

    if (ib_is_container(head)) {
        /* Not support */
    } else if (cb_is_container(head)) {
        result = cb_read(contnr, type, param);
    } else {
        /* Not support */
    }

    return result;    
}

void ext_show(void *contnr)
{
    container_header_t *head = contnr;

    if (ib_is_container(head)) {
        ib_show(contnr);
    } else if (cb_is_container(head)) {
        cb_show(contnr);
    } else {
        /* Not support */
    }
}