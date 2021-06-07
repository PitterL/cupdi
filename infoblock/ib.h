#ifndef __INFORMATION_BLOCK_H
#define __INFORMATION_BLOCK_H

typedef enum { IB_HEAD = 1, IB_FW_VER, IB_FW_SIZE, IB_CRC, IB_REG_SR, IB_DATA_TYPES } IB_DTYPE;

#define SUB_OP_START(_dt) ((_dt) * 32)
#define TO_OP(_dt) ((_dt) > IB_DATA_TYPES ? ((_dt)/ 32) : (_dt))

typedef union _header_version {
    char ver[2];
    unsigned short value;
}header_version_t;

PACK(
    typedef struct _info_header {
    header_version_t version;
    unsigned short size;
}) info_header_t;
enum { IB_HEAD_ST=SUB_OP_START(IB_HEAD), IB_HEAD_VER_N0, IB_HEAD_VER_N1, IB_HEAD_SIZE };

typedef union _information_header {
    info_header_t data;
    unsigned int value;
}information_header_t;

typedef struct _build_number {
    unsigned char minor : 4;
	unsigned char major : 4;
}build_number_t;

PACK(
    typedef struct _fw_ver {
    char ver[3];
    build_number_t build;
}) fw_ver_t;

typedef union _firmware_version {
    fw_ver_t data;
    unsigned int value;
}firmware_version_t;
enum { IB_FW_VER_ST=SUB_OP_START(IB_FW_VER), IB_FW_VER_NAME_N0, IB_FW_VER_NAME_N1, IB_FW_VER_NAME_N2, IB_FW_VER_NAME_BUILD_MAJOR, IB_FW_VER_NAME_BUILD_MINOR};

PACK(
    typedef struct _fw_size {
    unsigned short size;
    char rsv[2];
}) fw_size_t;

typedef union _firmware_size {
    fw_size_t data;
    unsigned int value;
}firmware_size_t;
enum {IB_FW_SIZE_ST = SUB_OP_START(IB_FW_SIZE)};

typedef struct _info_crc {
    unsigned int fw : 24;
    unsigned int info : 8;
}info_crc_t;

typedef union _information_crc {
    info_crc_t data;
    unsigned int value;
}information_crc_t;
enum { IB_CRC_ST = SUB_OP_START(IB_CRC), IB_CRC_FW, IB_CRC_INFO };

PACK(
typedef struct _ds_dr {
    unsigned short ds;
    unsigned short dr;
}) ds_dr_t;

typedef union _dsdr_address {
    ds_dr_t data;
    unsigned int value;
}dsdr_address_t;
enum { IB_IB_REG_SR_ST = SUB_OP_START(IB_REG_SR), IB_REG_SR_SIGNAL, IB_REG_SR_REF};

//typedef int (*cb_ib_build_t)(information_container **info, int fw_crc, int version, int dsdr);

typedef bool (*cb_ib_test_t)(int type);
typedef int (*cb_ib_get_t)(information_header_t *head, int type);
typedef void (*cb_ib_show_t)(information_header_t *head);
typedef struct ib_interface{
    cb_ib_test_t test;
    cb_ib_get_t get;
    cb_ib_show_t show;
}ib_interface_t;

/*
    head memory type
    MEM_ALLOC: alloc new memory and copy data
    MEM_SHARE: share exist memory
    MEM_SHARE_RELEASE: share exist memory, and will release it when destory
*/
typedef enum {MEM_ALLOC, MEM_SHARE, MEM_SHARE_RELEASE} IB_MEM_TYPE;
typedef struct information_container {
    ib_interface_t intf;
    information_header_t *head;
    int type;
}information_container_t;

int infoblock_set_data_ptr(information_container_t *info, char *data, int len, int type);
void ib_destory(information_container_t *info);
int ib_max_block_size(void);
bool ib_test(information_container_t *info, IB_DTYPE type);
int ib_get(information_container_t *info, IB_DTYPE type);
void ib_show(information_container_t *info);

#include "s1.h"
#include "s2.h"

#endif