/* getline.c
 *
 * getdelim(), getline() - read a delimited record from stream, ersatz implementation
 *
 * For more details, see: http://pubs.opengroup.org/onlinepubs/9699919799/functions/getline.html
 *
 */

#include "getline.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <os\platform.h>

int getdelim(char **lineptr, unsigned int *n, int delim, FILE *stream) {
    char c, *cur_pos, *new_lineptr;
    unsigned int new_lineptr_len;

    if (lineptr == NULL || n == NULL || stream == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (*lineptr == NULL) {
        *n = 128; /* init len */
        if ((*lineptr = (char *)malloc(*n)) == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }

    cur_pos = *lineptr;
    for (;;) {
        c = getc(stream);

        if (ferror(stream) || (c == EOF && cur_pos == *lineptr))
            return -1;

        if (c == EOF)
            break;

        if ((*lineptr + *n - cur_pos) < 2) {
            if (/*SSIZE_MAX*/INT_MAX / 2 < *n) {
#ifdef EOVERFLOW
                errno = EOVERFLOW;
#else
                errno = ERANGE; /* no EOVERFLOW defined */
#endif
                return -1;
            }
            new_lineptr_len = *n * 2;

            if ((new_lineptr = (char *)realloc(*lineptr, new_lineptr_len)) == NULL) {
                errno = ENOMEM;
                return -1;
            }
            cur_pos = new_lineptr + (cur_pos - *lineptr);
            *lineptr = new_lineptr;
            *n = new_lineptr_len;
        }

        *cur_pos++ = c;

        if (c == delim)
            break;
    }

    *cur_pos = '\0';
    return (int)(cur_pos - *lineptr);
}

int getline(char **lineptr, unsigned int *n, FILE *stream) {
    return getdelim(lineptr, n, '\n', stream);
}

