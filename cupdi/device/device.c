#ifdef CUPDI

#include <stdlib.h>  
#include <stdio.h>  
#include <stdarg.h>
#include <string.h>
#include "platform/platform.h"
#include "device.h"
/*
    Contains device specific information needed for programming
*/


/* dev_name | {flash_start | flash_size | flash_pagesize} | {syscfg_address | nvmctrl_address | sigrow_address } | {fuses} | {userrow} | {eeprom} */
const chip_info_t device_tiny_161x = {
    //  tiny1617/tiny1616
    "tiny161x",{ 0x8000, 16 * 1024, 64 },{ 0x0F00, 0x1000, 0x1100 },{ 0x1280, 11, 1 },{ 0x1300, 32, 32 },{ 0x1400, 128, 32 }
};

static const device_info_t device_1617 = {
	.name = "tiny1617", 
	.mmap = &device_tiny_161x,
};

inline const device_info_t * get_chip_info(const char *dev_name) 
{
    return (&device_1617);
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
    default:
        return -2;
    }

    memcpy(info, iblock, sizeof(*info));

    return 0;
}
#endif