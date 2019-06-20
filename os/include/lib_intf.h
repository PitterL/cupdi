#include "platform.h"

/*
const char *lib_proc_name[] = {
    "dev_open",
    "dev_close",
    "dev_get_device_info",
    "dev_enter_progmode",
    "dev_leave_progmode",
    "dev_unlock_device",
    "dev_chip_erase",
    "dev_read_flash",
    "dev_write_flash",
    "dev_write_fuse",
    "dev_read_mem",
    "dev_write_mem",
    "dev_get_flash_info",
};
*/

typedef int (*fn_open)(const char *port, int baud, const char *devname);
typedef void (*fn_close)(void);
typedef int (*fn_get_device_info)(void);
typedef int (*fn_enter_progmode)(void);
typedef int (*fn_leave_progmode)(void);
typedef int (*fn_unlock_device)(void);
typedef int (*fn_chip_erase)(void);
typedef int (*fn_read_flash)(u16 address, u8 *data, int len);
typedef int (*fn_write_flash)(u16 address, const u8 *data, int len);
typedef int (*fn_write_fuse)(int fusenum, u8 fuseval);
typedef int (*fn_read_mem)(u16 address, u8 *data, int len);
typedef int (*fn_write_mem)(u16 address, const u8 *data, int len);
typedef int (*fn_get_flash_info)(void *info_ptr);
typedef verbose_t (*fn_set_verbose_level)(verbose_t level);

struct libIntfCallback {
    fn_open dev_open;
    fn_close dev_close;
    fn_get_device_info dev_get_device_info;
    fn_enter_progmode dev_enter_progmode;
    fn_leave_progmode dev_leave_progmode;
    fn_unlock_device dev_unlock_device;
    fn_chip_erase dev_chip_erase;
    fn_read_flash dev_read_flash;
    fn_write_flash dev_write_flash;
    fn_write_fuse dev_write_fuse;
    fn_read_mem dev_read_mem;
    fn_write_mem dev_write_mem;
    fn_get_flash_info dev_get_flash_info;
    fn_set_verbose_level dev_set_verbose_level;

    HINSTANCE hinst;
};

#define CUPDILIB_NAME "cupdilib.dll"

bool libGetInterface(const char *libname, struct libIntfCallback *intfptr);
bool libReleaseInterface(struct libIntfCallback *intfptr);