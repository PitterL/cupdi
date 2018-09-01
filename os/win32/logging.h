#ifndef __UP_LOGGING_H
#define __UP_LOGGING_H

void _loginfo(char *format, const unsigned char *data, int len, const unsigned char *dformat, ...);
void _loginfo_i(char* format, ...);

#endif