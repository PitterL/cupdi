#ifndef __IHEX_H
#define __IHEX_H

#include "kk_ihex_read.h"
#include "kk_ihex_write.h"

typedef struct _hex_data {
    ihex_address_t addr_from;
    ihex_address_t addr_to;
    ihex_count_t total_size;
    ihex_count_t actual_size;

    //data buffer
    unsigned char *data;
    int len;
    int offset;
}hex_data_t;

int get_hex_info(const char *file, hex_data_t *info);
int save_hex_info(const char *file, hex_data_t *dhex);
#endif
