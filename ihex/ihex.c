#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ihex.h"
/*
Get buffer by the segment value
    @dhex: hex_data_t structure, the 
    
*/

/*
segment_buffer_t *get_segment_default(hex_data_t *dhex)
{
    //no segment record here, default use first
    return &dhex->segment[DEFAULT_SID_WITHOUT_SEGMENT_RECORD];
}
*/

int set_default_segment_id(hex_data_t *dhex, ihex_segment_t segmentid)
{
    segment_buffer_t *seg;
    int result = 0;

    for (int i = 0; i < MAX_SEGMENT_COUNT_IN_RECORDS; i++) {
        seg = &dhex->segment[i];
        if (!seg->sid && seg->addr_to && seg->data) {
            seg->sid = segmentid;
            result++;
        }
    }

    return result;
}

segment_buffer_t *get_segment_by_id(hex_data_t *dhex, ihex_segment_t segmentid)
{
    segment_buffer_t *seg;

    for (int i = 0; i < MAX_SEGMENT_COUNT_IN_RECORDS; i++) {
        seg = &dhex->segment[i];
        if (seg->sid == segmentid)
            return seg;
    }

    return NULL;
}

segment_buffer_t *get_segment_by_id_addr(hex_data_t *dhex, ihex_segment_t segmentid, ihex_address_t addr)
{
    segment_buffer_t *seg;

    for (int i = 0; i < MAX_SEGMENT_COUNT_IN_RECORDS; i++) {
        seg = &dhex->segment[i];
        if (seg->sid == segmentid && addr >= seg->addr_from && addr <= seg->addr_to)
            return seg;
    }

    return NULL;
}

/*
void walk_segments_by_id(hex_data_t *dhex, ihex_segment_t segmentid, int (*cb)(segment_buffer_t *, void *), void *param)
{
    segment_buffer_t *seg;
    int result = 0;

    for (int i = 0; i < MAX_SEGMENT_COUNT_IN_RECORDS; i++) {
        seg = &dhex->segment[i];
        if (seg->sid == segmentid) {
            if (cb) {
                result = cb(seg, param);
                if (result)
                    break;
            }
        }
    }
}
*/

/*
the body of create segment informantion by sgmentid, addr, len, and data,
if data is null, the seg only record sgmentid/from/to informantion
    The new segment may not combined together if address is increasing discontinously
@dhex: hex_data_t created by create_dhex()
@segmentid: segment id
@addr: addr for the new data
@len: len for the new data
@data: data pointer
return seg pointer if success, NULl if failed
*/
segment_buffer_t *_set_segment_data_by_id_addr(hex_data_t *dhex, ihex_segment_t segmentid, ihex_address_t addr, ihex_count_t len, char *data)
{
    segment_buffer_t *seg;
    ihex_address_t addr_to;
    ihex_count_t size;

    //search seg first
    for (int i = 0; i < MAX_SEGMENT_COUNT_IN_RECORDS; i++) {
        seg = &dhex->segment[i];
        //the segment exist, expand the address
        if (seg->sid == segmentid && seg->addr_to) {
            if (addr >= seg->addr_from && addr <= seg->addr_to + 1) {   //in buffer, or continous at tail coul call realloc
                addr_to = addr + len - 1;
                addr_to = max(addr_to, seg->addr_to);
                size = addr_to - seg->addr_from + 1;
                seg->addr_to = addr_to;

                if (data) {
                    if (seg->data) {
                        if (size > seg->len) {
                            seg->data = realloc(seg->data, size);
                            if (seg->data)
                                seg->len = size;
                            else
                                seg->len = 0;// report mem error
                        }
                    }
                    else {
                        seg->data = malloc(size);
                        if (seg->data)
                            seg->len = size;
                        else
                            seg->len = 0;// report mem error
                    }
                }

                if (seg->data && data)
                    memcpy(seg->data + addr - seg->addr_from, data, len);
                
                return seg;
            }
        }
    }

    //Not found existing segment in current list
    for (int i = 0; i < MAX_SEGMENT_COUNT_IN_RECORDS; i++) {
        seg = &dhex->segment[i];
        //get an unused seg
        if (!seg->sid && !seg->addr_from && !seg->addr_to) {
            if (data) {
                seg->data = malloc(len);
                if (seg->data) {
                    seg->len = len;
                    memcpy(seg->data, data, len);
                }
                else
                    seg->len = 0; // report mem error
            }

            seg->sid = segmentid;
            seg->addr_from = addr;
            seg->addr_to = addr + len - 1;
            return seg;
        }
    }

    return NULL;
}

