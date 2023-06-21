#ifndef __INFORMATION_BLOCK_H
#define __INFORMATION_BLOCK_H

#include "../include/blk.h"

#define INFO_BLOCK_S_VER_MAJOR 's'

#define B_MAJOR_OP(_t, _m)  (((_t) << 24) | ((_m) << 16))
#define IB_OP(_m) B_MOP(_m)

typedef ext_header_t info_header_t;

typedef union _information_header {
    info_header_t data;
    unsigned int value;
}information_header_t;

typedef struct _build_number {
    unsigned char minor : 4;
	unsigned char major : 4;
}build_number_t;

#pragma pack(1)
typedef struct _fw_ver {
    char ver[3];
    build_number_t build;
} fw_ver_t;
#pragma pack()

typedef union _firmware_version {
    fw_ver_t data;
    unsigned int value;
}firmware_version_t;
enum { IB_FW_VER_ST=SUB_OP_START(IB_FW_VER), IB_FW_VER_NAME_N0 = IB_FW_VER_ST, IB_FW_VER_NAME_N1, IB_FW_VER_NAME_N2, IB_FW_VER_NAME_BUILD_MAJOR, IB_FW_VER_NAME_BUILD_MINOR};

#pragma pack(1)
    typedef struct _fw_size {
    unsigned int size;
} fw_size_t;
#pragma pack()

typedef union _firmware_size {
    fw_size_t data;
    unsigned int value;
}firmware_size_t;
enum {IB_FW_SIZE_ST = SUB_OP_START(IB_FW_SIZE)};

typedef ext_header_t config_info_t;

typedef union {
    config_info_t data;
    unsigned int value;
}config_information_t;
enum { IB_CFG_ST = SUB_OP_START(IB_CFG), IB_CFG_VER = IB_CFG_ST, IB_CFG_SIZE };

typedef struct {
    header_version_t version;
    unsigned char size;
    unsigned char crc;
}fuse_info_t;

typedef union {
    fuse_info_t data;
    unsigned int value;
}fuse_information_t;
enum { IB_FUSE_ST = SUB_OP_START(IB_FUSE), IB_FUSE_VER = IB_FUSE_ST, IB_FUSE_SIZE, IB_FUSE_CRC };

typedef struct _info_crc {
    unsigned int fw : 24;
    unsigned int info : 8;
}info_crc_t;

typedef union _information_crc {
    info_crc_t data;
    unsigned int value;
}information_crc_t;
enum { IB_CRC_ST = SUB_OP_START(IB_CRC), IB_CRC_FW = IB_CRC_ST, IB_CRC_INFO };

#define IB_HEAD_AND_TAIL_SIZE (sizeof(information_header_t) + sizeof(information_crc_t))

#pragma pack(1)
typedef struct {
    unsigned short ds;
    unsigned short dr;
} ds_dr_t;
#pragma pack()

typedef union {
    ds_dr_t data;
    unsigned int value;
}dsdr_address_t;

#pragma pack(1)
typedef struct {
    unsigned short acq;
    unsigned short node;
} acq_node_t;
#pragma pack()

typedef union {
    acq_node_t data;
    unsigned int value;
}acqnode_address_t;
enum { IB_REG_ST = SUB_OP_START(IB_REG), IB_REG_SR_SIGNAL = IB_REG_ST, IB_REG_SR_REF, IB_REG_AN_ACQ, IB_REG_AN_NODE, IB_REG_END };

typedef struct {
    dsdr_address_t dsdr;
    acqnode_address_t acqnd;
} varible_address_t;

typedef union {
    varible_address_t var_addr;
    unsigned short data[IB_REG_END - IB_REG_ST];
} varible_address_array_t;

typedef bool (*cb_ib_test_t)(int type);
typedef int (*cb_ib_get_t)(information_header_t *head, int type);
typedef void (*cb_ib_show_t)(information_header_t *head);
typedef struct ib_interface{
    cb_ib_test_t test;
    cb_ib_get_t get;
    cb_ib_show_t show;
}ib_interface_t;

typedef struct information_container {
    container_header_t header;
    ib_interface_t intf;
    information_header_t *head;
}information_container_t;

typedef struct {
    int fw_crc24;
    int fw_size;
    int fw_version;
    varible_address_t var_addr;
    fuse_information_t fuse;
    config_information_t config;
}information_content_params_t;

int ib_create_information_block(information_container_t *info, information_content_params_t *param, int len);
int ib_set_information_block_data_ptr(information_container_t *info, char *data, int len, unsigned short flag);
void ib_destory(information_container_t *info);
int ib_max_block_size(void);
bool ib_test(information_container_t *info, IB_DTYPE type);
int ib_get(information_container_t *info, IB_DTYPE type);
void ib_show(information_container_t *info);
bool ib_is_container(container_header_t *ct);
bool ib_is_head(info_header_t *head);

#include "s1.h"
#include "s2.h"
#include "s3.h"

#endif