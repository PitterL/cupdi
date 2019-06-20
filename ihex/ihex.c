#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ihex.h"

ihex_bool_t ihex_data_read(struct ihex_state *ihex,
    ihex_record_type_t type,
    ihex_bool_t checksum_error) {
    hex_data_t *dhex = ihex->args;
    unsigned long start, end, off=0;

    if (type == IHEX_DATA_RECORD) {
        start = (unsigned long)IHEX_LINEAR_ADDRESS(ihex);
        end = start + ihex->length - 1;

        if (dhex->data) {
            if (start >= dhex->addr_from && end < dhex->addr_from + dhex->len) {
                off = start - dhex->addr_from;
                memcpy(dhex->data + dhex->offset + off, ihex->data, ihex->length);
            }
        }
        else {
            if (dhex->actual_size) {
                if (dhex->addr_from > start)
                    dhex->addr_from = start;

                if (dhex->addr_to < end)
                    dhex->addr_to = end;
            }
            else {
                dhex->addr_from = start;
                dhex->addr_to = end;
            }
            dhex->actual_size += ihex->length;
        }
    }
    else if (type == IHEX_END_OF_FILE_RECORD) {
        if (!dhex->data)
            dhex->total_size = dhex->addr_to - dhex->addr_from + 1;
    }

    return true;
}

int get_hex_info(const char *file, hex_data_t *dhex)
{
    struct ihex_state ihex;
    unsigned long line_number = 1L;
    ihex_count_t count;
    char buf[256];

    FILE *infile = stdin;

    if (!(infile = fopen(file, "r"))) {
        return -2;
    }

    ihex_read_at_address(&ihex, 0, ihex_data_read, dhex);
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

void ihex_flush_buffer(struct ihex_state *ihex, char *buffer, char *eptr) 
{
    FILE *outfile = (FILE *)ihex->args;

    *eptr = '\0';
    (void)fputs(buffer, outfile);
}

int save_hex_info(const char *file, hex_data_t *dhex)
{
    struct ihex_state ihex;
    bool write_initial_address = 0;
    bool debug_enabled = 1;

    FILE *outfile = stdout;

    if (!(outfile = fopen(file, "w"))) {
        return -2;
    }

#ifdef IHEX_EXTERNAL_WRITE_BUFFER
    // How to provide an external write buffer with limited duration:
    char buffer[IHEX_WRITE_BUFFER_LENGTH];
    ihex_write_buffer = buffer;
#endif
    ihex_init(&ihex, ihex_flush_buffer, outfile);
    ihex_write_at_address(&ihex, dhex->addr_from);
    if (write_initial_address) {
        if (debug_enabled) {
            (void)fprintf(stderr, "Address offset: 0x%lx\n",
                (unsigned long)ihex.address);
        }
        ihex.flags |= IHEX_FLAG_ADDRESS_OVERFLOW;
    }
    ihex_write_bytes(&ihex, dhex->data + dhex->offset, dhex->total_size);
    
    ihex_end_write(&ihex);

    if (outfile != stdout) {
        (void)fclose(outfile);
    }

    return 0;
}