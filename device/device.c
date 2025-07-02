#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "os/platform.h"
#include "device.h"
/*
    Contains device specific information needed for programming
*/

/* fuse field */
enum {
    FS_WDTCFG = 0x0,
    FS_BODCFG = 0x01,
    FS_OSCCFG = 0x02,
    FS_TINY_TCD0CFG = 0x04,
    FS_SYSCFG0 = 0x05,
    FS_SYSCFG1 = 0x06,
    FS_CODESIZE = 0x07,
    FS_BOOTSIZE = 0x08,
    FS_AVRDU_PDICFG = 0x0A
};

/*
    NVM info
    {nvm_start | nvm_size | nvm_pagesize | nvm_blocksize | nvm_mapped_start | nvm_magoff}
*/

const chip_mem_t device_avr64du = {
    //  avr64du28/32
    .dev_name = "avr64dux",
    .nvms = {
        {0, 64 * 1024, 512, 32 * 1024, 0x8000},
        {0x1050, 11, 1, 0, 0, MAGIC_ID_FUSE},
        {0x1200, 512, 512, 0, 0, MAGIC_ID_USER},
        {0x1400, 256, 32, 0, 0, MAGIC_ID_EEPROM},
        {0x6000, 8 * 1024, 1024 /*dummy*/, 0, 0, 0xFF /*dummy*/},
        {0x1040, 4, 1, 0, 0, MAGIC_ID_LOCKBITS},
        {0x1100, 256, 256, 0, 0, MAGIC_ID_BOOTROW},
    },
    .reg = {0x0F00, 0x1000, 0x1080},
    .crc = {512, FS_CODESIZE, FS_BOOTSIZE}
};

const chip_mem_t device_avr32du = {
    //  avr32du14/20/28/32
    .dev_name = "avr32dux",
    .nvms = {
        {0, 32 * 1024, 512, 32 * 1024, 0x8000},
        {0x1050, 11, 1, 0, 0, MAGIC_ID_FUSE},
        {0x1200, 512, 512, 0, 0, MAGIC_ID_USER},
        {0x1400, 256, 32, 0, 0, MAGIC_ID_EEPROM},
        {0x7000, 4 * 1024, 1024 /*dummy*/, 0, 0, 0xFF /*dummy*/},
        {0x1040, 4, 1, 0, 0, MAGIC_ID_LOCKBITS},
        {0x1100, 256, 256, 0, 0, MAGIC_ID_BOOTROW},
    },
    .reg = {0x0F00, 0x1000, 0x1080},
    .crc = {512, FS_CODESIZE, FS_BOOTSIZE}
};

const chip_mem_t device_avr16du = {
    //  avr16du14/20/28/32
    .dev_name = "avr16dux",
    .nvms = {
        {0, 16 * 1024, 512, 16 * 1024, 0x8000},
        {0x1050, 11, 1, 0, 0, MAGIC_ID_FUSE},
        {0x1200, 512, 512, 0, 0, MAGIC_ID_USER},
        {0x1400, 256, 32, 0, 0, MAGIC_ID_EEPROM},
        {0x7800, 4 * 1024, 1024 /*dummy*/, 0, 0, 0xFF /*dummy*/},
        {0x1040, 4, 1, 0, 0, MAGIC_ID_LOCKBITS},
        {0x1100, 256, 256, 0, 0, MAGIC_ID_BOOTROW},
    },
    .reg = {0x0F00, 0x1000, 0x1080},
    .crc = {512, FS_CODESIZE, FS_BOOTSIZE}
};
    
const chip_mem_t device_avr128da = {
    //  avr128da28/32/48/64
    .dev_name = "avr128dax",
    .nvms = {
        {0, 128 * 1024, 512, 32 * 1024, 0x8000},
        {0x1050, 9, 1, 0, 0, MAGIC_ID_FUSE},
        {0x1080, 32, 32, 0, 0, MAGIC_ID_USER},
        {0x1400, 512, 32, 0, 0, MAGIC_ID_EEPROM},
        {0x4000, 16 * 1024, 1024 /*dummy*/, 0, 0, 0xFF /*dummy*/},
        {0x1040, 4, 1, 0, 0, MAGIC_ID_LOCKBITS},
    },
    .reg = {0x0F00, 0x1000, 0x1100},
    .crc = {512, FS_CODESIZE, FS_BOOTSIZE}
};