/*
calculate record sgmentid from/to informantion
The new segment may not combined together is address if address is increasing discontinously
    @dhex: hex_data_t created by create_dhex()
    @segmentid: segment id
    @addr: addr for the new data
    @len: len for the new data
return seg pointer if success, NULl if failed
*/
segment_buffer_t *_set_segment_range_by_id_addr(hex_data_t *dhex, ihex_segment_t segmentid, ihex_address_t addr, ihex_count_t len)
{
    segment_buffer_t *seg;
    ihex_address_t addr_from, addr_to;
    
    addr_from = addr;
    addr_to = addr + len - 1;

    for (int i = 0; i < MAX_SEGMENT_COUNT_IN_RECORDS; i++) {
        seg = &dhex->segment[i];
        //the segment exist, expand the address
        if (seg->sid == segmentid && seg->addr_to) {
            if ((addr_from >= seg->addr_from && addr_from <= seg->addr_to + 1) ||
                (addr_to >= seg->addr_from - 1 && addr_to <= seg->addr_to) ||
                (addr_from <= seg->addr_from && addr_to >= seg->addr_to)) {
                seg->addr_from = min(addr_from, seg->addr_from);
                seg->addr_to = max(addr_to, seg->addr_to);
                return seg;
            }
        }
    }

    //Not found existing segment in current list
    for (int i = 0; i < MAX_SEGMENT_COUNT_IN_RECORDS; i++) {
        seg = &dhex->segment[i];
        //get an unused seg
        if (!seg->sid && !seg->addr_from && !seg->addr_to) {
            seg->sid = segmentid;
            seg->addr_from = addr;
            seg->addr_to = addr + len - 1;
            return seg;
        }
    }

    return NULL;
}

/*
create segment informantion by sgmentid, addr, len, and data,
    if SEG_ALLOC_MEMORY is not set(or data is invalid), the seg only record sgmentid/from/to informantion
    @dhex: hex_data_t created by create_dhex()
    @segmentid: segment id
    @addr: addr for the new data
    @len: len for the new data
    @data: data pointer
    @flag: flag options
        SEG_ALLOC_MEMORY: alloc memory
    return seg pointer if success, NULl if failed
*/
segment_buffer_t *set_segment_data_by_id_addr(hex_data_t *dhex, ihex_segment_t segmentid, ihex_address_t addr, ihex_count_t len, char *data, int flag)
{
    if (flag & SEG_ALLOC_MEMORY)
        return _set_segment_data_by_id_addr(dhex, segmentid, addr, len, data);
    else
        return _set_segment_range_by_id_addr(dhex, segmentid, addr, len);
}

/*
Hex reading callback for each record.
    it will be passed to ihex_state structure as a pointer(void *cb_func) together with arguments(void *args).
    @ihex: ihex_state structure, initialiezed at ihex_init(), store parser state and data
        @flag:
            SEG_EXPAND_MEMORY: use the maximum memory boundary
            SEG_ALLOC_MEMORY: alloc memory in to each segment
    @type: record type
    @checksum_error: checksum result of the record
*/
ihex_bool_t ihex_data_read(struct ihex_state *ihex,
    ihex_record_type_t type,
    ihex_bool_t checksum_error) {
    hex_data_t *dhex = ihex->args;
    ihex_segment_t sid;
    
#ifndef IHEX_DISABLE_SEGMENTS
    sid = ihex->segment;
#else
    sid = 0;
#endif

    if (type == IHEX_DATA_RECORD) {
        //start = (ihex_address_t)IHEX_LINEAR_ADDRESS(ihex);
        set_segment_data_by_id_addr(dhex, sid, ihex->address, ihex->length, ihex->data, dhex->flag);
    }
    else if (type == IHEX_EXTENDED_SEGMENT_ADDRESS_RECORD) {
    
    }
    else if (type == IHEX_END_OF_FILE_RECORD) {
    
    }

    return true;
}

