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
typedef struct _upd_nvm
{
#define UPD_NVM_MAGIC_WORD 0xD2D2 //'unvm'
    unsigned int mgwd;            // magic word
    bool progmode;
    bool erased;
    void *app;
    const device_info_t *dev;
} upd_nvm_t;

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

/*
    NVM object init
    @port: serial port name of Window or Linux
    @baud: baudrate
    @guard: guard time of when the transmission direction switches
    @dev: point chip dev object
    @return NVM ptr, NULL if failed
*/
void *updi_nvm_init(const char *port, int baud, int guard, int breaks, const void *dev)
{
    upd_nvm_t *nvm = NULL;
    size_t size;
    void *app;

    DBG_INFO(NVM_DEBUG, "<NVM> init nvm");

    app = updi_application_init(port, baud, guard, breaks, dev);
    if (app)
    {
        size = sizeof(*nvm);
        nvm = (upd_nvm_t *)malloc(size);
        if (!nvm)
        {
            DBG_INFO(NVM_DEBUG, "<NVM> nvm malloc memory(%d) failed", size);
            updi_application_deinit(app);
            return NULL;
        }

        memset(nvm, 0, size);
        nvm->mgwd = UPD_NVM_MAGIC_WORD;
        nvm->progmode = false;
        nvm->erased = false;
        nvm->dev = (const device_info_t *)dev;
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
    if (VALID_NVM(nvm))
    {
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
    NVM set chip into Program Mode with UPDI_KEY_NVM command
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
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "app_enter_progmode failed %d", result);
        return -2;
    }

    nvm->progmode = true;

    return 0;
}

/*
    NVM chip leave Program Mode
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @reset_or_halt: reset mcu or halt mcu
    @return 0 successful, other value failed
*/
int nvm_leave_progmode(void *nvm_ptr, bool reset_or_halt)
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

    result = app_leave_progmode(APP(nvm), reset_or_halt);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "app_leave_progmode failed %d", result);
        return -2;
    }

    nvm->progmode = false;

    return 0;
}

/*
    Check NVM chip whether is in progmode
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 successful, other value failed
*/
bool nvm_in_progmode(void *nvm_ptr)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;

    if (!VALID_NVM(nvm))
        return false;

    return nvm->progmode;
}