const chip_mem_t device_avr64da = {
    //  avr64da28/32/48/64
    .dev_name = "avr64dax",
    .nvms = {
        {0, 64 * 1024, 512, 32 * 1024, 0x8000},
        {0x1050, 9, 1, 0, 0, MAGIC_ID_FUSE},
        {0x1080, 32, 32, 0, 0, MAGIC_ID_USER},
        {0x1400, 512, 32, 0, 0, MAGIC_ID_EEPROM},
        {0x4000, 8 * 1024, 1024 /*dummy*/, 0, 0, 0xFF /*dummy*/},
        {0x1040, 4, 1, 0, 0, MAGIC_ID_LOCKBITS},
    },
    .reg = {0x0F00, 0x1000, 0x1100},
    .crc = {512, FS_CODESIZE, FS_BOOTSIZE}
};

const chip_mem_t device_avr32da = {
    //  avr32da28/32/48
    .dev_name = "avr32dax",
    .nvms = {
        {0, 32 * 1024, 512, 32 * 1024, 0x8000},
        {0x1050, 9, 1, 0, 0, MAGIC_ID_FUSE},
        {0x1080, 32, 32, 0, 0, MAGIC_ID_USER},
        {0x1400, 512, 32, 0, 0, MAGIC_ID_EEPROM},
        {0x4000, 4 * 1024, 1024 /*dummy*/, 0, 0, 0xFF /*dummy*/},
        {0x1040, 4, 1, 0, 0, MAGIC_ID_LOCKBITS},
    },
    .reg = {0x0F00, 0x1000, 0x1100},
    .crc = {512, FS_CODESIZE, FS_BOOTSIZE}
};

const chip_mem_t device_tiny_321x = {
    //  tiny3217/tiny3216
    .dev_name = "tiny321x",
    .nvms = {
        {0, 32 * 1024, 128, 32 * 1024, 0x8000},
        {0x1280, 11, 1, 0, 0, MAGIC_ID_FUSE},
        {0x1300, 64, 64, 0, 0, MAGIC_ID_USER},
        {0x1400, 256, 64, 0, 0, MAGIC_ID_EEPROM},
        {0x3800, 2 * 1024, 1024 /*dummy*/, 0, 0, 0xFF /*dummy*/},
        {0x128A, 1, 1, 0, 0, MAGIC_ID_LOCKBITS},
    },
    .reg = {0x0F00, 0x1000, 0x1100},
    .crc = {256, FS_CODESIZE, FS_BOOTSIZE},
    .updipin = { FS_SYSCFG0, 2, 3 }
};

const chip_mem_t device_tiny_161x = {
    //  tiny1617/tiny1616
    .dev_name = "tiny161x",
    .nvms = {
        {0, 16 * 1024, 64, 16 * 1024, 0x8000},
        {0x1280, 11, 1, 0, 0, MAGIC_ID_FUSE},
        {0x1300, 32, 32, 0, 0, MAGIC_ID_USER},
        {0x1400, 256, 32, 0, 0, MAGIC_ID_EEPROM},
        {0x3800, 2 * 1024, 1024 /*dummy*/, 0, 0, 0xFF /*dummy*/},
        {0x128A, 1, 1, 0, 0, MAGIC_ID_LOCKBITS},
     },
    .reg = {0x0F00, 0x1000, 0x1100},
    .crc = {256, FS_CODESIZE, FS_BOOTSIZE},
    .updipin = { FS_SYSCFG0, 2, 3 }
};

const chip_mem_t device_tiny_81x = {
    //  tiny817/tiny816/tiny814
    .dev_name = "tiny81x",
    .nvms = {
        {0, 8 * 1024, 64, 8 * 1024, 0x8000},
        {0x1280, 11, 1, 0, 0, MAGIC_ID_FUSE},
        {0x1300, 32, 32, 0, 0, MAGIC_ID_USER},
        {0x1400, 128, 32, 0, 0, MAGIC_ID_EEPROM},
        {0x3E00, 512, 512 /*dummy*/, 0, 0, 0xFF /*dummy*/},
        {0x128A, 1, 1, 0, 0, MAGIC_ID_LOCKBITS},
    },
    .reg = {0x0F00, 0x1000, 0x1100},
    .crc = {256, FS_CODESIZE, FS_BOOTSIZE},
    .updipin = { FS_SYSCFG0, 2, 3 }
};

