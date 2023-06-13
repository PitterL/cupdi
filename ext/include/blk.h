#ifndef __BLK_H
#define __BLK_H

typedef enum { B_HEAD = 1, B_VER, B_SIZE, B_CRC, B_DATA_TYPES } BLOCK_DTYPE;

#define B_DATA_MAX_TYPES 31
#define SUB_OP_START(_dt) ((_dt) * 32)
#define TO_OP(_dt) ((_dt) > B_DATA_MAX_TYPES ? ((_dt)/ 32) : (_dt))

typedef union _header_version {
    char ver[2];
    unsigned short value;
}header_version_t;
 
#pragma pack(1)
typedef struct {
    header_version_t version;
    unsigned short size;
}ext_header_t;
#pragma pack()

enum { B_HEAD_ST=SUB_OP_START(B_HEAD), B_HEAD_VER_N0, B_HEAD_VER_N1, B_HEAD_SIZE };

#define _VER(_h, _n) (_h).version.ver[(_n)]
#define VALID_HEADER(_h) ((_VER(_h, 0) >= 'A' && _VER(_h, 0) <= 'z') && (_VER(_h, 1) >= '0' && _VER(_h, 1) <= '9'))
#define HEADER_MINOR(_h, _m0) ((_VER(_h, 0) >= 'A' && _VER(_h, 0) <= 'z') && (_VER(_h, 1) >= (_m0) && _VER(_h, 1) <= '9'))

/*
    head memory type
    MEM_ALLOC: alloc new memory and copy data
    MEM_SHARE: share exist memory
    MEM_SHARE_RELEASE: share exist memory, and will release it when destory
*/
typedef enum {MEM_ALLOC, MEM_SHARE, MEM_SHARE_RELEASE, TYPES_MEM_OPS} B_MEM_TYPE;

/*
    head container type
    BLOCK_INFO: it's information block
    BLOCK_CFG: it's config block
*/
typedef enum {BLOCK_INFO = TYPES_MEM_OPS, BLOCK_CFG, TYPES_BLOCK} B_BLOCK_TYPE;

#include "vardef.h"

#endif /*__BLK_H*/