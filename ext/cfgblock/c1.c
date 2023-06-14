#include <os/platform.h>
#include <crc/crc.h>

#include "cb.h"

/*
DataBlock:  This is data block of storage in eeprom or userdata
address:    `INFO_BLOCK_ADDRESS_OFFSET` offset from storage
Layout:
    [header]
        0-3:    Tag='c'(1)    Ver='1'(1)    block size(2)
    [data content]
        4-[4 + sizeof(signal_limit_t)]:    data content[n]: `signal_limit_t`
        ....
    [tail]
        [last 4bytes]:  Fw crc(3)   rsv(1)
*/

bool cb_test_element_c1(int type)
{
    short major = TO_OP(type);
    bool result = true;

    switch (major) {
    case CB_HEAD:
    case CB_BODY:
    case CB_CRC:
        break;
    default:
        result = false;
    }

    return result;
}

static int get_header_value(const config_header_t *head, int type)
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

static int get_body_value(config_body_info_t *body, int type)
{
    switch (type) {
    case CB_CFG_BODY_SIZE:
        return body->size;
    case CB_CFG_BODY_ELEM_COUNT:
        return body->elem_count;
    default:
        return 0;
    }
}

static int get_crc_value(const config_tail_t *tail, int type)
{
    switch (type) {
    case CB_CFG_CRC:
        return tail->data.cfg;
    default:
        return tail->value;
    }
}

static uint32_t cb_get_element_c1(/*config_container_t*/void *cfg_ptr, int type)
{
    config_container_t *cfg = (config_container_t *)cfg_ptr;
    int major = TO_OP(type);

    switch (major) {
    case CB_HEAD:
        return get_header_value(cfg->head, type);
    case CB_BODY:
        return get_body_value(&cfg->body_info, type);
    case CB_CRC:
        return get_crc_value(cfg->tail, type);
    default:
        return 0;
    }
}

static config_body_c1_t *_read_body_value(const config_body_info_t *body, int type, int param)
{
    config_body_c1_t *body_c1 = (config_body_c1_t *)body->buf;
    int count = 0;
    
    switch (type) {
    case CB_CFG_BODY_ELEM_DATA:
        if (param >= 0 && param < body->elem_count) {
            return body_c1 + param;
        }
    default:
        return NULL;
    }
}

static void *cb_read_data_c1(/* config_container_t */void *cfg_ptr, int type, int param)
{
    config_container_t *cfg = (config_container_t *)cfg_ptr;
    int major = TO_OP(type);

    switch (major) {
    case CB_BODY:
        return _read_body_value(&cfg->body_info,type, param);
    default:
        return NULL;
    }
}

static void cb_show_element_c1(/*config_container_t*/void *cfg_ptr)
{
    config_container_t *cfg = (config_container_t *)cfg_ptr;
    config_body_elem_c1_t *elem;
    int i, start;

	DBG_INFO(UPDI_DEBUG, "==========================");
    
    DBG(UPDI_DEBUG, "Config Block Content(c1):", (u8 *)cfg->head, cfg->head->data.size, "%02X ");

    DBG_INFO(UPDI_DEBUG, "Signal Limit: ");

    elem = (config_body_elem_c1_t *)cfg->body_info.buf;
    for ( i = 0, start = 0; i < cfg->body_info.elem_count; i++ ) {
        DBG_INFO(UPDI_DEBUG, "K%d(n%d): [%d - %d / %d]",
            start, 
            elem[i].limit.count, 
            elem[i].limit.siglo,
            elem[i].limit.sighi,
            elem[i].limit.range);
        start += elem[i].limit.count;
    }
}

int cb_create_configure_block_c1(config_container_t *cfg, char *data, int len)
{
    config_header_t *hdr;
    config_crc_t * tail;
    unsigned short size;
    unsigned char *body;

    size = (unsigned short)(sizeof(config_header_t) + len + sizeof(config_crc_t));
    hdr = (config_header_t *)malloc(size);
    if (!hdr)
        return -2;

    memset(hdr, 0, size);

    hdr->data.version.ver[0] = CONFIG_BLOCK_C1_VER_MAJOR;
    hdr->data.version.ver[1] = CONFIG_BLOCK_C1_VER_MINOR;
    hdr->data.size = size;

    body = (unsigned char *)(hdr + 1);
    memcpy(body, data, len);

    tail = (config_crc_t * )(body + len);
    tail->data.cfg = calc_crc24((unsigned char *)hdr, sizeof(config_header_t) + len);

    cfg->head = (config_header_t *)hdr;
	cfg->body_info.buf = (void *)body;
    cfg->body_info.size = (unsigned short)len;
    cfg->body_info.elem_count = (unsigned short)(len / sizeof(config_body_elem_c1_t));
    cfg->tail = (config_tail_t *)tail;
    cfg->type = BIT_MASK(MEM_ALLOC) | BIT_MASK(BLOCK_CFG);
    cfg->intf.test = cb_test_element_c1;
    cfg->intf.get = cb_get_element_c1;
    cfg->intf.read = cb_read_data_c1;
    cfg->intf.show = cb_show_element_c1;

    return 0;
}

int cb_set_configure_block_ptr_c1(config_container_t *cfg, char *data, int len, int flag)
{
    //config_container_t *cfg = (config_container_t *)info_ptr;
    config_header_t * head = (config_header_t *)data;
    unsigned short size;

    size = (unsigned short)(sizeof(config_header_t) + sizeof(config_body_elem_c1_t) + sizeof(config_tail_t));

    if (len < size)
        return -2;

    if (TEST_BIT(flag, MEM_ALLOC)) {
        cfg->head = (config_header_t *)malloc(len);
        if (!cfg->head)
            return -3;
        memcpy(cfg->head, data, len);
    }
    else if (TEST_BIT(flag, MEM_SHARE) || TEST_BIT(flag, MEM_SHARE_RELEASE)) {
        cfg->head = head;
    } else {
        return -4;
    }

    cfg->body_info.buf = (void *)(cfg->head + 1);
    cfg->body_info.size = (unsigned short)(len - sizeof(config_header_t) - sizeof(config_tail_t));
    cfg->body_info.elem_count = (unsigned short)(cfg->body_info.size / sizeof(config_body_elem_c1_t));
    cfg->tail = (config_tail_t *)((uint8_t *)cfg->body_info.buf + cfg->body_info.size);
    cfg->type = flag | BIT_MASK(BLOCK_CFG);
    cfg->intf.test = cb_test_element_c1;
    cfg->intf.get = cb_get_element_c1;
    cfg->intf.read = cb_read_data_c1;
    cfg->intf.show = cb_show_element_c1;

    return 0;
}

void cb_destory_c1(void *cfg_ptr)
{
    config_container_t *cfg = (config_container_t *)cfg_ptr;
    config_header_t * head = cfg->head;

    if (TEST_BIT(cfg->type, MEM_ALLOC) || TEST_BIT(cfg->type, MEM_SHARE_RELEASE)) {
        free(cfg->head);
    }

    memset(cfg, 0, sizeof(cfg));
}

int cb_max_block_size_c1(void)
{
    return (unsigned short)-1;
}