const chip_mem_t device_tiny_41x = {
    //  tiny417
    .dev_name = "tiny41x",
    .nvms = {
        {0, 4 * 1024, 64, 4 * 1024, 0x8000},
        {0x1280, 11, 1, 0, 0, MAGIC_ID_FUSE},
        {0x1300, 32, 32, 0, 0, MAGIC_ID_USER},
        {0x1400, 128, 32, 0, 0, MAGIC_ID_EEPROM},
        {0x3F00, 256, 256 /*dummy*/, 0, 0, 0xFF /*dummy*/},
        {0x128A, 1, 1, 0, 0, MAGIC_ID_LOCKBITS},
    },
    .reg = {0x0F00, 0x1000, 0x1100},
    .crc = {256, FS_CODESIZE, FS_BOOTSIZE},
    .updipin = { FS_SYSCFG0, 2, 3 }
};

static const device_info_t g_device_list[] = {
    {"avr64du", AVRDU, &device_avr64du},
    {"avr32du", AVRDU, &device_avr32du},
    {"avr16du", AVRDU, &device_avr16du},
    {"avr128da", AVRDA, &device_avr128da},
    {"avr64da", AVRDA, &device_avr64da},
    {"avr32da", AVRDA, &device_avr32da},
    {"tiny3216", TINY321x, &device_tiny_321x},
    {"tiny3217", TINY321x, &device_tiny_321x},
    {"tiny1616", TINY161x, &device_tiny_161x},
    {"tiny1617", TINY161x, &device_tiny_161x},
    {"tiny814", TINY81x, &device_tiny_81x},
    {"tiny816", TINY81x, &device_tiny_81x},
    {"tiny817", TINY81x, &device_tiny_81x},
    {"tiny417", TINY41x, &device_tiny_41x},
};

const char *chip_nvm_name[NUM_NVM_EX_TYPES] = {
    "FLASH",
    "FUSES",
    "USERROW",
    "EEPROM",
    "SRAM",
    "LOCKBITS",
    "BOOTROW"
};

const device_info_t *get_chip_info(const char *dev_name)
{
    const device_info_t *dev;
    int i;

    DBG_INFO(OTHER_DEBUG, "get_chip_info search `%s`", dev_name);

    for (i = 0; i < ARRAY_SIZE(g_device_list); i++)
    {
        dev = &g_device_list[i];
        DBG_INFO(OTHER_DEBUG, "get_chip_info current dev `%s`", dev->name);
        if (!strcmp(dev_name, dev->name))
        {
            return dev;
        }
    }

    return NULL;
}

/*
Device get block info, this is defined in device.c
    @dev_ptr: device info structure pointer, get by get_chip_info()
    @type: NVM type
    @info: chip flash information
    @return 0 successful, other value failed
*/
int dev_get_nvm_info(const void *dev_ptr, NVM_TYPE_EX_T type, nvm_info_t *inf)
{
    /*
    get NVM information
    */
    device_info_t* dev = (device_info_t *)dev_ptr;
    const chip_mem_t *mmap = dev->mmap;

    if (type >= NUM_NVM_EX_TYPES) {
        return -1;
    }

    memcpy(inf, &mmap->nvms[type], sizeof(*inf));
    
    return 0;
}

/*
Device get block info, this is defined in device.c
    @dev_ptr: device info structure pointer, get by get_chip_info()
    @type: NVM type
    @info: chip flash information
    @pname: output the nvm name string
    @return 0 successful, other value failed
*/
int dev_get_nvm_info_ext(const void *dev_ptr, NVM_TYPE_EX_T type, nvm_info_t *info, const char **pname)
{
    int result = dev_get_nvm_info(dev_ptr, type, info);

    if (result == 0)
    {
        if (pname)
        {
            *pname = chip_nvm_name[type];
        }
    }

    return result;
}

/*
Device get crcsrc info, this is defined in device.c
    @dev_ptr: device info structure pointer, get by get_chip_info()
    @src: crcsrc information
    @return 0 successful, other value failed
*/
int dev_get_crc_info(const void *dev_ptr, crc_src_t *src)
{
    const device_info_t *dev = (const device_info_t *)dev_ptr;

    if (src) {
        memcpy(src, &dev->mmap->crc, sizeof(*src));
    }

    return 0;
}

/*
Device get updi pin config info, this is defined in device.c
    @dev_ptr: device info structure pointer, get by get_chip_info()
    @src: updi_pin_cfg_t information
    @return 0 successful, other value failed
*/
int dev_get_updi_pincfg_info(const void *dev_ptr, updi_pincfg_t *src)
{
    const device_info_t *dev = (const device_info_t *)dev_ptr;

    if (src) {
        memcpy(src, &dev->mmap->updipin, sizeof(*src));
    }

    return 0;
}