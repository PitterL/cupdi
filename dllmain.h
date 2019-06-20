#ifndef __DLL_MAIN_H
#define __DLL_MAIN_H

#include "cupdi\os\include\platform.h"

#define EXPORTING_DLL
#ifdef EXPORTING_DLL
#define DLLENTRY __declspec(dllexport)
#else
#define DLLENTRY __declspec(dllimport)
#endif

DLLENTRY BOOL dev_open(const char *port, int baud, const char *devname);
DLLENTRY void dev_close(void);
DLLENTRY int dev_get_device_info(void);
DLLENTRY int dev_enter_progmode(void);
DLLENTRY int dev_leave_progmode(void);
DLLENTRY int dev_unlock_device(void);
DLLENTRY int dev_chip_erase(void);
DLLENTRY int dev_read_flash(u16 address, u8 *data, int len);
DLLENTRY int dev_write_flash(u16 address, const u8 *data, int len);
DLLENTRY int dev_write_fuse(int fusenum, u8 fuseval);
DLLENTRY int dev_read_mem(u16 address, u8 *data, int len);
DLLENTRY int dev_write_mem(u16 address, const u8 *data, int len);
DLLENTRY int dev_get_flash_info(void *info_ptr);
DLLENTRY int dev_set_verbose_level(verbose_t level);
#endif