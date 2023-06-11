#ifndef __EXT_H
#define __EXT_H
#include "include/blk.h"
#include "infoblock/ib.h"
#include "cfgblock/cb.h"

int ext_create_data_block(void *contnr, void *data, int len, int flag);
int ext_info_set_data_ptr(void *contnr, char *data, int len, int flag);
void ext_destory(void *contnr, int flag);
int ext_info_max_size(int flag);
bool ext_test(void *contnr, int type, int flag);
int ext_get(void *contnr, int type, int flag);
void *ext_read(void *contnr, int type, int param, int flag);
void ext_show(void *contnr, int flag);

#endif /* __EXT_H */