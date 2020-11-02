#ifndef __UP_LOGGING_H
#define __UP_LOGGING_H

#ifdef CUPDI

#if 0
#define DEFAULT_ROWDATA_SIZE 16

void _loginfo(char *format, const unsigned char *data, int len, const unsigned char *dformat, ...);
void _loginfo_i(char* format, ...);

typedef enum {
    DEFAULT_DEBUG,
    UPDI_DEBUG,
    NVM_DEBUG, 
    APP_DEBUG, 
    LINK_DEBUG, 
    PHY_DEBUG, 
    SER_DEBUG
} verbose_t;

void set_verbose_level(verbose_t level);
void DBG(verbose_t level, char *format, const unsigned char *data, int len, const unsigned char * dformat, ...);
void DBG_INFO(verbose_t level, char* format, ...);
#else
#define DBG(level, format, data, len, dformat...) 
#define DBG_INFO(level, format...) 
#endif
#endif

#endif