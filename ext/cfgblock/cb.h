#ifndef __CONFIG_BLOCK_H
#define __CONFIG_BLOCK_H

#include "../include/blk.h"

typedef enum { CB_HEAD = B_HEAD, CB_BODY, CB_CRC, CB_DATA_TYPES } CB_DTYPE;

typedef ext_header_t cfg_header_t;

typedef union {
    ext_header_t data;
    unsigned int value;
} config_header_t;

typedef struct {
    void *buf;
    unsigned short size;
    unsigned short elem_count;
} config_body_info_t;
enum { CB_BODY_ST = SUB_OP_START(CB_BODY), CB_CFG_BODY_ELEM_COUNT = CB_BODY_ST, CB_CFG_BODY_SIZE, CB_CFG_BODY_ELEM_DATA };

#pragma pack(1)
typedef struct {
    unsigned int cfg : 24;
    unsigned int rsv : 8;
} cfg_crc_t;
#pragma pack()

typedef union {
    cfg_crc_t data;
    unsigned int value;
} config_crc_t;
enum { CB_CRC_ST = SUB_OP_START(CB_CRC), CB_CFG_CRC = CB_CRC_ST };

typedef config_crc_t config_tail_t;

typedef bool (*cb_cb_test_t)(int type);
typedef int (*cb_cb_get_t)(/*config_container_t*/void *cfg_ptr, int type);
typedef void *(*cb_cb_read_t)(/*config_container_t*/void *cfg_ptr, int type, int param);
typedef void (*cb_cb_show_t)(/*config_container_t*/void *cfg_ptr);
typedef struct {
    cb_cb_test_t test;
    cb_cb_get_t get;
    cb_cb_read_t read;
    cb_cb_show_t show;
} cb_interface_t;

typedef struct config_container {
    cb_interface_t intf;
    config_header_t *head;
    config_body_info_t body_info;
    config_tail_t *tail;
    int type;
} config_container_t;

#define CB_HEAD_AND_TAIL_SIZE (sizeof(config_header_t) + sizeof(config_tail_t))

int cb_create_configure_block(config_container_t *cfg, void *data, int len);
int cb_set_configure_block_data_ptr(config_container_t *cfg, char *data, int len, int type);
void cb_destory(config_container_t *cfg);
int cb_max_block_size(void);
bool cb_test(config_container_t *cfg, CB_DTYPE type);
int cb_get(config_container_t *cfg, CB_DTYPE type);
void *cb_read(config_container_t *cfg, CB_DTYPE type, int param);
void cb_show(config_container_t *cfg);

#include "c0.h"
#include "c1.h"

#endif