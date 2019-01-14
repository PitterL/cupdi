#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "strlcat.h"

/*
Join the string array with delimiter, and store to dst buffer
@dst: buffer to store joined string
@dstlen: buffer len
@varlist: string list
@varnum: string list count
@delim: delimter for join
return dst pointer if success, NULL if failed
*/
char * str_join(char *dst, int dstlen, const char **varlist, int varnum, const char *delim)
{
    int i, left, delim_size;

    if (!dst || !dstlen || !varlist || !varnum)
        return NULL;

    dst[0] = '\0';
    delim_size = (int)strlen(delim);
    left = dstlen;

    for (i = 0; i < varnum && left > 1; i++) {
        if (!varlist[i])
            continue;

        strlcat(dst + (dstlen - left), varlist[i], left - 1);
        left -= (int)strlen(varlist[i]);

        if (left <= 1)  //last one should set to '\0'
            break;

        if (i < varnum - 1) {
            strlcat(dst + (dstlen - left), delim, left - 1);
            left -= delim_size;
        }
    }

    return dst;
}
