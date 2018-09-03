/*
Copyright(c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""
*/

#include "os/platform.h"
#include "device/device.h"
#include "application.h"
#include "nvm.h"
#include "constants.h"

/*
    NVM level memory struct
    @mgwd: magicword
    @progmode: Unlock mode flag
    @app: pointer to app object
    @dev: point chip dev object
*/
typedef struct _upd_nvm {
#define UPD_NVM_MAGIC_WORD 'unvm'
    unsigned int mgwd;  //magic word
    bool progmode;
    void *app;
    device_info_t *dev;
}upd_nvm_t;

/*
    Macro definition of NVM level
    @VALID_NVM(): check whether valid NVM object
    @APP(): get app object ptr
    @NVM_REG(): chip reg address
    @NVM_FLASH: chip flash address
*/
#define VALID_NVM(_nvm) ((_nvm) && ((_nvm)->mgwd == UPD_NVM_MAGIC_WORD))
#define APP(_nvm) ((_nvm)->app)
#define NVM_REG(_nvm, _name) ((_nvm)->dev->mmap->reg._name)
#define NVM_FLASH(_nvm, _name) ((_nvm)->dev->mmap->flash._name)

/*
    NVM object init
    @port: serial port name of Window or Linux
    @baud: baudrate
    @dev: point chip dev object
    @return NVM ptr, NULL if failed
*/
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

/*
    NVM object destroy
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
*/
void updi_nvm_deinit(void *nvm_ptr)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    if (VALID_NVM(nvm)) {
        DBG_INFO(NVM_DEBUG, "<NVM> deinit nvm");

        updi_application_deinit(APP(nvm));
        free(nvm);
    }
}

/*
    NVM get device ID information
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 successful, other value failed
*/
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

/*
    NVM set chip into Unlocked Mode with UPDI_KEY_NVM command
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 successful, other value failed
*/
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

/*
    NVM chip leave Locked Mode
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 successful, other value failed
*/
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

/*
    NVM set chip into Locked Mode with UPDI_KEY_CHIPERASE command
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 successful, other value failed
*/
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

/*
    NVM erase flash with UPDI_NVMCTRL_CTRLA_CHIP_ERASE command
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 successful, other value failed
*/
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

/*
    NVM read flash
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data output buffer
    @len: data len
    @return 0 successful, other value failed
*/
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
        DBG_INFO(NVM_DEBUG, "Flash read at locked mode");
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

/*
    NVM write flash
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_write_flash(void *nvm_ptr, u16 address, const u8 *data, int len)
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

/*
    NVM read fuse
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
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
 
    if (!nvm->progmode) {
        DBG_INFO(NVM_DEBUG, "Fuse read at locked mode");
    }

    result = app_ld(APP(nvm), NVM_REG(nvm, fuses_address) + fusenum, data);
    if (!result) {
        DBG_INFO(NVM_DEBUG, "app_ld failed %d", result);
        return -3;
    }

    return 0;
}

/*
    NVM write fuse
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @fusenum: fuse index of the reg
    @fuseval: fuse value
    @return 0 successful, other value failed
*/
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

/*
    NVM read memory
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data output buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_read_mem(void *nvm_ptr, u16 address, u8 *data, int len)
{
    /*
        Read Memory
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Read memory");

    if (!nvm->progmode) {
        DBG_INFO(NVM_DEBUG, "Memory read at locked mode");
    }

    return app_read_data_bytes(APP(nvm), address, data, len);
}

/*
    NVM write memory
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_write_mem(void *nvm_ptr, u16 address, const u8 *data, int len)
{
    /*
        Write Memory
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Write Memory");

    if (!nvm->progmode) {
        DBG_INFO(NVM_DEBUG, "Memory write at locked mode");
    }

    return app_write_data_bytes(APP(nvm), address, data, len);
}

/*
    NVM write memory
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @info: chip flash information
    @return 0 successful, other value failed
*/
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
