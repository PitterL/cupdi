#ifdef CUPDI

#include <stdlib.h>  
#include <stdio.h>  
#include <stdarg.h>

#include "logging.h"
#if 0
verbose_t g_verbose_level = DEFAULT_DEBUG;

int _vscprintf (const char * format, va_list pargs)
{ 
    int retval; 
    va_list argcopy;
    va_copy(argcopy, pargs); 
    retval = vsnprintf(NULL, 0, format, argcopy); 
    va_end(argcopy); 
    return retval;
}

void set_verbose_level(verbose_t level)
{
    g_verbose_level = level;
}

void _logv(verbose_t level, char *format, const unsigned char *data, int len, const unsigned char * dformat, int rowsize, va_list args)
{
    int     size;
    char    *buffer;

    if (level > g_verbose_level)
        return;

    if (rowsize <= 0)
        rowsize = DEFAULT_ROWDATA_SIZE;

    if (format && format[0]) {
        size = _vscprintf(format, args) // _vscprintf doesn't count  
            + 1; // terminating '\0'  

        buffer = (char*)malloc(size * sizeof(char));

        vsnprintf(buffer, size, format, args); // C4996  
                                               // Note: vsprintf is deprecated; consider using vsprintf_s instead  
        puts(buffer);

        free(buffer);
    }

    if (data && len) {
        for (int i = 0; i < len; i++) {
            if (len > rowsize) {
                if (!(i % rowsize)) {
                    if (i)
                        puts("");
                    printf("%04x:\t", i);
                }
            }

            printf(dformat, data[i]);
        }
        puts("");
    }
}

void _loginfo(char *format, const unsigned char *data, int len, const unsigned char * dformat, ...)
{
    va_list args;

    va_start(args, dformat);
    _logv(DEFAULT_DEBUG, format, data, len, dformat, DEFAULT_ROWDATA_SIZE, args);
    va_end(args);
}

void _loginfo_i(char* format, ...)
{
    va_list args;

    va_start(args, format);
    _logv(DEFAULT_DEBUG, format, NULL, 0, NULL, 0, args);
    va_end(args);
}

void DBG(verbose_t level, char *format, const unsigned char *data, int len, const unsigned char * dformat, ...)
{
    va_list args;

    va_start(args, dformat);
    _logv(level, format, data, len, dformat, DEFAULT_ROWDATA_SIZE, args);
    va_end(args);
}

void DBG_INFO(verbose_t level, char* format, ...)
{
    va_list args;

    va_start(args, format);
    _logv(level, format, NULL, 0, NULL, DEFAULT_ROWDATA_SIZE, args);
    va_end(args);
}
#endif

#endif