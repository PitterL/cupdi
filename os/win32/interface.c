#include <wtypes.h>

#include "../include/platform.h"
#include "../include/lib_intf.h"

BOOL libGetInterface(const char *libname, struct libIntfCallback *intfptr)
{
    HINSTANCE hinstDLL;

    if (!libname || !intfptr)
        return FALSE;

    hinstDLL = (HINSTANCE)LoadLibraryA(libname);
    if (hinstDLL != NULL)
    {
        intfptr->dev_open = (fn_open)GetProcAddress(hinstDLL, "dev_open");
        intfptr->dev_close = (fn_close)GetProcAddress(hinstDLL, "dev_close");
        intfptr->dev_get_device_info = (fn_get_device_info)GetProcAddress(hinstDLL, "dev_get_device_info");
        intfptr->dev_enter_progmode = (fn_enter_progmode)GetProcAddress(hinstDLL, "dev_enter_progmode");
        intfptr->dev_leave_progmode = (fn_leave_progmode)GetProcAddress(hinstDLL, "dev_leave_progmode");
        intfptr->dev_unlock_device = (fn_unlock_device)GetProcAddress(hinstDLL, "dev_unlock_device");
        intfptr->dev_chip_erase = (fn_chip_erase)GetProcAddress(hinstDLL, "dev_chip_erase");
        intfptr->dev_read_flash = (fn_read_flash)GetProcAddress(hinstDLL, "dev_read_flash");
        intfptr->dev_write_flash = (fn_write_flash)GetProcAddress(hinstDLL, "dev_write_flash");
        intfptr->dev_write_fuse = (fn_write_fuse)GetProcAddress(hinstDLL, "dev_write_fuse");
        intfptr->dev_read_mem = (fn_read_mem)GetProcAddress(hinstDLL, "dev_read_mem");
        intfptr->dev_write_mem = (fn_write_mem)GetProcAddress(hinstDLL, "dev_write_mem");
        intfptr->dev_get_flash_info = (fn_get_flash_info)GetProcAddress(hinstDLL, "dev_get_flash_info");
        intfptr->dev_set_verbose_level = (fn_set_verbose_level)GetProcAddress(hinstDLL, "dev_set_verbose_level");
        intfptr->hinst = hinstDLL;
        
        return TRUE;
    }

    return FALSE;
}

bool libReleaseInterface(struct libIntfCallback *intfptr)
{
    HINSTANCE hinstDLL;

    if (!intfptr)
        return FALSE;

    hinstDLL = intfptr->hinst;
    if (!hinstDLL)
        return FALSE;

    memset(intfptr, 0, sizeof(*intfptr));
    return FreeLibrary(hinstDLL);
}
