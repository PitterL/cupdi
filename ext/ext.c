#include "os/platform.h"
#include "ext.h"


int ext_create_data_block(void *contnr, void *data, int len, int flag)
{
    int result = -2;

    if (TEST_BIT(flag, BLOCK_INFO)) {
        result = ib_create_information_block(contnr, data, len);
    } else if (TEST_BIT(flag, BLOCK_CFG)) {
        result = cb_create_configure_block(contnr, data, len);
    } else {
        /* Not support */
    }

    return result;
}

int ext_info_set_data_ptr(void *contnr, char *data, int len, int flag)
{
    int result = -2;

    if (TEST_BIT(flag, BLOCK_INFO)) {
        result = ib_set_information_block_data_ptr(contnr, data, len, flag);
    } else if (TEST_BIT(flag, BLOCK_CFG)) {
        result = cb_set_configure_block_data_ptr(contnr, data, len, flag);
    } else {
        /* Not support */
    }

    return result;
}

void ext_destory(void *contnr, int flag)
{
    if (TEST_BIT(flag, BLOCK_INFO)) {
        ib_destory(contnr);
    } else if (TEST_BIT(flag, BLOCK_CFG)) {
        cb_destory(contnr);
    } else {
        /* Not support */
    }
}

int ext_info_max_size(int flag)
{
    int result = 0;
    
    if (TEST_BIT(flag, BLOCK_INFO)) {
        result = ib_max_block_size();
    } else if (TEST_BIT(flag, BLOCK_CFG)) {
        result = cb_max_block_size();
    } else {
        /* Not support */
    }

    return result;
}

bool ext_test(void *contnr, int type, int flag)
{
    bool result = false;

    if (TEST_BIT(flag, BLOCK_INFO)) {
        result = ib_test(contnr, type);
    } else if (TEST_BIT(flag, BLOCK_CFG)) {
        result = cb_test(contnr, type);
    } else {
        /* Not support */
    }

    return result;
}

int ext_get(void *contnr, int type, int flag)
{
    int result = 0;

    if (TEST_BIT(flag, BLOCK_INFO)) {
        result = ib_get(contnr, type);
    } else if (TEST_BIT(flag, BLOCK_CFG)) {
        result = cb_get(contnr, type);
    } else {
        /* Not support */
    }

    return result;    
}

void *ext_read(void *contnr, int type, int param, int flag)
{
    void *result = NULL;

    if (TEST_BIT(flag, BLOCK_INFO)) {
        //result = ib_read(contnr, type, param);
    } else if (TEST_BIT(flag, BLOCK_CFG)) {
        result = cb_read(contnr, type, param);
    } else {
        /* Not support */
    }

    return result;    
}

void ext_show(void *contnr, int flag)
{
    if (TEST_BIT(flag, BLOCK_INFO)) {
        ib_show(contnr);
    }
    
    if (TEST_BIT(flag, BLOCK_CFG)) {
        cb_show(contnr);
    }
}