#include <stdlib.h>  
#include <stdio.h>  
#include <stdarg.h>
#include <string.h>
#include "os/platform.h"
#include "device.h"
/*
    Contains device specific information needed for programming
*/


/* dev_name | {flash_start | flash_size | flash_pagesize} | {syscfg_address | nvmctrl_address | sigrow_address } | {fuses} | {userrow} | {eeprom} {sram} */
const chip_info_t device_avr128da = {
	//  avr128da28/32/48/64
	"avr128dax",{ 0x8000, 128 * 1024, 512 },{ 0x0F00, 0x1000, 0x1100 },{ 0x1050, 9, 1 },{ 0x1080, 32, 32 },{ 0x1400, 512, 1 }, {0x4000, 16 * 1024, 1024/*dummy*/}
};

const chip_info_t device_avr64da = {
	//  avr64da28/32/48/64
	"avr64dax",{ 0x8000, 64 * 1024, 512 },{ 0x0F00, 0x1000, 0x1100 },{ 0x1050, 9, 1 },{ 0x1080, 32, 32 },{ 0x1400, 512, 1 }, {0x4000, 8 * 1024, 1024/*dummy*/}
};

const chip_info_t device_avr32da = {
	//  avr32da28/32/48
	"avr32dax",{ 0x8000, 32 * 1024, 512 },{ 0x0F00, 0x1000, 0x1100 },{ 0x1050, 9, 1 },{ 0x1080, 32, 32 },{ 0x1400, 512, 1 }, {0x4000, 4 * 1024, 1024/*dummy*/}
};

const chip_info_t device_tiny_321x = {
    //  tiny3217/tiny3216
    "tiny321x",{ 0x8000, 32 * 1024, 128 },{ 0x0F00, 0x1000, 0x1100 },{ 0x1280, 11, 1 },{ 0x1300, 64, 64 },{ 0x1400, 256, 64}, {0x3800, 2 * 1024, 1024/*dummy*/}
};

const chip_info_t device_tiny_161x = {
    //  tiny1617/tiny1616
    "tiny161x",{ 0x8000, 16 * 1024, 64 },{ 0x0F00, 0x1000, 0x1100 },{ 0x1280, 11, 1 },{ 0x1300, 32, 32 },{ 0x1400, 256, 32 }, { 0x3800, 2 * 1024, 1024/*dummy*/ }
};

const chip_info_t device_tiny_81x = {
    //  tiny817/tiny816/tiny814
    "tiny81x", {0x8000, 8 * 1024, 64}, {0x0F00, 0x1000, 0x1100 },{ 0x1280, 11, 1 },{ 0x1300, 32, 32 },{ 0x1400, 128, 32 }, { 0x3E00, 512, 512/*dummy*/ }
};

const chip_info_t device_tiny_41x = {
    //  tiny417
    "tiny41x", {0x8000, 4 * 1024, 64}, {0x0F00, 0x1000, 0x1100 },{ 0x1280, 11, 1 },{ 0x1300, 32, 32 },{ 0x1400, 128, 32 },{ 0x3F00, 256, 256/*dummy*/ }
};

static const device_info_t g_device_list[] = {
	{ "avr128da", &device_avr128da },
	{ "avr64da", &device_avr64da },
	{ "avr32da", &device_avr32da },
    { "tiny3216", &device_tiny_321x },
    { "tiny3217", &device_tiny_321x },
    { "tiny1616", &device_tiny_161x },
    { "tiny1617", &device_tiny_161x },
    { "tiny814", &device_tiny_81x },
    { "tiny816", &device_tiny_81x },
    { "tiny817", &device_tiny_81x },
    { "tiny417", &device_tiny_41x },
};

const char * chip_nvm_name[NUM_NVM_TYPES] =
{
    "Flash",
    "EEPRom",
    "User",
    "Fuse",
    "Ram"
};

const device_info_t * get_chip_info(const char *dev_name) 
{
    const device_info_t *dev;
    int i;

	DBG_INFO(OTHER_DEBUG, "get_chip_info search `%s`", dev_name);

    for (i = 0; i < ARRAY_SIZE(g_device_list); i++) {
        dev = &g_device_list[i];
		DBG_INFO(OTHER_DEBUG, "get_chip_info current dev `%s`", dev->name);
        if (!strcmp(dev_name, dev->name)) {
            return dev;
        }
    }

    return NULL;
}

/*
Device get block info, this is defined in device.c
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @type: NVM type
    @info: chip flash information
    @return 0 successful, other value failed
*/
int dev_get_nvm_info(const void *dev_ptr, NVM_TYPE_T type, nvm_info_t * info)
{
    /*
    get NVM information
    */
    const device_info_t *dev = (const device_info_t *)dev_ptr;
    const nvm_info_t *iblock;

    switch (type) {
    case NVM_FLASH:
        iblock = &dev->mmap->flash;
        break;
    case NVM_EEPROM:
        iblock = &dev->mmap->eeprom;
        break;
    case NVM_USERROW:
        iblock = &dev->mmap->userrow;
        break;
    case NVM_FUSES:
        iblock = &dev->mmap->fuse;
        break;
    case MEM_SRAM:
        iblock = &dev->mmap->sram;
        break;
    default:
        return -2;
    }

    memcpy(info, iblock, sizeof(*info));

    return 0;
}

/*
Device get block info, this is defined in device.c
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @type: NVM type
    @info: chip flash information
    @pname: output the nvm name string
    @return 0 successful, other value failed
*/
int dev_get_nvm_info_ext(const void *dev_ptr, NVM_TYPE_T type, nvm_info_t * info, const char **pname)
{
    int result = dev_get_nvm_info(dev_ptr, type, info);

    if (result == 0) {
        if (pname) {
            *pname = chip_nvm_name[type];   
        }
    }

    return result;
}