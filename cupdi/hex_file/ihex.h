#ifndef __IHEX_H
#define __IHEX_H

#ifdef CUPDI

#include <stdint-gcc.h>

#define MAX_SEGMENT_COUNT_IN_RECORDS 64

typedef uint_least32_t ihex_address_t;
typedef uint_least16_t ihex_segment_t;

typedef struct _segment_buffer {
#define DEFAULT_SID_WITHOUT_SEGMENT_RECORD 0
    ihex_segment_t  sid;    //segment id

    ihex_address_t addr_from;
    ihex_address_t addr_to;

    const char *data;    //buffer pointer
    int len;       //buffer data len

}segment_buffer_t;

typedef struct _hex_data {
    segment_buffer_t segment[MAX_SEGMENT_COUNT_IN_RECORDS]; //index 0 for record without segment id(address)

#define SEG_ALLOC_MEMORY (1 << 0)
    int flag;
}hex_data_t;

#define ADDR_TO_SEGMENTID(_addr) ((_addr) >> 4)
#define SEGMENTID_TO_ADDR(_segid) ((_segid) << 4)

extern hex_data_t hexdata;
#endif

#endif
