/*
 * hexfile.h
 *
 * Created: 16/08/2020 09:42:13
 *  Author: A18425
 */ 


#ifndef HEXFILE_H_
#define HEXFILE_H_

#include "ihex.h"

//extern const char* hexarry[];
extern hex_data_t hexdata;


/*void hexarray_seek_begin();
char *hexarray_gets(char *str, int n, const char *fp[]);
*/
int set_default_segment_id(hex_data_t *dhex, ihex_segment_t segmentid);

#endif /* HEXFILE_H_ */