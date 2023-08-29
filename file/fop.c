#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include <regex/re.h>
#include <string/strlcat.h>
#include <string/join.h>
#include <string/split.h>
#include <string/strndup.h>
#include <os/platform.h>

#include "fop.h"
/*
Combine the input <main name> + <ext name>, user should release the memory after use.
@name: original name
@a_delim: delimiter to trim
@order: skip chars from begin
@tailname: extensition name
@return new combined name, NULL means failed
*/
char *trim_name_with_extesion(const char *name, const char a_delim, int order, const char *tailname)
{
    char *new_name;
    int i, size, mainsize, extsize, new_size;
    int found = 0;

    if (!name || !tailname) {
        fprintf(stderr, "Name or Extname is Null\n");
        return NULL;
    }

    if (order <= 0)
        order = 1;

    //search delimiter position
    size = (int)strlen(name) + 1;
    extsize = (int)strlen(tailname);
    for (i = -1; i >= -size; i--) {
        if (name[size + i] == a_delim)
            found++;

        if (found == order)
            break;
    }

    if (found != order) {
        fprintf(stderr, "delimiter not found\n");
        i = -1; // add extension directly
    }

    mainsize = size + i;
    new_size = mainsize + extsize + 2; /*<trim>[d]<tailname>[NULL]*/
    new_name = malloc(new_size);
    if (!new_name) {
        fprintf(stderr, "Not enough memory.\n");
        return NULL;
    }

    strncpy(new_name, name, mainsize);
    new_name[mainsize] = a_delim;
    strncpy(new_name + mainsize + 1, tailname, extsize + 1); //Copy 'NULL'

    return new_name;
}

typedef int(*fn_search_defined_buf)(char *content, const char *pat_str, const char *pat_value, unsigned int *output, int outlen, unsigned int invalid);
/*
Get value content in source file, the format is: #define <varname> 0x12345678 (hex format, 32bit max)
@buffer: data content buff to search (the content will be changed after function called)
@pat_str: defined array regex trunk for search
@pat_val: defined regex element in array trunk for search
@output: varible value output array buffer
@outlen: varible value count
@invalid: if var in array not valid, filled to this value
@return how many varibles get, negative mean error occur
*/
int _search_defined_value_from_buf(char *content, const char *pat_str, const char *pat_value, unsigned int *output, int outlen, unsigned int invalid)
{
    tre_comp tregex;
    const char *st, *end = NULL;

    unsigned int val;
    int result = -2;

    if (!content || !pat_str || !pat_value || !output || !outlen)
        return -1;

    tre_compile(pat_str, &tregex);
    st = tre_match(&tregex, content, NULL);

    if (st) {
        result = 0;

        tre_compile(pat_value, &tregex);
        st = tre_match(&tregex, st, &end);
        if (st) {
			if (end > st/* && end - st <= 10*/) {	// 32bit hex number start with `0x`, so 10 chars(2+8), but there may be many ZERO pre-fix at the beginning, so no limited the length
				val = (unsigned int)strtol(st, NULL, 0); // 0 is error
				*output = val;
				result++;
			}
        }
    }

    return result;
}

/*
Get Array content in source file, the format is: #define <varname> {NULL, 0x6a, ...}
@buffer: data content buff to search (the content will be changed after function called)
@pat_str: defined array regex trunk for search
@pat_val: defined regex element in array trunk for search
@output: varible value output array buffer
@outlen: varible value count
@invalid: if var in array not valid, filled to this value
@return how many varibles get, negative mean error occur
*/
int _search_defined_array_from_buf(char *content, const char *pat_str, const char *pat_value, unsigned int *output, int outlen, unsigned int invalid)
{
    tre_comp tregex;
    const char *st, *end;
    char *trunk;

    const char array_delim = ',';
    char **tk_s;

    unsigned int val;
    int i;
    int result = -2;

    if (!content || !pat_str ||!pat_value || !output || !outlen)
        return -1;

    tre_compile(pat_str, &tregex);
    st = tre_match(&tregex, content, &end);

    if (st) {
        result = 0;
        //get data array start position
        while (st < end && *st != '{')
            st++;

        if (*st != '{') {
            fprintf(stderr, "Parse defined array search '{' failed");
            goto out;
        }

        if (end - st <= 2) {
            //no data here
            goto out;
        }

        //*end = '\0';    //replace '}' to terminate
        trunk = strndup(st + 1, end - st - 2);

        tk_s = str_split(trunk, array_delim);
        if (!tk_s) {
            fprintf(stderr, "Parse defined array str tk_s: %s failed", trunk);
            free(trunk);
            goto out;
        }

        tre_compile(pat_value, &tregex);
        for (i = 0; tk_s[i]; i++) {
            if (i < outlen) {
                if (tre_match(&tregex, tk_s[i], NULL)) {
                    val = (unsigned int)strtol(tk_s[i], NULL, 0); // 0 is error
                    output[i] = val;
                }
                else {
                    output[i] = invalid;
                }
                result++;
            }
            free(tk_s[i]);
        }
        free(tk_s);
        free(trunk);
    }

out:
    return result;
}

