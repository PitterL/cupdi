#include <os/platform.h>
#include <crc/crc.h>
#include "cb.h"

int cb_create_configure_block(config_container_t *cfg, char *data, int len)
{
    return cb_create_configure_block_c1(cfg, data, len);
}

int cb_set_configure_block_data_ptr(config_container_t *cfg, char *data, int len, int flag)
{
    config_header_t * head;
    config_tail_t *tail;
    unsigned int crc24;

    if (!cfg || !data)
        return -2;

    head = (config_header_t *)data;
    if (head->data.size < CB_HEAD_AND_TAIL_SIZE || head->data.size > len)
        return -3;

    tail = (config_tail_t *)((char *)head + head->data.size - sizeof(config_tail_t));

    crc24 = calc_crc24((unsigned char *)data, head->data.size - sizeof(config_tail_t));
    if (crc24 != tail->data.cfg)
        return -4;

    switch (head->data.version.value) {
    case CONFIG_BLOCK_C1_VERSION:
        return cb_set_configure_block_ptr_c1(cfg, data, head->data.size, flag);
    default:
        return -5;
    }
}

void cb_destory(config_container_t *cfg)
{
    config_header_t * head;
    
    if (!cfg || !cfg->head)
        return;

    head = cfg->head;
    switch (head->data.version.value) {
    case CONFIG_BLOCK_C1_VERSION:
        cb_destory_c1(cfg);
        break;
    default:
        ;
    }
}

int cb_max_block_size(void)
{
    int size;

    size = cb_max_block_size_c1();

    return size;
}

bool cb_test(config_container_t *cfg, CB_DTYPE type)
{
    if (!cfg || !cfg->intf.test)
        return false;

    return cfg->intf.test(type);
}

int cb_get(config_container_t *cfg, CB_DTYPE type)
{
    if (!cfg || !cfg->intf.get)
        return 0;

    return cfg->intf.get(cfg, type);
}

void *cb_read(config_container_t *cfg, CB_DTYPE type, int param)
{
    if (!cfg || !cfg->intf.read)
        return NULL;

    return cfg->intf.read(cfg, type, param);
}

void cb_show(config_container_t *cfg)
{
    if (!cfg || !cfg->intf.show)
        return;

    cfg->intf.show(cfg);
}