/*
NVM chip disable UPDI interface temporarily
@nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
@return 0 successful, other value failed
*/
int nvm_disable(void *nvm_ptr)
{
    /*
    Disable UPDI interface temporarily
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Disable UPDI interface");

    result = app_disable(APP(nvm));
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "app_disable failed %d", result);
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

    if (nvm->progmode)
        DBG_INFO(NVM_DEBUG, "Device in programe mode and unlocked");

    // Unlock
    result = app_unlock(APP(nvm));
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "app_unlock failed %d", result);
        return -2;
    }

    // Unlock after using the NVM key results in prog mode.
    nvm->progmode = true;
    nvm->erased = true;

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

    if (!nvm->progmode)
    {
        DBG_INFO(NVM_DEBUG, "Enter progmode first!");
        return -2;
    }

    result = app_chip_erase(APP(nvm));
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "app_chip_erase failed %d", result);
        return -3;
    }

    nvm->erased = true;

    return 0;
}

/*
    NVM read common nvm area(flash/eeprom/userrow/fuses)
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @info: NVM memory info
    @address: target address
    @data: data output buffer
    @len: data len
    @return 0 successful, other value failed
*/
int _nvm_read_common(void *nvm_ptr, const nvm_info_t *info, u32 address, u8 *data, int len)
{
    /*
    Read from common area
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    unsigned int start, size;

    if (!VALID_NVM(nvm) || !data)
        return ERROR_PTR;

    DBG_INFO(OTHER_DEBUG, "<NVM> Read from common area");

    if (!nvm->progmode)
    {
        DBG_INFO(NVM_ERROR, "NVM area read at locked mode(may be denied)");
        // return -2;
    }

    start = info->nvm_mapped_start ? info->nvm_mapped_start : info->nvm_start;
    size = info->nvm_size;
    if (address < start || address + len > start + size)
    {
        DBG_INFO(NVM_DEBUG, "nvm area address overflow, addr %hx, len %x.", address, len);
        return -3;
    }

    return nvm_read_mem(nvm_ptr, address, data, len);
}

/*
    NVM read flash
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data output buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_read_flash(void *nvm_ptr, u32 address, u8 *data, int len)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int flash_address, flash_size, size, block_start, block_end, block_size, block_mask;
    u8 bid;
    int result;

    result = nvm_get_block_info(nvm, NVM_FLASH, &info);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -2;
    }

    flash_address = info.nvm_mapped_start ? info.nvm_mapped_start : info.nvm_start;
    flash_size = info.nvm_size;
    if (address < (u32)flash_address || address + len > (u32)(flash_address + flash_size))
    {
        DBG_INFO(NVM_DEBUG, "flash address overflow, addr %hx, len %x.", address, len);
        return -3;
    }

    block_size = info.nvm_blocksize;
    block_mask = block_size - 1;

    do
    {
        block_start = address - info.nvm_mapped_start;
        bid = block_start / block_size;
        block_start &= block_mask;

        block_end = block_start + len;
        if (block_end > block_size)
        {
            size = block_size - block_start;
        }
        else
        {
            size = block_end - block_start;
        }

        DBG_INFO(NVM_DEBUG, "Reading flash block %d at 0x%x(@0x%x) size %d", bid, flash_address + block_start, address, size);

        result = app_read_nvm(APP(nvm), bid, flash_address + block_start, data, size);
        if (result)
        {
            DBG_INFO(NVM_DEBUG, "app_read_nvm failed %d", result);
            return -4;
        }

        data += size;
        address += size;
        len -= size;

    } while (len > 0);

    return 0;
}

/*
    NVM write flash
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @erased: Has the chip been erased yet
    @return 0 successful, other value failed
*/
int nvm_write_flash(void *nvm_ptr, u32 address, const u8 *data, int len, bool erased)
{
    /*
    Writes to flash
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int i, off, size, flash_address, flash_size, pages, page_size, block_start, block_size, block_mask;
    u8 bid;
    int result = 0;

    if (!VALID_NVM(nvm) || !data)
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Writes to flash");

    if (!nvm->progmode)
    {
        DBG_INFO(NVM_DEBUG, "Enter progmode first!");
        return -2;
    }

    result = nvm_get_block_info(nvm, NVM_FLASH, &info);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -3;
    }

    flash_address = info.nvm_mapped_start ? info.nvm_mapped_start : info.nvm_start;
    flash_size = info.nvm_size;
    if (address < (u32)flash_address || address + len > (u32)(flash_address + flash_size))
    {
        DBG_INFO(NVM_DEBUG, "flash address overflow, addr %hx, len %x.", address, len);
        return -4;
    }

    page_size = info.nvm_pagesize;
    block_size = info.nvm_blocksize;
    block_mask = block_size - 1;
    pages = (len + page_size - 1) / page_size;
    for (i = 0, off = 0; i < pages; i++)
    {
        size = len - off;
        if (size > page_size)
            size = page_size;

        block_start = address + off - info.nvm_mapped_start;
        bid = block_start / block_size;
        block_start &= block_mask;

        DBG_INFO(NVM_DEBUG, "Writing flash block %d page(%d/%d) at 0x%x(@0x%x)", bid, i, pages, block_start, address + off);
        if (erased)
        {
            result = app_write_flash(APP(nvm), bid, flash_address + block_start, data + off, size, true);
        }
        else
        {
            result = app_erase_write_flash(APP(nvm), bid, flash_address + block_start, data + off, size, true);
        }

        if (result)
        {
            DBG_INFO(NVM_DEBUG, "app_write_flash failed %d", result);
            break;
        }

        off += size;
    }

    if (i < pages || result)
    {
        DBG_INFO(NVM_DEBUG, "Write flash page %d failed %d", i, result);
        return -6;
    }

    return 0;
}

/*
NVM read eeprom
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_read_eeprom(void *nvm_ptr, u32 address, u8 *data, int len)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int result;

    result = nvm_get_block_info(nvm, NVM_EEPROM, &info);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -2;
    }

    return _nvm_read_common(nvm_ptr, &info, address, data, len);
}

/*
NVM read userrow
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_read_userrow(void *nvm_ptr, u32 address, u8 *data, int len)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int result;

    result = nvm_get_block_info(nvm, NVM_USERROW, &info);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -2;
    }

    return _nvm_read_common(nvm_ptr, &info, address, data, len);
}

/*
NVM write eeprom (compatible with userrow)
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @type: memory type
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int _nvm_write_user_eeprom(void *nvm_ptr, int type, u32 address, const u8 *data, int len)
{
    /*
    Writes to user/eeprom
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int i, off, pages, page_size, size;
    unsigned int start;
    int result = 0;

    if (!VALID_NVM(nvm) || !data)
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Writes to user/eeprom");

    if (!nvm->progmode)
    {
        DBG_INFO(NVM_DEBUG, "Enter progmode first!");
        return -2;
    }

    result = nvm_get_block_info(nvm, type, &info);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -3;
    }

    start = info.nvm_mapped_start ? info.nvm_mapped_start : info.nvm_start;
    size = (int)info.nvm_size;
    if (address < start || (u32)(address + len) > start + size)
    {
        DBG_INFO(NVM_DEBUG, "User/eeprom address overflow, addr %hx, len %x.", address, len);
        return -4;
    }

    page_size = info.nvm_pagesize;
    pages = (len + page_size - 1) / page_size;
    for (i = 0, off = 0; i < pages; i++)
    {
        DBG_INFO(NVM_DEBUG, "Writing user/eeprom page(%d/%d) at 0x%x", i, pages, address + off);

        size = len - off;
        if (size > page_size)
            size = page_size;

        if (type == NVM_EEPROM)
        {
            result = app_erase_write_eeprom(APP(nvm), address + off, data + off, size);
        }
        else if (type == NVM_USERROW)
        {
            result = app_erase_write_userrow(APP(nvm), address + off, data + off, size);
        }
        else
        {
            DBG_INFO(NVM_DEBUG, "Writing user/eeprom unsupport type %d", type);
            result = -5;
        }
        if (result)
        {
            DBG_INFO(NVM_DEBUG, "app_erase_write_user/eeprom(byte mode) failed %d", result);
            break;
        }

        off += page_size;
    }

    if (i < pages || result)
    {
        DBG_INFO(NVM_DEBUG, "Write user/eeprom page %d failed %d", i, result);
        return -6;
    }

    return 0;
}

/*
    NVM write eeprom
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    #dummy: not used
    @return 0 successful, other value failed
*/
int nvm_write_eeprom(void *nvm_ptr, u32 address, const u8 *data, int len, bool dummy)
{
    return _nvm_write_user_eeprom(nvm_ptr, NVM_EEPROM, address, data, len);
}

/*
    NVM write userrow
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @dummy: not used
    @return 0 successful, other value failed
*/
int nvm_write_userrow(void *nvm_ptr, u32 address, const u8 *data, int len, bool dummy)
{
    return _nvm_write_user_eeprom(nvm_ptr, NVM_USERROW, address, data, len);
}

/*
    NVM read fuse
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_read_fuse(void *nvm_ptr, u32 address, u8 *data, int len)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int result;

    result = nvm_get_block_info(nvm, NVM_FUSES, &info);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -3;
    }

    return _nvm_read_common(nvm_ptr, &info, address, data, len);
}

/*
    NVM write fuse
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @info: Fuse memory info
    @address: target address
    @value: fuse value
    @return 0 successful, other value failed
*/
int _nvm_write_fuse(void *nvm_ptr, const nvm_info_t *info, u16 address, const u8 value)
{
    /*
    Writes to fuse
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    unsigned int start, size;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Writes to fuse(hex) [%04hX]: %02hhX", address, value);

    if (!nvm->progmode)
    {
        DBG_INFO(NVM_DEBUG, "Enter progmode first!");
        return -2;
    }

    start = info->nvm_mapped_start ? info->nvm_mapped_start : info->nvm_start;
    size = info->nvm_size;
    if (address < start || address >= start + size)
    {
        DBG_INFO(NVM_DEBUG, "fuse address overflow, addr %hx.", address);
        return -3;
    }

    return app_write_fuse(APP(nvm), address, value);
}

/*
    NVM write fuse or lockbits
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @type: NVM type
    @address: target address
    @data: data buffer
    @len: data len
    @dummy: not used
    @return 0 successful, other value failed
*/
int _nvm_write_fuse_lockbits(void *nvm_ptr, int type, u32 address, const u8 *data, int len, bool dummy)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int i, result;
    u8 val;

    result = nvm_get_block_info(nvm, NVM_FUSES, &info);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -2;
    }

    for (i = 0; i < len; i++)
    {
        // compare current value then write
        result = _nvm_read_common(nvm_ptr, &info, address + i, &val, 1);
        if (result || val != data[i])
        {
            result = _nvm_write_fuse(nvm_ptr, &info, address + i, data[i]);
            if (result)
            {
                DBG_INFO(NVM_DEBUG, "_nvm_write_fuse fuse (%d) failed %d", i, result);
                return -3;
            }
        }
    }

    return 0;
}

/*
    NVM write fuse
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @dummy: not used
    @return 0 successful, other value failed
*/
int nvm_write_fuse(void *nvm_ptr, u32 address, const u8 *data, int len, bool dummy)
{
    return _nvm_write_fuse_lockbits(nvm_ptr, NVM_FUSES, address, data, len, dummy);
}

/*
    NVM write lockbits
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @dummy: not used
    @return 0 successful, other value failed
*/
int nvm_write_lockbits(void *nvm_ptr, u32 address, const u8 *data, int len, bool dummy)
{
    return _nvm_write_fuse_lockbits(nvm_ptr, NVM_LOCKBITS, address, data, len, dummy);
}

/*
    NVM read lockbits
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_read_lockbits(void *nvm_ptr, u32 address, u8 *data, int len)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int result;

    result = nvm_get_block_info(nvm, NVM_LOCKBITS, &info);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -3;
    }

    return _nvm_read_common(nvm_ptr, &info, address, data, len);
}

/*
    NVM read memory
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data output buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_read_mem(void *nvm_ptr, u32 address, u8 *data, int len)
{
    /*
        Read Memory
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(OTHER_DEBUG, "<NVM> Read memory");

    if (!nvm->progmode)
    {
        DBG_INFO(NVM_DEBUG, "Memory read at locked mode");
    }

    DBG_INFO(NVM_DEBUG, "Reading Memory %d(0x%x) bytes at address 0x%x", len, len, address);

    result = app_read_data_bytes(APP(nvm), address, data, len);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "app_read_data_bytes failed %d at 0x%x, size %d", result, address, len);
    }

    return result;
}

/*
    NVM read auto selec which part to be operated
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @dummy: not used
    @return 0 successful, negative value failed, positive value continue
*/

int _nvm_read_auto(void *nvm_ptr, u32 address, u8 *data, int len, u8 dummy)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    nvm_rop rop, nvm_rops[NUM_NVM_EX_TYPES] = {nvm_read_flash, nvm_read_eeprom, nvm_read_userrow, nvm_read_fuse, nvm_read_mem, nvm_read_lockbits};
    unsigned start, size;
    int i, result = 0;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Read Auto addr 0x%x len 0x%x(%d)", address, len, len);

    for (i = 0; i < NUM_NVM_EX_TYPES; i++)
    {
        if (nvm_get_block_info(nvm_ptr, i, &info))
        {
            DBG_INFO(NVM_DEBUG, "<NVM> nvm_get_block_info %d failed", i);
            return -2;
        }

        start = info.nvm_mapped_start ? info.nvm_mapped_start : info.nvm_start;
        size = info.nvm_size;

        if (address >= start && address < start + size)
        {
            if (address + len > start + size)
            {
                DBG_INFO(NVM_DEBUG, "<NVM> read auto - block overflow with (addr, len): target(0x%x, %d) / memory(0x%x, %d) ", address, len, start, size);
                len = start + size - address;
                DBG_INFO(NVM_DEBUG, "<NVM> NVM resize to %d", len);
            }

            rop = nvm_rops[i];
            if (rop)
            {
                result = rop(nvm_ptr, address, data, len);
                if (result)
                {
                    DBG_INFO(NVM_DEBUG, "<NVM> NVM rop return failed %d", result);
                    return -3;
                }

                break;
            }
            else
            {
                DBG_INFO(NVM_DEBUG, "<NVM> Not support nvm op %d size %d", i, len);
                return -4;
            }
        }
    }

    if (i == NUM_NVM_EX_TYPES)
    {
        DBG_INFO(NVM_DEBUG, "<NVM> read auto no op found with (0x%x, %d)", address, len);
        return -6;
    }

    return 0;
}

/*
    NVM read auto selec which part to be operated, real address first to check, if not found then use mapped address
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_read_auto(void *nvm_ptr, u32 address, u8 *data, int len)
{
    return _nvm_read_auto(nvm_ptr, address, data, len, 0);
}

/*
    NVM write memory
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @dummy: not used
    @return 0 successful, other value failed
*/
int nvm_write_mem(void *nvm_ptr, u32 address, const u8 *data, int len, bool dummy)
{
    /*
        Write Memory
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Write Memory");

    if (!nvm->progmode)
    {
        DBG_INFO(NVM_DEBUG, "Memory write at locked mode");
    }

    DBG_INFO(NVM_DEBUG, "Writing Memory %d(0x%x) bytes at address 0x%x", len, len, address);

    result = app_write_data_bytes(APP(nvm), address, data, len);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "app_write_data_bytes failed %d", result);
    }

    return result;
}

/*
    NVM write auto selec which part to be operated
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @flag: real address or mapped; whether readback
    @return 0 successful, other value failed
*/
int _nvm_write_auto(void *nvm_ptr, u32 address, const u8 *data, int len, u8 flag)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    nvm_wop wop = NULL, nvm_wops[NUM_NVM_EX_TYPES] = {nvm_write_flash, nvm_write_eeprom, nvm_write_userrow, nvm_write_fuse, nvm_write_mem, nvm_write_lockbits};
    nvm_rop rop, nvm_rops[NUM_NVM_EX_TYPES] = {nvm_read_flash, nvm_read_eeprom, nvm_read_userrow, nvm_read_fuse, nvm_read_mem, nvm_read_lockbits};
    bool chip_erased = nvm->erased;
    char *buf = NULL;
    unsigned int start, size;
    int i, result = 1;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Write Auto addr 0x%x len 0x%x(%d) (chip_erase %d)", address, len, len, chip_erased);

    // Get process function
    for (i = 0; i < NUM_NVM_EX_TYPES; i++)
    {
        if (nvm_get_block_info(nvm_ptr, i, &info))
        {
            DBG_INFO(NVM_DEBUG, "<NVM> nvm_get_block_info %d failed", i);
            return -2;
        }

        // Check data address range
        start = info.nvm_mapped_start ? info.nvm_mapped_start : info.nvm_start;
        size = info.nvm_size;
        if (address >= start &&
            address < start + size)
        {
            if (address + len > start + size)
            {
                DBG_INFO(NVM_DEBUG, "<NVM> write auto - block overflow with (addr, len): target(0x%x, %d) / memory(0x%x, %d) ", address, len, start, size);
                len = start + size - address;
                DBG_INFO(NVM_DEBUG, "<NVM> NVM resize to %d", len);
            }

            wop = nvm_wops[i];
            break;
        }
    }

    if (!wop || !len)
    {
        DBG_INFO(NVM_DEBUG, "<NVM> Not support block op %p size %d", wop, len);
        return -3;
    }

    // Process data
    result = wop(nvm_ptr, address, data, len, chip_erased);
    if (result == 0)
    {
        // Read back the data and do data check
        if (flag & FLAG_DATA_READBACK)
        {
            rop = nvm_rops[i];
            if (rop)
            {
                buf = malloc(len);
                if (buf)
                {
                    result = rop(nvm_ptr, address, buf, len);
                    if (result == 0)
                    {
                        result = memcmp(data, buf, len);
                        if (result)
                        {
                            DBG_INFO(NVM_DEBUG, "<NVM> Data verified compare failed %d", result);
                            result = -4;
                        }
                    }
                    else
                    {
                        DBG_INFO(NVM_DEBUG, "<NVM> Data readback for verification return failed %d", result);
                        result = -5;
                    }
                    free(buf);
                }
                else
                {
                    DBG_INFO(NVM_DEBUG, "<NVM> Malloc data checking buffer failed");
                }
            }
        }
    }
    else
    {
        DBG_INFO(NVM_DEBUG, "<NVM> NVM write data return failed %d", result);
    }

    if (result)
    {
        DBG_INFO(NVM_DEBUG, "<NVM> data write faield");
        return -6;
    }

    nvm->erased = false;

    return result;
}

/*
    NVM write auto selec which part to be operated
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @check: whether readback data for double checking
    @return 0 successful, other value failed
*/
int nvm_write_auto(void *nvm_ptr, u32 address, const u8 *data, int len, bool check)
{
    return _nvm_write_auto(nvm_ptr, address, data, len, check ? FLAG_DATA_READBACK : 0);
}

/*
    NVM erase flash page
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @count: page count
    @return 0 successful, other value failed
*/
int nvm_erase_flash_page(void *nvm_ptr, u32 address, int count)
{
    /*
    Erase to flash page
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int flash_address, flash_size, block_size;
    unsigned int mapped_start;
    u8 bid = BLOCK_ID_NA;
    int result = 0;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Erase flash page");

    if (!nvm->progmode)
    {
        DBG_INFO(NVM_DEBUG, "Enter progmode first!");
        return -2;
    }

    result = nvm_get_block_info(nvm, NVM_FLASH, &info);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -3;
    }

    flash_address = info.nvm_mapped_start ? info.nvm_mapped_start : info.nvm_start;
    flash_size = info.nvm_size;
    if (address < (u32)flash_address || address + count > (u32)(flash_address + flash_size))
    {
        DBG_INFO(NVM_DEBUG, "flash address overflow, addr %hx, len %x.", address, count);
        return -4;
    }

    block_size = info.nvm_blocksize;
    mapped_start = info.nvm_mapped_start;
    if (mapped_start)
    {
        bid = (address - flash_address) / block_size;
    }

    result = app_erase_flash_page(APP(nvm), bid, address, count);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "app_write_flash failed %d", result);
        return -5;
    }

    return 0;
}

/*
NVM erase eeprom
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 successful, other value failed
*/
int _nvm_erase_eeprom(void *nvm_ptr)
{
    /*
    Writes to eeprom
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    unsigned int start, size;
    int result = 0;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Erase eeprom");

    if (!nvm->progmode)
    {
        DBG_INFO(NVM_DEBUG, "Enter progmode first!");
        return -2;
    }

    result = nvm_get_block_info(nvm, NVM_EEPROM, &info);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -3;
    }

    start = info.nvm_mapped_start ? info.nvm_mapped_start : info.nvm_start;
    size = info.nvm_size;
    result = app_erase_eeprom(APP(nvm), start, size);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "app_erase_weeprom(byte mode) failed %d", result);
    }

    return result;
}

/*
    NVM erase eeprom
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @dummy1: not used
    @dummy2: not used
    @return 0 successful, other value failed
*/
int nvm_erase_eeprom(void *nvm_ptr, u32 dummy1, int dummy2)
{
    return _nvm_erase_eeprom(nvm_ptr);
}
/*
    NVM erase auto selec which part to be operated
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @count: page/size count
    @return 0 successful, other value failed
*/
int nvm_erase_auto(void *nvm_ptr, u32 address, int count)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    nvm_eop eop, nvm_eops[NUM_NVM_TYPES] = {nvm_erase_flash_page, nvm_erase_eeprom, NULL, NULL, NULL};
    char *buf = NULL;
    int i, result;
    unsigned int start, size;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Erase Auto addr(0x%x) count(%d) ", address, count);

    for (i = 0; i < NUM_NVM_TYPES; i++)
    {
        result = nvm_get_block_info(nvm_ptr, i, &info);
        if (result)
        {
            DBG_INFO(NVM_DEBUG, "<NVM> nvm_get_block_info %d failed", i);
            return -2;
        }

        start = info.nvm_mapped_start ? info.nvm_mapped_start : info.nvm_start;
        size = info.nvm_size;
        if (address >= start && address < start + size)
        {
            eop = nvm_eops[i];
            if (eop)
            {
                result = eop(nvm_ptr, address, count);
                // Read back the data and do data check
                if (result == 0)
                {
                    break;
                }
                else
                {
                    DBG_INFO(NVM_DEBUG, "<NVM> NVM Erase data return failed %d", result);
                }

                if (result)
                {
                    DBG_INFO(NVM_DEBUG, "<NVM> Malloc data erase faield");
                    return -3;
                }
            }
            else
            {
                DBG_INFO(NVM_DEBUG, "<NVM> Not support block op %p size %d", eop, count);
                return -4;
            }
        }
    }

    return 0;
}

/*
    NVM wait flash ready after operation
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 successful, other value failed
*/
int nvm_wait(void *nvm_ptr)
{
    /*
    Wait programming
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Wait");

    if (!nvm->progmode)
    {
        // DBG_INFO(NVM_DEBUG, "Not in programming mode, skip");
        return 0;
    }

    // Check that NVM controller is ready
    result = app_wait_flash_ready(APP(nvm), TIMEOUT_WAIT_FLASH_READY);
    if (result)
    {
        DBG_INFO(APP_DEBUG, "app_wait_flash_ready timeout failed %d", result);
        return -2;
    }

    return 0;
}

/*
    NVM reset
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @reset_or_halt: rest mcu or halt mcu
    @return 0 successful, other value failed
*/
int nvm_reset(void *nvm_ptr, int delay_ms, bool reset_or_halt)
{
    /*
        Reset
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Reset");

    result = app_toggle_reset(APP(nvm), reset_or_halt);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "app_toggle_reset failed %d", result);
        return -2;
    }

    if (delay_ms)
    {
        msleep(delay_ms);
    }

    /*
        if (nvm->progmode) {
            result = app_enter_progmode(APP(nvm));
            if (result) {
                DBG_INFO(APP_DEBUG, "app_enter_progmode after reset, failed result %d", result);
            }
            else {
                nvm->progmode = false;
            }
        }
    */
    nvm->erased = false;

    return result;
}

/*
    NVM halt
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 successful, other value failed
*/
int nvm_halt(void *nvm_ptr)
{
    /*
        Halt
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Halt");

    result = app_toggle_reset(APP(nvm), false);
    if (result)
    {
        DBG_INFO(NVM_DEBUG, "app_toggle_reset failed %d", result);
        return -2;
    }

    return result;
}

/*
    NVM get block info, this is defined in device.c
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @type: NVM type
    @info: chip flash information
    @return 0 successful, other value failed
*/
int nvm_get_block_info(void *nvm_ptr, int type, nvm_info_t *inf)
{
    /*
        get block info
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    // DBG_INFO(NVM_DEBUG, "<NVM> Get chip nvm type %d info", type);

    return dev_get_nvm_info(nvm->dev, type, inf);
}

/*
    NVM get block info, this is defined in device.c
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @type: NVM type
    @info: chip flash information
    @pname: output block name
    @return 0 successful, other value failed
*/
int nvm_get_block_info_ext(void *nvm_ptr, int type, nvm_info_t *info, char **pname)
{
    /*
        get block info
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    // DBG_INFO(NVM_DEBUG, "<NVM> Get chip nvm type %d info", type);

    return dev_get_nvm_info_ext(nvm->dev, type, info, pname);
}

/*
    Get flash content by indicated size, if negative mean whole flash
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @type: nvm type
    @req_size: size pointer for input, output buf size, NULL mean whole NVM block size
    @return buffer pointer, NULL indicated failed
*/
void *nvm_get_content(void *nvm_ptr, int type, int *req_size)
{
    nvm_info_t iblock;
    void *buf = NULL;
    int size;
    unsigned int start;
    int result;

    result = nvm_get_block_info(nvm_ptr, type, &iblock);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "nvm_get_flash_info failed %d", result);
        return NULL;
    }

    if (!req_size || *req_size <= 0)
        size = iblock.nvm_size;
    else
        size = *req_size;

    if (iblock.nvm_size < (u32)size)
    {
        DBG_INFO(UPDI_DEBUG, "size %d invalid", size);
        return NULL;
    }

    buf = malloc(size);
    if (!buf)
    {
        DBG_INFO(UPDI_DEBUG, "alloc memory size = %d  failed", size);
        return NULL;
    }

    start = iblock.nvm_mapped_start ? iblock.nvm_mapped_start : iblock.nvm_start;
    result = nvm_read_auto(nvm_ptr, start, (u8 *)buf, size);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "nvm_read_auto failed %d", result);
        free(buf);
        return NULL;
    }

    if (req_size)
        *req_size = size;

    return buf;
}