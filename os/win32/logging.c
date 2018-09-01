#include <stdlib.h>  
#include <stdio.h>  
#include <stdarg.h>

#include "logging.h"

void _loginfo(char *format, const unsigned char *data, int len, const unsigned char * dformat, ...)
{
    va_list args;
    int     size;
    char    *buffer;

    // retrieve the variable arguments  
    va_start(args, dformat);

    size = _vscprintf(format, args) // _vscprintf doesn't count  
        + 1; // terminating '\0'  

    buffer = (char*)malloc(size * sizeof(char));

    vsnprintf(buffer, size, format, args); // C4996  
                                          // Note: vsprintf is deprecated; consider using vsprintf_s instead  
    puts(buffer);

    free(buffer);

    va_end(args);

    if (data && len) {
        for (int i = 0; i < len; i++) {
            printf(dformat, data[i]);
        }
    }

    printf("\r\n");
}

void _loginfo_i(char* format, ...)
{
    va_list args;
    int     len;
    char    *buffer;

    // retrieve the variable arguments  
    va_start(args, format);

    len = _vscprintf(format, args) // _vscprintf doesn't count  
        + 1; // terminating '\0'  

    buffer = (char*)malloc(len * sizeof(char));

    vsnprintf(buffer, len, format, args); // C4996  
                                          // Note: vsprintf is deprecated; consider using vsprintf_s instead  

    puts(buffer);
    
    free(buffer);

    va_end(args);
}
