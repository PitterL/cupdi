#ifndef __IHEX_H
#define __IHEX_H

#include "kk_ihex_read.h"

typedef struct _hex_info {
    ihex_address_t addr_from;
    ihex_address_t addr_to;
    ihex_count_t total_size;
    ihex_count_t actual_size;
    unsigned char *data;
    int len;
}hex_info_t;

int get_hex_info(const char *file, hex_info_t *info);
#endif
