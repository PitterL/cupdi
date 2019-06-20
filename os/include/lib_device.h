#ifndef __UDPI_DEVICE
#define __UDPI_DEVICE

typedef struct _flash_info{
    unsigned short flash_start;
    unsigned short flash_size;
    unsigned short flash_pagesize;
}flash_info_t;

typedef struct _reg_info {
    unsigned short syscfg_address;
    unsigned short nvmctrl_address;
    unsigned short sigrow_address;
    unsigned short fuses_address;
    unsigned short userrow_address;
}reg_info_t;

typedef struct _chip_info {
    const char *dev_name;
    flash_info_t flash;
    reg_info_t reg;
}chip_info_t;

typedef struct _device_info {
    const char *name;
    const chip_info_t *mmap;
}device_info_t;

const device_info_t * get_chip_info(const char *dev_name);

#endif
