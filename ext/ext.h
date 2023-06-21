#ifndef __EXT_H
#define __EXT_H
#include "include/blk.h"
#include "infoblock/ib.h"
#include "cfgblock/cb.h"

void set_infoblock_locaton(bool userrow);
int ext_get_storage_type(B_BLOCK_TYPE btype);
int ext_get_storage_offset(B_BLOCK_TYPE btype);
int ext_create_data_block(void *contnr, void *data, int len, B_BLOCK_TYPE btype);
int ext_info_set_data_ptr(void *contnr, char *data, int len, unsigned short flag);
void ext_destory(void *contnr);
int ext_info_max_size(B_BLOCK_TYPE btype);
bool ext_is(void *contnr, B_BLOCK_TYPE btype);
bool ext_test(void *contnr, int type);
int ext_get(void *contnr, int type);
void *ext_read(void *contnr, int type, int param);
void ext_show(void *contnr);

#endif /* __EXT_H */