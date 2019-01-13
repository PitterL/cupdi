#ifndef __IHEX_H
#define __IHEX_H

#include "kk_ihex_read.h"
#include "kk_ihex_write.h"

#define MAX_SEGMENT_COUNT_IN_RECORDS 64

typedef struct _segment_buffer {
#define DEFAULT_SID_WITHOUT_SEGMENT_RECORD 0
    ihex_segment_t  sid;    //segment id

    ihex_address_t addr_from;
    ihex_address_t addr_to;

    char *data;    //buffer pointer
    int len;    //buffer data len

}segment_buffer_t;

typedef struct _hex_data {
    segment_buffer_t segment[MAX_SEGMENT_COUNT_IN_RECORDS]; //index 0 for record without segment id(address)

#define SEG_ALLOC_MEMORY (1 << 0)
    int flag;
}hex_data_t;

#define ADDR_TO_SEGMENTID(_addr) ((_addr) >> 4)
#define SEGMENTID_TO_ADDR(_segid) ((_segid) << 4)

segment_buffer_t *get_segment_by_id(hex_data_t *dhex, ihex_segment_t segmentid);
segment_buffer_t *get_segment_by_id_addr(hex_data_t *dhex, ihex_segment_t segmentid, ihex_address_t addr);
int set_default_segment_id(hex_data_t *dhex, ihex_segment_t segmentid);

int load_segments_from_file(const char *file, hex_data_t *dhex);
void unload_segment_by_sid(hex_data_t *dhex, ihex_segment_t segmentid);
void unload_segments(hex_data_t *dhex);
segment_buffer_t *set_segment_data_by_id_addr(hex_data_t *dhex, ihex_segment_t segmentid, ihex_address_t addr, ihex_count_t len, char *data, int flag);
hex_data_t * get_hex_info_from_file(const char *file);
void release_dhex(hex_data_t *dhex);
int save_hex_info_to_file(const char *file, const hex_data_t *dhex);
#endif
