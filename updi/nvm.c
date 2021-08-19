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
#define UPD_NVM_MAGIC_WORD 0xD2D2 //'unvm'
    unsigned int mgwd;  //magic word
    bool progmode;
    void *app;
    const device_info_t *dev;
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

/*
    NVM object init
    @port: serial port name of Window or Linux
    @baud: baudrate
	@gaurd: gaurd time of when the transmission direction switches
    @dev: point chip dev object
    @return NVM ptr, NULL if failed
*/
void *updi_nvm_init(const char *port, int baud, int gaurd, const void *dev)
{
    upd_nvm_t *nvm = NULL;
    void *app;

    DBG_INFO(NVM_DEBUG, "<NVM> init nvm");

    app = updi_application_init(port, baud, gaurd, dev);
    if (app) {
        nvm = (upd_nvm_t *)malloc(sizeof(*nvm));
        nvm->mgwd = UPD_NVM_MAGIC_WORD;
        nvm->progmode = false;
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
    if (result) {
        DBG_INFO(NVM_DEBUG, "app_disable failed %d", result);
        return -2;
    }

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
    if (result) {
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
    NVM read common nvm area(flash/eeprom/userrow/fuses)
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @info: NVM memory info
    @address: target address
    @data: data output buffer
    @len: data len
    @return 0 successful, other value failed
*/
int _nvm_read_common(void *nvm_ptr, const nvm_info_t *info, u16 address, u8 *data, int len)
{
    /*
    Read from nvm area
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;

    if (!VALID_NVM(nvm) || !data)
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Read from nvm area");

    if (!nvm->progmode) {
        DBG_INFO(NVM_DEBUG, "NVM area read at locked mode");
        return -2;
    }

    if (address < info->nvm_start)
        address += info->nvm_start;

    if (address + len > info->nvm_start + info->nvm_size) {
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
int nvm_read_flash(void *nvm_ptr, u16 address, u8 *data, int len)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int result;

    result = nvm_get_block_info(nvm, NVM_FLASH, &info);
    if (result) {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -2;
    }

    return _nvm_read_common(nvm_ptr, &info, address, data, len);
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
    nvm_info_t info;
    int i, off, size, flash_address, flash_size, pages, page_size;
    int result = 0;

    if (!VALID_NVM(nvm) || !data)
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Writes to flash");

    if (!nvm->progmode) {
        DBG_INFO(NVM_DEBUG, "Enter progmode first!");
        return -2;
    }

    result = nvm_get_block_info(nvm, NVM_FLASH, &info);
    if (result) {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -3;
    }

    flash_address = info.nvm_start;
    flash_size = info.nvm_size;
    if (address < flash_address)
        address += flash_address;

    if (address + len > flash_address + flash_size) {
        DBG_INFO(NVM_DEBUG, "flash address overflow, addr %hx, len %x.", address, len);
        return -4;
    }

    page_size = info.nvm_pagesize;
    pages = (len + page_size - 1) / page_size;
    for (i = 0, off = 0; i < pages; i++) {
        DBG_INFO(NVM_DEBUG, "Writing flash page(%d/%d) at 0x%x", i, pages, address + off);

        size = len - off;
        if (size > page_size)
            size = page_size;

        result = app_write_nvm(APP(nvm), address + off, data + off, size);
        if (result) {
            DBG_INFO(NVM_DEBUG, "app_write_nvm failed %d", result);
            break;
        }

        off += page_size;
    }


    if (i < pages || result) {
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
int nvm_read_eeprom(void *nvm_ptr, u16 address, u8 *data, int len)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int result;

    result = nvm_get_block_info(nvm, NVM_EEPROM, &info);
    if (result) {
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
int nvm_read_userrow(void *nvm_ptr, u16 address, u8 *data, int len)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int result;

    result = nvm_get_block_info(nvm, NVM_USERROW, &info);
    if (result) {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -2;
    }

    return _nvm_read_common(nvm_ptr, &info, address, data, len);
}

/*
NVM write eeprom (compatible with userrow)
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @info: EEPROM memory info
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int _nvm_write_eeprom(void *nvm_ptr, const nvm_info_t *info, u16 address, const u8 *data, int len)
{
    /*
    Writes to eeprom
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int i, off, size, pages, page_size;
    int result = 0;

    if (!VALID_NVM(nvm) || !data)
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Writes to eeprom");

    if (!nvm->progmode) {
        DBG_INFO(NVM_DEBUG, "Enter progmode first!");
        return -2;
    }

    if (address < info->nvm_start)
        address += info->nvm_start;

    if (address + len > info->nvm_start + info->nvm_size) {
        DBG_INFO(NVM_DEBUG, "eeprom address overflow, addr %hx, len %x.", address, len);
        return -3;
    }

    page_size = info->nvm_pagesize;
    pages = (len + page_size - 1) / page_size;
    for (i = 0, off = 0; i < pages; i++) {
        DBG_INFO(NVM_DEBUG, "Writing eeprom page(%d/%d) at 0x%x", i, pages, address + off);

        size = len - off;
        if (size > page_size)
            size = page_size;

        result = _app_erase_write_nvm(APP(nvm), address + off, data + off, size, false);
        if (result) {
            DBG_INFO(NVM_DEBUG, "app_write_nvm failed %d", result);
            break;
        }

        off += page_size;
    }

    if (i < pages || result) {
        DBG_INFO(NVM_DEBUG, "Write eeprom page %d failed %d", i, result);
        return -5;
    }

    return 0;
}

/*
    NVM write eeprom
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_write_eeprom(void *nvm_ptr, u16 address, const u8 *data, int len)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int result;

    result = nvm_get_block_info(nvm, NVM_EEPROM, &info);
    if (result) {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -2;
    }

    return _nvm_write_eeprom(nvm_ptr, &info, address, data, len);
}

/*
    NVM write userrow
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_write_userrow(void *nvm_ptr, u16 address, const u8 *data, int len)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int result;

    result = nvm_get_block_info(nvm, NVM_USERROW, &info);
    if (result) {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -2;
    }

    return _nvm_write_eeprom(nvm_ptr, &info, address, data, len);
}

/*
    NVM read fuse
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_read_fuse(void *nvm_ptr, u16 address, u8 *data, int len)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int result;

    result = nvm_get_block_info(nvm, NVM_FUSES, &info);
    if (result) {
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
    u16 nvmctrl_address = NVM_REG(nvm, nvmctrl_address);
    u16 data;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Writes to fuse(hex) [%04hX]: %02hhX", address, value);

    if (!nvm->progmode) {
        DBG_INFO(NVM_DEBUG, "Enter progmode first!");
        return -2;
    }

    if (address < info->nvm_start)
        address += info->nvm_start;

    if (address >= info->nvm_start + info->nvm_size) {
        DBG_INFO(NVM_DEBUG, "fuse address overflow, addr %hx.", address);
        return -3;
    }

    // Check that NVM controller is ready
    result = app_wait_flash_ready(APP(nvm), TIMEOUT_WAIT_FLASH_READY);
    if (result) {
        DBG_INFO(APP_DEBUG, "app_wait_flash_ready timeout before page buffer clear failed %d", result);
        return -2;
    }

    result = app_write_data_bytes(APP(nvm), nvmctrl_address + UPDI_NVMCTRL_ADDRL, (u8 *)&address, 2);
    if (result) {
        DBG_INFO(NVM_DEBUG, "app_write_data_bytes fuse address %04x failed %d", address, result);
        return -4;
    }

    data = value;
    result = app_write_data_bytes(APP(nvm), nvmctrl_address + UPDI_NVMCTRL_DATAL, (u8 *)&data, 2);
    if (result) {
        DBG_INFO(NVM_DEBUG, "app_write_data_bytes fuse data %02x failed %d", data, result);
        return -5;
    }

    result = app_execute_nvm_command(APP(nvm), UPDI_NVMCTRL_CTRLA_WRITE_FUSE);
    if (result) {
        DBG_INFO(NVM_DEBUG, "app_execute_nvm_command fuse command failed %d", result);
        return -6;
    }

    return 0;
}

/*
    NVM write fuse
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @info: Fuse memory info
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_write_fuse(void *nvm_ptr, u16 address, const u8 *data, int len)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    int i, result;
	u8 val;

    result = nvm_get_block_info(nvm, NVM_FUSES, &info);
    if (result) {
        DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
        return -3;
    }

    for (i = 0; i < len; i++) {
		// compare current value then write
		result = _nvm_read_common(nvm_ptr, &info, address + i, &val, 1);
		if (result || val != data[i]) {
			result = _nvm_write_fuse(nvm_ptr, &info, address + i, data[i]);
			if (result) {
				DBG_INFO(NVM_DEBUG, "_nvm_write_fuse fuse (%d) failed %d", i, result);
				return -2;
			}
		}
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
    int size, off;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Read memory 0x%x size %d(0x%x)", address, len, len);

    if (!nvm->progmode)
        DBG_INFO(NVM_DEBUG, "Memory read at locked mode");

    off = 0;
    do {
        size = len - off;
        if (size > UPDI_MAX_TRANSFER_SIZE)
            size = UPDI_MAX_TRANSFER_SIZE;
    
        //DBG_INFO(NVM_DEBUG, "Reading Memory %d bytes at address 0x%x", size, address + off);

        result = app_read_data_bytes(APP(nvm), address + off, data + off, size);
        if (result) {
            DBG_INFO(NVM_DEBUG, "app_read_data_bytes failed %d at 0x%x, size %d", result, address + off, size);
            break;
        }

        off += size;
    } while (off < len);

    return result;
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
    int size, off;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Write Memory");

    if (!nvm->progmode)
        DBG_INFO(NVM_DEBUG, "Memory write at locked mode");

    off = 0;
    do {
        size = len - off;
        if (size > UPDI_MAX_TRANSFER_SIZE)
            size = UPDI_MAX_TRANSFER_SIZE;

        DBG_INFO(NVM_DEBUG, "Writing Memory %d bytes at address 0x%x", size, address + off);

        result = app_write_data_bytes(APP(nvm), address + off, data + off, size);
        if (result) {
            DBG_INFO(NVM_DEBUG, "app_write_data_bytes failed %d", result);
            break;
        }

        off += size;
    } while (off < len);

    return result;
}

/*
    NVM write auto selec which part to be operated
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @address: target address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int nvm_write_auto(void *nvm_ptr, u16 address, const u8 *data, int len)
{
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    nvm_info_t info;
    nvm_op op, nvm_ops[] = { nvm_write_flash, nvm_write_eeprom, nvm_write_userrow, nvm_write_fuse, nvm_write_mem };
    int i, result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Write Auto");

    for (i = 0; i < NUM_NVM_TYPES; i++) {
        result = nvm_get_block_info(nvm_ptr, i, &info);
        if (result) {
            DBG_INFO(NVM_DEBUG, "<NVM> nvm_get_block_info %d failed", i);
            return -2;
        }

        if (address >= info.nvm_start && address < info.nvm_start + info.nvm_size) {
            if (address + len <= info.nvm_start + info.nvm_size) {
                op = nvm_ops[i];
                if (op && len)
                    return op(nvm_ptr, address, data, len);

                DBG_INFO(NVM_DEBUG, "<NVM> Not support block op %p size %d", op, len);
                return -3;
            }
            else {
                DBG_INFO(NVM_DEBUG, "<NVM> write auto - block overflow (addr, len): target(%x, %x) / memory(%x, %x) ", address, len, info.nvm_start, info.nvm_size);
                return -4;
            }
        }
    }

    return 0;
}

/*
    NVM reset
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 successful, other value failed
*/
int nvm_reset(void *nvm_ptr, int delay_ms)
{
    /*
        Reset
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    int result;

    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    DBG_INFO(NVM_DEBUG, "<NVM> Reset");

    result = app_toggle_reset(APP(nvm), 1);
    if (result) {
        DBG_INFO(NVM_DEBUG, "app_toggle_reset failed %d", result);
        return -2;
    }

    if (delay_ms)
        msleep(delay_ms);

    if (nvm->progmode) {
        result = app_enter_progmode(APP(nvm));
        if (result) {
            DBG_INFO(APP_DEBUG, "app_enter_progmode, faled result %d", result);
            return -3;
        }
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
int nvm_get_block_info(void *nvm_ptr, int type, nvm_info_t *info)
{
    /*
        get block info
    */
    upd_nvm_t *nvm = (upd_nvm_t *)nvm_ptr;
    
    if (!VALID_NVM(nvm))
        return ERROR_PTR;

    //DBG_INFO(NVM_DEBUG, "<NVM> Get chip nvm type %d info", type);

    return dev_get_nvm_info(nvm->dev, type, info);
}