/*
Read dhex content, and process in cb_read()
    @fp: file pointer
    @cb_read: callback function for process each record
    @args: arguments for cb
    return true if success, else failed
*/
ihex_bool_t dhex_read(FILE *fp, cb_ihex_data_read_t cb_read, void *args)
{
    struct ihex_state ihex;
    unsigned long line_number = 1L;
    ihex_count_t count;
    char buf[256];

    fseek(fp, 0, SEEK_SET);

    ihex_read_at_address(&ihex, 0, cb_read, args);
    while (fgets(buf, sizeof(buf), fp)) {
        count = (ihex_count_t)strlen(buf);
        ihex_read_bytes(&ihex, buf, count);
        line_number += (count && buf[count - 1] == '\n');
    }
    ihex_end_read(&ihex);

    return true;
}

/*
Load file into hex data structure
    @file: hex file to read
    @dhex: dhex data structure
    return 0 if sucess else failed
*/
int load_segments_from_file(const char *file, hex_data_t *dhex)
{
    FILE *infile = stdin;
    int result = 0;

    if (!(infile = fopen(file, "r"))) {
        return -2;
    }
    
    //walk without alloc memory (for data range combination)
    dhex->flag = 0;
    if (!dhex_read(infile, ihex_data_read, dhex)) {
        result = -3;
        goto out;
    }

    //alloc memory and read data
    dhex->flag = SEG_ALLOC_MEMORY;
    if (!dhex_read(infile, ihex_data_read, dhex)) {
        result = -4;
        goto out;
    }

out:
    if (infile != stdin) {
        (void)fclose(infile);
    }

    return result;
}

void unload_segment_by_sid(hex_data_t *dhex, ihex_segment_t segmentid)
{
    segment_buffer_t *seg;
    int i;

    //alloc data buffer
    for (i = 0; i < _countof(dhex->segment); i++) {
        seg = &dhex->segment[i];
        if (seg->sid == segmentid) {
            if (seg->data)
                free(seg->data);
            memset(seg, 0, sizeof(*seg));
        }
    }
}

void unload_segments(hex_data_t *dhex)
{
    segment_buffer_t *seg;
    int i;

    //alloc data buffer
    for (i = 0; i < _countof(dhex->segment); i++) {
        seg = &dhex->segment[i];
        if (seg->data)
            free(seg->data);

        memset(seg, 0, sizeof(*seg));
    }
}

hex_data_t * get_hex_info_from_file(const char *file)
{
    hex_data_t *dhex;
    int result = 0;

    dhex = malloc(sizeof(*dhex));
    if (!dhex)
        return NULL;

    memset(dhex, 0, sizeof(*dhex));

    result = load_segments_from_file(file, dhex);
    if (result) {
        free(dhex);
        return NULL;
    }

    return dhex;
}

void release_dhex(hex_data_t *dhex)
{
    if (dhex) {
        unload_segments(dhex);
        free(dhex);
    }
}

void ihex_flush_buffer(struct ihex_state *ihex, char *buffer, char *eptr) 
{
    FILE *outfile = (FILE *)ihex->args;

    *eptr = '\0';
    (void)fputs(buffer, outfile);
}

int save_hex_info_to_file(const char *file, const hex_data_t *dhex)
{
    struct ihex_state ihex;
    bool write_initial_address = 0;
    bool debug_enabled = 1;

    const segment_buffer_t *seg;
    int i;

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
    
    for (i = 0; i < _countof(dhex->segment); i++) {
        seg = &dhex->segment[i];
        if (seg->data) {
            ihex_write_at_segment(&ihex, seg->sid, seg->addr_from);
            //ihex_write_at_address(&ihex, seg->addr_from);
            if (write_initial_address) {
                if (debug_enabled) {
                    (void)fprintf(stderr, "Address offset: 0x%lx\n",
                        (unsigned long)ihex.address);
                }
                ihex.flags |= IHEX_FLAG_ADDRESS_OVERFLOW;
            }
            ihex_write_bytes(&ihex, seg->data /*+ seg->offset*/, seg->len);
        }
    }

    ihex_end_write(&ihex);

    if (outfile != stdout) {
        (void)fclose(outfile);
    }

    return 0;
}