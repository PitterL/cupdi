#ifndef __UP_LOGGING_H
#define __UP_LOGGING_H

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

typedef enum {
    DEFAULT_ERROR = 0,
    UPDI_ERROR = 1,
    NVM_ERROR = 1,
    APP_ERROR = 1,
    LINK_ERROR = 1,
    PHY_ERROR = 1,
    SER_ERROR = 1
} verbose_t;

void set_verbose_level(verbose_t level);
void DBG(verbose_t level, char *format, const unsigned char *data, int len, const unsigned char * dformat, ...);
void DBG_INFO(verbose_t level, char* format, ...);


#endif