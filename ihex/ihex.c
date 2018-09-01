#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ihex.h"

ihex_bool_t ihex_data_read(struct ihex_state *ihex,
    ihex_record_type_t type,
    ihex_bool_t checksum_error) {
    hex_info_t *info = ihex->args;
    unsigned long start, end, off=0;

    if (type == IHEX_DATA_RECORD) {
        start = (unsigned long)IHEX_LINEAR_ADDRESS(ihex);
        end = start + ihex->length - 1;

        if (info->data) {
            if (start >= info->addr_from && end < info->addr_from + info->len) {
                off = start - info->addr_from;
                memcpy(info->data + off, ihex->data, ihex->length);
            }
        }
        else {
            if (info->actual_size) {
                if (info->addr_from > start)
                    info->addr_from = start;

                if (info->addr_to < end)
                    info->addr_to = end;
            }
            else {
                info->addr_from = start;
                info->addr_to = end;
            }
            info->actual_size += ihex->length;
        }
    }
    else if (type == IHEX_END_OF_FILE_RECORD) {
        if (!info->data)
            info->total_size = info->addr_to - info->addr_from + 1;
    }

    return true;
}

int get_hex_info(const char *file, hex_info_t *info)
{
    struct ihex_state ihex;
    unsigned long line_number = 1L;
    ihex_count_t count;
    char buf[256];

    FILE *infile = stdin;

    if (!(infile = fopen(file, "r"))) {
        return -2;
    }

    ihex_read_at_address(&ihex, 0, ihex_data_read, info);
    while (fgets(buf, sizeof(buf), infile)) {
        count = (ihex_count_t)strlen(buf);
        ihex_read_bytes(&ihex, buf, count);
        line_number += (count && buf[count - 1] == '\n');
    }
    ihex_end_read(&ihex);

    if (infile != stdin) {
        (void)fclose(infile);
    }

    return 0;
}