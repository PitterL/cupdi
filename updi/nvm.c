#include "os/platform.h"
#include "device/device.h"
#include "application.h"
#include "nvm.h"
#include "constants.h"

typedef struct _upd_nvm {
#define UPD_NVM_MAGIC_WORD 'unvm'
    unsigned int mgwd;  //magic word
    bool progmode;
    void *app;
    device_info_t *dev;
}upd_nvm_t;

#define VALID_NVM(_nvm) ((_nvm) && ((_nvm)->mgwd == UPD_NVM_MAGIC_WORD))
#define APP(_nvm) ((_nvm)->app)
#define NVM_REG(_nvm, _name) ((_nvm)->dev->mmap->reg._name)
#define NVM_FLASH(_nvm, _name) ((_nvm)->dev->mmap->flash._name)

void *updi_nvm_init(const char *port, int baud, void *dev)
{
    upd_nvm_t *nvm = NULL;
    void *app;

    DBG_INFO(NVM_DEBUG, "<NVM> init nvm");

    app = updi_application_init(port, baud, dev);
    if (app) {
        nvm = (upd_nvm_t *)malloc(sizeof(*nvm));
        nvm->mgwd = UPD_NVM_MAGIC_WORD;
        nvm->progmode = false;
        nvm->dev = (device_info_t *)dev;
        nvm->app = (void *)app;
    }

    return nvm;
}

void updi_nvm_deinit(void *nvm_ptr)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    if (VALID_NVM(nvm)) {
        DBG_INFO(NVM_DEBUG, "<NVM> deinit nvm");

        updi_application_deinit(APP(nvm));
        free(nvm);
    }
}

int nvm_get_device_info(void *nvm_ptr)
{
    /*
        Reads device info
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    
    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Reading device info");

    return app_device_info(APP(nvm));
}

int nvm_enter_progmode(void *nvm_ptr)
{
    /*
    Enter programming mode
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Entering NVM programming mode");
    
    result = app_enter_progmode(APP(nvm));
    if (result) {
        DBG_INFO(NVM_DEBUG, "app_enter_progmode failed %d", result);
        return -2;
    }

    nvm->progmode = true;

    return 0;
}

int nvm_leave_progmode(void *nvm_ptr)
{
    /*
        Leave programming mode
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    if (!nvm->progmode)
        return 0;

    DBG_INFO(NVM_DEBUG, "<NVM> Leaving NVM programming mode");

    result = app_leave_progmode(APP(nvm));
    if (result) {
        DBG_INFO(NVM_DEBUG, "app_leave_progmode failed %d", result);
        return -2;
    }

    nvm->progmode = false;

    return 0;
}

int nvm_unlock_device(void *nvm_ptr)
{
    /*
    Unlock and erase a device
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Unlock and erase a device");

    if (nvm->progmode) {
        DBG_INFO(NVM_DEBUG, "Device already unlocked");
        return 0;
    }

    // Unlock
    result = app_unlock(APP(nvm));
    if (!result) {
        DBG_INFO(NVM_DEBUG, "app_unlock failed %d", result);
        return -2;
    }

    // Unlock after using the NVM key results in prog mode.
    nvm->progmode = true;

    return 0;
}

int nvm_chip_erase(void *nvm_ptr)
{
    /*
    Erase (unlocked) device
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Erase device");

    if (!nvm->progmode) {
        DBG_INFO(NVM_DEBUG, "Enter progmode first!");
        return -2;
    }

    result = app_chip_erase(APP(nvm));
    if (result) {
        DBG_INFO(NVM_DEBUG, "app_chip_erase failed %d", result);
        return -3;
    }

    return 0;
}

int nvm_read_flash(void *nvm_ptr, u16 address, u8 *data, int len)
{
    /*
    Reads from flash
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int i, off, pages, page_size;
    int result = -4;

    if (!VALID_NVM(nvm) || !data)
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Read from flash");

    if (!nvm->progmode) {
        DBG_INFO(NVM_DEBUG, "Flash is locked");
    }

    page_size = NVM_FLASH(nvm, flash_pagesize);
    if (len & (page_size - 1)) {
        DBG_INFO(NVM_DEBUG, "Only full page aligned flash supported, len %x.", len);
        return -3;
    }
    
    pages = len / page_size;
    for (i = 0, off = 0; i < pages; i++) {
        DBG_INFO(NVM_DEBUG, "Reading Page(%d/%d) at 0x%x", i, pages, address + off);
        
        result = app_read_nvm(APP(nvm), address + off, data + off, page_size);
        if (result) {
            DBG_INFO(NVM_DEBUG, "app_read_nvm failed %d", result);
            break;
        }
        
        off += page_size;
    }

    if (i < pages || result) {
        DBG_INFO(NVM_DEBUG, "Read flash failed %d", i, result);
        return -4;
    }

    return 0;
}

int nvm_write_flash(void *nvm_ptr, u16 address, u8 *data, int len)
{
    /*
    Writes to flash
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int i, off, pages, page_size;
    int result = -5;

    if (!VALID_NVM(nvm) || !data)
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Writes to flash");

    if (!nvm->progmode) {
        DBG_INFO(NVM_DEBUG, "Enter progmode first!");
        return -2;
    }

    page_size = NVM_FLASH(nvm,flash_pagesize);
    if (len & (page_size - 1)) {
        DBG_INFO(NVM_DEBUG, "Only full page aligned flash supported, len %x.", len);
        return -3;
    }

    pages = len / page_size;
    for (i = 0, off = 0; i < pages; i++) {
        DBG_INFO(NVM_DEBUG, "Writing Page(%d/%d) at 0x%x", i, pages, address + off);

        result = app_write_nvm(APP(nvm), address + off, data + off, page_size);
        if (result) {
            DBG_INFO(NVM_DEBUG, "app_write_nvm failed %d", result);
            break;
        }

        off += page_size;
    }

    if (i < pages || result) {
        DBG_INFO(NVM_DEBUG, "Write flash page %d failed %d", i, result);
        return -4;
    }

    return 0;
}

int nvm_read_fuse(void *nvm_ptr, int fusenum, u8 *data)
{
    /*
    Read one fuse value
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Read Fuse");

    /*  
    if (!nvm->progmode) {
        DBG_INFO(NVM_DEBUG, "Enter progmode first!");
        return -2;
    }
    */

    result = app_ld(APP(nvm), NVM_REG(nvm, fuses_address) + fusenum, data);
    if (!result) {
        DBG_INFO(NVM_DEBUG, "app_ld failed %d", result);
        return -3;
    }

    return 0;
}

