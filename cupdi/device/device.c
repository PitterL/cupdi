#include <stdlib.h>  
#include <stdio.h>  
#include <stdarg.h>
#include <string.h>
#include "os/include/platform.h"
#include "device.h"
/*
    Contains device specific information needed for programming
*/


/* dev_name | {flash_start | flash_size | flash_pagesize} | {syscfg_address | nvmctrl_address | sigrow_address | fuses_address | userrow_address} */
const chip_info_t device_tiny_321x = {
    //  tiny1617/tiny1616
    "tiny321x",{ 0x8000, 32 * 1024, 64 },{ 0x0F00, 0x1000, 0x1100, 0x1280, 0x1300 }
};

const chip_info_t device_tiny_161x = {
    //  tiny1617/tiny1616
    "tiny161x",{ 0x8000, 16 * 1024, 64 },{ 0x0F00, 0x1000, 0x1100, 0x1280, 0x1300 }
};

const chip_info_t device_tiny_81x = {
    //  tiny817/tiny816/tiny814
    "tiny81x", {0x8000, 8 * 1024, 64}, {0x0F00, 0x1000, 0x1100, 0x1280, 0x1300}
};

const chip_info_t device_tiny_41x = {
    //  tiny417
    "tiny41x", {0x8000, 4 * 1024, 64}, {0x0F00, 0x1000, 0x1100, 0x1280, 0x1300}
};

static const device_info_t g_device_list[] = {
    { "tiny3216", &device_tiny_321x },
    { "tiny3217", &device_tiny_321x },
    { "tiny1616", &device_tiny_161x },
    { "tiny1617", &device_tiny_161x },
    { "tiny814", &device_tiny_81x },
    { "tiny816", &device_tiny_81x },
    { "tiny817", &device_tiny_81x },
    { "tiny417", &device_tiny_41x },
};

const device_info_t * get_chip_info(const char *dev_name) 
{
    const device_info_t *dev;
    int i;

    for (i = 0; i < ARRAY_SIZE(g_device_list); i++) {
        dev = &g_device_list[i];
        if (!strcmp(dev_name, dev->name)) {
            return dev;
        }
    }

    return NULL;
}