/*
Get Array content in source file, the format is: #define FUSES_CONTENT {NULL, 0x6a, ...}
@file: file to search
@pat_str: defined array regex trunk for search
@pat_val: defined regex element in array trunk for search
@output: varible value output array buffer
@outlen: varible value count
@invalid: if var in array not valid, filled to this value
@return how many varibles get, negative mean error occur
*/
int _search_defined_array_from_file(const char *file, fn_search_defined_buf fn_search, const char *pat_str, const char *pat_val, unsigned int *output, int outlen, unsigned int invalid)
{
    FILE * f = NULL;
    int status;

    char * line = NULL;
    size_t len = 0;
    int read;
    int result = -2;

    f = fopen(file, "r");
    if (!f) {
        fprintf(stderr, "Could not open '%s': %s.\n", file,
            strerror(errno));
        return -1;
    }

    while ((read = getline(&line, &len, f)) != -1) {
        /*
            printf("Retrieved line of length %zu:\n", read);
            printf("%s", line);
        */

        result = fn_search(line, pat_str, pat_val, output, outlen, invalid);
        if (result >= 0)
            break;
    }

    if (line)
        free(line);

    status = fclose(f);
    if (status != 0) {
        fprintf(stderr, "Error closing '%s': %s.\n", file,
            strerror(errno));
    }

    return result;
}

/*
Get int array content in source file, the format is: #define <varname> {NULL, 0x6a, ...}
@file: file to search
@varname: defined varible array name
@output: varible value output array buffer
@outlen: varible value count
@invalid: if var in array not valid, filled to this value
@return how many varibles get, negative mean error occur
*/
int search_defined_array_int_from_file(const char *file, const char *varname, unsigned int *output, int outlen, unsigned int invalid, datatype_t type)
{
    char pat_str[TRE_MAX_BUFLEN];
    const char *pat_raw[] = { "^#define", varname, "\\{[\\w\\s,]*\\}" };
    const char *space_delims = "\\s+";
    const char *pat_value;
    
    if (type == HEX_FORMAT) {
        pat_value = "0x[0-9a-fA-F]{1,8}";
    } else if (type == DEC_FORMAT) {
        pat_value = "[0-9]{1,}";
    } else {
        return -2;
    }

    //create pattern to search
    pat_str[0] = '\0';
    str_join(pat_str, sizeof(pat_str), pat_raw, ARRAY_SIZE(pat_raw), space_delims);

    return _search_defined_array_from_file(file, _search_defined_array_from_buf, pat_str, pat_value, output, outlen, invalid);
}

/*
Get value content in source file, the format is: #define <varname> 0x123456
@file: file to search
@varname: defined varible array name
@output: varible value output array buffer
@outlen: varible value count
@invalid: if var in array not valid, filled to this value
@return how many varibles get, negative mean error occur
*/
int search_defined_value_int_from_file(const char *file, const char *varname, unsigned int *output, datatype_t type)
{
    char pat_str[TRE_MAX_BUFLEN];
    const char *pat_raw[] = { "^#define", varname, "[\\w]{4,}" };
    const char *space_delims = "\\s+";
    const char *pat_value;
    
    if (type == HEX_FORMAT) {
        pat_value = "0x[0-9a-fA-F]{1,8}";
    } else if (type == DEC_FORMAT) {
        pat_value = "[0-9]{1,}";
    } else {
        return -2;
    }

    //create pattern to search
    pat_str[0] = '\0';
    str_join(pat_str, sizeof(pat_str), pat_raw, ARRAY_SIZE(pat_raw), space_delims);

    return _search_defined_array_from_file(file, _search_defined_value_from_buf, pat_str, pat_value, output, 1, 0x0);
}

/*
Get value content in source file, the format is: #define <varname> 0x123456
@file: file to search
@varname: defined varible array name
@output: varible value output array buffer
@outlen: varible value count
@invalid: if var in array not valid, filled to this value
@return how many varibles get, negative mean error occur
*/
int search_map_value_int_from_file(const char *file, const char *varname, unsigned int *output)
{
    char pat_str[TRE_MAX_BUFLEN];
    const char *pat_raw[] = { "", "0x[0-9a-fA-F]{4,}", varname };
    const char *pat_value = "0x[0-9a-fA-F]{4,}";
    const char *space_delims = "\\s+";

    //create pattern to search
    pat_str[0] = '\0';
    str_join(pat_str, sizeof(pat_str), pat_raw, ARRAY_SIZE(pat_raw), space_delims);

    return _search_defined_array_from_file(file, _search_defined_value_from_buf, pat_str, pat_value, output, 1, (unsigned int)-1);
}