int nvm_write_fuse(void *nvm_ptr, int fusenum, u8 fuseval)
{
    /*
    Writes one fuse value
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    u16 address, fuse_address;
    u8 val;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Write Fuse");

    if (!nvm->progmode) {
        DBG_INFO(NVM_DEBUG, "Enter progmode first!");
        return -2;
    }

    fuse_address = NVM_REG(nvm, fuses_address) + fusenum;
    address = NVM_REG(nvm, nvmctrl_address) + UPDI_NVMCTRL_ADDRL;
    val = fuse_address & 0xff;
    result = app_write_data(APP(nvm), address, &val, 1);
    if (!result) {
        DBG_INFO(NVM_DEBUG, "app_write_data fuse addrL failed %d", result);
        return -3;
    }

    address = NVM_REG(nvm, nvmctrl_address) + UPDI_NVMCTRL_ADDRH;
    val = fuse_address >> 8;
    result = app_write_data(APP(nvm), address, &val, 1);
    if (!result) {
        DBG_INFO(NVM_DEBUG, "app_write_data fuse addrH failed %d", result);
        return -4;
    }

    address = NVM_REG(nvm, nvmctrl_address) + UPDI_NVMCTRL_DATAL;
    val = fuseval;
    result = app_write_data(APP(nvm), address, &val, 1);
    if (!result) {
        DBG_INFO(NVM_DEBUG, "app_write_data fuse value failed %d", result);
        return -5;
    }

    address = NVM_REG(nvm, nvmctrl_address) + UPDI_NVMCTRL_CTRLA;
    val = UPDI_NVMCTRL_CTRLA_WRITE_FUSE;
    result = app_write_data(APP(nvm), address, &val, 1);
    if (!result) {
        DBG_INFO(NVM_DEBUG, "app_write_data fuse cmd failed %d", result);
        return -6;
    }

    return 0;
}

int nvm_get_flash_info(void *nvm_ptr, flash_info_t *info)
{
    /*
    get flash size
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Get chip flash info");

    memcpy(info, &nvm->dev->mmap->flash, sizeof(*info));

    return 0;
}
