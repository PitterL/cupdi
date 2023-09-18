#ifndef __IHEX_H
#define __IHEX_H

#include "kk_ihex_read.h"
#include "kk_ihex_write.h"

#define MAX_SEGMENT_COUNT_IN_RECORDS 64
#define MAX_DATA_IN_ONE_SEGMENT 0x10000 // 16 bits address of hex data record

typedef struct _segment_buffer
{
    /* active segment id */
    /* The 32-bit Extended Linear Address Record is used to 
        specify bits 16-31 of the Linear Base Address (LBA) */
    /* The 16-bit Extended Segment Address Record is used to 
        specify bits 4-19 of the Segment Base Address (SBA)*/
    ihex_segment_t sid;
    
    
    ihex_seg_type_t flag;

    ihex_address_t addr_from;
    ihex_address_t addr_to;

    char *data; // buffer pointer
    int len;    // buffer data len

} segment_buffer_t;

#define VALID_SEG(_seg) (!!(_seg->addr_to))

typedef union
{
    struct
    {
/* bit 0~15 segment type */
#define SEG_EX_SEGMENT_ADDRESS (1 << 0)
#define SEG_EX_LINEAR_ADDRESS (1 << 1)
#define SEG_EX_TYPE_MASK (0xFFFF)
        ihex_seg_type_t seg;

//  bit 16~31 hex type
#define HEX_INIT_SEGMENT (1 << 0)
#define HEX_ALLOC_MEMORY (1 << 1)
//#define HEX_MAGIC_OFFSET_ADDRESS (1 << 2)
#define HEX_TYPE_SHIFT (16)
#define HEX_TYPE_MASK (0xFFFF)
        ihex_hex_flag_t hex;
    };
    int value;
} ihex_flag_t;
#define GET_SEG_TYPE(_s) ((_s) & SEG_EX_TYPE_MASK)
#define GET_HEX_TYPE(_h) (((_h) >> HEX_TYPE_SHIFT) & HEX_TYPE_MASK)
#define HEX_TYPE(_h, _s) ((((_h) & HEX_TYPE_MASK) << HEX_TYPE_SHIFT) | ((_s) & SEG_EX_TYPE_MASK))

typedef struct _hex_data
{
    segment_buffer_t segments[MAX_SEGMENT_COUNT_IN_RECORDS]; // index 0 for record without segment id(address)
    ihex_flag_t flag;
} hex_data_t;

#define EX_SEGMENT_ADDRESS_SHIFT 4
#define ADDR_TO_EX_SEGMENT_ID(_addr) ((_addr) >> 4)
#define ADDR_OFFSET_EX_SEGMENT(_addr) ((_addr) & 0xF)
#define EX_SEGMENT_ID_TO_ADDR(_segid) ((_segid) << 4)

#define EX_LINEAR_ADDRESS_SHIFT 16
#define ADDR_TO_EX_LINEAR_ID(_addr) ((_addr) >> 16)
#define ADDR_OFFSET_EX_LINEAR(_addr) ((_addr) & 0xFFFF)
#define EX_LINEAR_ID_TO_ADDR(_segid) ((_segid) << 16)
#define LINEAR_ID_MAGIC(_segid) ((_segid) & 0x80)

segment_buffer_t *get_segment_by_id(hex_data_t *dhex, ihex_segment_t segmentid);
segment_buffer_t *get_segment_by_id_addr(hex_data_t *dhex, ihex_segment_t segmentid, ihex_address_t addr);
//int set_default_segment_id(hex_data_t *dhex, ihex_segment_t source, ihex_segment_t target);
int walk_segments_by_id(hex_data_t *dhex, ihex_seg_type_t flag, int (*cb)(segment_buffer_t *, const void *, ihex_seg_type_t), const void *param);

int load_segments_from_file(const char *file, hex_data_t *dhex);
void unload_segment_by_sid(hex_data_t *dhex, ihex_segment_t segmentid);
void unload_segments(hex_data_t *dhex);
segment_buffer_t *set_segment_data_by_id_addr(hex_data_t *dhex, ihex_segment_t segmentid, ihex_address_t addr, ihex_count_t len, char *data, int flag);
//hex_data_t *get_hex_info_from_file(const char *file);
void release_dhex(hex_data_t *dhex);
int save_hex_info_to_file(const char *file, const hex_data_t *dhex);
#endif