#ifndef __FOP_H
#define __FOP_H

typedef enum { HEX_FORMAT = 0, DEC_FORMAT, TYPES_DATA_FORMAT } datatype_t;

char *trim_name_with_extesion(const char *name, const char a_delim, int order, const char *tailname);
int search_defined_array_int_from_file(const char *file, const char *varname, unsigned int *output, int outlen, unsigned int invalid, datatype_t type);
int search_defined_value_int_from_file(const char *file, const char *varname, unsigned int *output, datatype_t type);
int search_map_value_int_from_file(const char *file, const char *varname, unsigned int *output);

#endif