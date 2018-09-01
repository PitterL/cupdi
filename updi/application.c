#include "os/platform.h"
#include "device/device.h"
#include "link.h"
#include "application.h"
#include "constants.h"

typedef struct _upd_application {
#define UPD_APPLICATION_MAGIC_WORD 'uapp'
    unsigned int mgwd;  //magic word
    void *link;
    device_info_t *dev;
}upd_application_t;

#define VALID_APP(_app) ((_app) && ((_app)->mgwd == UPD_APPLICATION_MAGIC_WORD))
#define LINK(_app) ((_app)->link)
#define APP_REG(_app, _name) ((_app)->dev->mmap->reg._name)

void *updi_application_init(const char *port, int baud, void *dev)
{
    upd_application_t *app = NULL;
    void *link;

    _loginfo_i("<APP> init application");

    link = updi_datalink_init(port, baud);
    if (link) {
        app = (upd_application_t *)malloc(sizeof(*app));
        app->mgwd = UPD_APPLICATION_MAGIC_WORD;
        app->link = (void *)link;
        app->dev = (device_info_t *)dev;
    }

    return app;
}

void updi_application_deinit(void *app_ptr)
{
    upd_application_t *app = (upd_application_t *)app_ptr;
    if (VALID_APP(app)) {
        _loginfo_i("<APP> deinit application");

        updi_datalink_deinit(LINK(app));
        free(app);
    }
}

int app_device_info(void *app_ptr)
{
    /*
        Reads out device information from various sources
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    u8 sib[16];
    u8 pdi;
    u8 dev_id[3];
    u8 dev_rev[1];
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> Device info");

    result = link_read_sib(LINK(app), sib, sizeof(sib));
    if (result) {
        _loginfo_i("link_read_sib failed %d", result);
        return -2;
    }

    _loginfo("SIB read out as: ", sib, sizeof(sib), "0x%02x ");
    _loginfo("Family ID = ", sib, 8, "%c ");
    _loginfo_i("NVM revision = %c", sib[10]);
    _loginfo_i("OCD revision = %c", sib[13]);
    _loginfo_i("PDI OSC = %cMHz ", sib[15]);

    pdi = link_ldcs(LINK(app), UPDI_CS_STATUSA);
    _loginfo_i("PDI OSC = %hhdMHz ", (pdi >> 4));

    if (app_in_prog_mode(app)) {
        if (app->dev) {
            result = app_read_data(app, APP_REG(app, sigrow_address), dev_id, sizeof(dev_id));
            if (result) {
                _loginfo_i("app_read_data failed %d", result);
                return -3;
            }

            result = app_read_data(app, APP_REG(app, syscfg_address) + 1, dev_rev, sizeof(dev_rev));
            if (result) {
                _loginfo_i("app_read_data failed %d", result);
                return -4;
            }

            _loginfo_i("Device ID = %02x %02x %02x rev %c", dev_id[0], dev_id[1], dev_id[2], dev_rev[0] + 'A');
        }
    }

    return 0;
}

bool app_in_prog_mode(void *app_ptr)
{
    /*
        Checks whether the NVM PROG flag is up
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    u8 status;
    int result;
    bool ret = false;

    if (!VALID_APP(app))
        return ret;

    _loginfo_i("<APP> Prog mode");

    result = _link_ldcs(LINK(app), UPDI_ASI_SYS_STATUS, &status);
    if (!result && status & (1 << UPDI_ASI_SYS_STATUS_NVMPROG))
        ret = true;

    _loginfo_i("<APP> Prog mode %d", ret);

    return ret;
}

int app_wait_unlocked(void *app_ptr, int timeout)
{
    /*   
        Waits for the device to be unlocked.
        All devices boot up as locked until proven otherwise
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    u8 status;
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> Wait Unlock");

    do {
        result = _link_ldcs(LINK(app), UPDI_ASI_SYS_STATUS, &status);
        if (result) {
            _loginfo_i("_link_ldcs failed %d", result);
        }
        else {
            if (!(status & (1 << UPDI_ASI_SYS_STATUS_LOCKSTATUS)))
                break;
        }

        msleep(1);
    } while (--timeout > 0);

    if (timeout <= 0 || result) {
        _loginfo_i("Timeout waiting for device to unlock status %02x result %d", status, result);
        return -2;
    }

    return 0;
}

int app_unlock(void *app_ptr)
{
    /*
        Unlock and erase
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    u8 status;
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> unlock");

    // Put in the key
    result = link_key(LINK(app), UPDI_KEY_64, UPDI_KEY_CHIPERASE);
    if (result) {
        _loginfo_i("link_key failed %d", result);
        return -2;
    }

    // Check key status
    result = _link_ldcs(LINK(app), UPDI_ASI_KEY_STATUS, &status);
    if (result || !(status & (1 << UPDI_ASI_KEY_STATUS_CHIPERASE))) {
        _loginfo_i("_link_ldcs Chiperase Key not accepted(%d), status 0x%02x", result, status);
        return -3;
    }

    //Toggle reset
    result = app_toggle_reset(app_ptr, 1);
    if (result) {
        _loginfo_i("app_toggle_reset failed %d", result);
        return -4;
    }

    //And wait for unlock
    result = app_wait_unlocked(app, 100);
    if (result) {
        _loginfo_i("Failed to chip erase using key result %d", result);
        return -5;
    }
    
    return 0;
}

int app_enter_progmode(void *app_ptr)
{
    /*
        Enters into NVM programming mode
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    u8 status;
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> Enter Progmode");

    // First check if NVM is already enabled
    if (app_in_prog_mode(app_ptr)) {
        _loginfo_i("Already in NVM programming mode");
        return 0;
    }

    _loginfo_i("Entering NVM programming mode");

    // Put in the key
    result = link_key(LINK(app), UPDI_KEY_64, UPDI_KEY_NVM);
    if (result) {
        _loginfo_i("link_key failed %d", result);
        return -2;
    }

    // Check key status
    result = _link_ldcs(LINK(app), UPDI_ASI_KEY_STATUS, &status);
    if (result || !(status & (1 << UPDI_ASI_KEY_STATUS_NVMPROG))) {
        _loginfo_i("_link_ldcs Nvm Key not accepted(%d), status 0x%02x", result, status);
        return -3;
    }

    //Toggle reset
    result = app_toggle_reset(app_ptr, 1);
    if (result) {
        _loginfo_i("app_toggle_reset failed %d", result);
        return -4;
    }

    //And wait for unlock
    result = app_wait_unlocked(app_ptr, 100);
    if (result) {
        _loginfo_i("Failed to enter NVM programming mode: device is locked result %d", result);
        return -5;
    }

    if (!app_in_prog_mode(app_ptr)) {
        _loginfo_i("Failed to enter NVM programming mode");
        return -6;
    }else {
        _loginfo_i("Now in NVM programming mode");
        return 0;
    }
}

int app_leave_progmode(void *app_ptr)
{
    /*
        Disables UPDI which releases any keys enabled
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> Now in NVM programming mode");

    result = app_toggle_reset(app_ptr, 1);
    if (result) {
        _loginfo_i("app_toggle_reset failed %d", result);
        return -2;
    }

    result = link_stcs(LINK(app), UPDI_CS_CTRLB, (1 << UPDI_CTRLB_UPDIDIS_BIT) | (1 << UPDI_CTRLB_CCDETDIS_BIT));
    if (result) {
        _loginfo_i("link_stcs failed %d", result);
        return -3;
    }

    return 0;
}

int app_reset(void *app_ptr, bool apply_reset)
{
    /*
    Applies or releases an UPDI reset condition
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> Reset %d", apply_reset);

    if (apply_reset) {
        _loginfo_i("Apply reset");
        result = link_stcs(LINK(app), UPDI_ASI_RESET_REQ, UPDI_RESET_REQ_VALUE);
    }
    else {
        _loginfo_i("Release reset");
        result = link_stcs(LINK(app), UPDI_ASI_RESET_REQ, 0);
    }

    if (result) {
        _loginfo_i("link_stcs failed %d", result);
        return -2;
    }

    return 0;
}

int app_toggle_reset(void *app_ptr, int delay)
{
    /*
    Toggle an UPDI reset condition
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> Toggle Reset %d");

    //Toggle reset
    result = app_reset(app, true);
    if (result) {
        _loginfo_i("app_reset failed %d", result);
        return -2;
    }

    msleep(delay);

    result = app_reset(app, false);
    if (result) {
        _loginfo_i("app_reset failed %d", result);
        return -3;
    }

    return 0;
}

int app_wait_flash_ready(void *app_ptr, int timeout)
{
    /*
        Waits for the NVM controller to be ready
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    u8 status;
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> Wait flash ready");

    do {
        result = _link_ld(LINK(app), APP_REG(app, nvmctrl_address) + UPDI_NVMCTRL_STATUS, &status);
        if (result) {
            _loginfo_i("_link_ld failed %d", result);
            result = -2;
            break;
        }
        else {
            if (status & (1 << UPDI_NVM_STATUS_WRITE_ERROR)) {
                result = -3;
                break;
            }

            if (!(status & ((1 << UPDI_NVM_STATUS_EEPROM_BUSY) | (1 << UPDI_NVM_STATUS_FLASH_BUSY))))
                break;
        }

        msleep(1);
    } while (--timeout > 0);

    if (timeout <= 0 || result) {
        _loginfo_i("Timeout waiting for wait flash ready status %02x result %d", status, result);
        return -3;
    }

    return 0;
}

int app_execute_nvm_command(void *app_ptr, u8 command)
{
    /*
        Executes an NVM COMMAND on the NVM CTRL
    */
    upd_application_t *app = (upd_application_t *)app_ptr;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> NVMCMD %d executing", command);

    return link_st(LINK(app), APP_REG(app, nvmctrl_address) + UPDI_NVMCTRL_CTRLA, command);
}

int app_chip_erase(void *app_ptr)
{
    /*
        Does a chip erase using the NVM controller
        Note that on locked devices this it not possible and the ERASE KEY has to be used instead
    */

    upd_application_t *app = (upd_application_t *)app_ptr;
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> Chip erase using NVM CTRL");

    //Wait until NVM CTRL is ready to erase
    result = app_wait_flash_ready(app, 1000);
    if (result) {
        _loginfo_i("app_wait_flash_ready timeout before erase failed %d", result);
        return -2;
    }

    //Erase
    result = app_execute_nvm_command(app, UPDI_NVMCTRL_CTRLA_CHIP_ERASE);
    if (result) {
        _loginfo_i("app_execute_nvm_command failed %d", result);
        return -3;
    }

    // And wait for it
    result = app_wait_flash_ready(app, 1000);
    if (result) {
        _loginfo_i("app_wait_flash_ready timeout after erase failed %d", result);
        return -2;
    }

    return 0;
}

int app_write_data_words(void *app_ptr, u16 address, u8 *data, int len)
{
    /*
        Writes a number of words to memory
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> Write words data(%d) addr: %hX", len, address);
    
    // Special-case of 1 word
    if (len == 2) {
        result = link_st16(LINK(app), address, data[0] + (data[1] << 8));
        if (result) {
            _loginfo_i("link_st16 failed %d", result);
            return -3;
        }
    }

    // Range check
    if (len > ((UPDI_MAX_REPEAT_SIZE + 1) << 1)) {
        _loginfo_i("Write words data length out of size %d", len);
        return -3;
    }

    // Store the address
    result = link_st_ptr(LINK(app), address);
    if (result) {
        _loginfo_i("link_st_ptr failed %d", result);
        return -4;
    }

    //Fire up the repeat
    if (len > 2) {
        result = link_repeat(LINK(app), len >> 1);
        if (result) {
            _loginfo_i("link_repeat failed %d", result);
            return -5;
        }
    }

    result = link_st_ptr_inc16(LINK(app), data, len);
    if (result) {
        _loginfo_i("link_st_ptr_inc16 failed %d", result);
        return -6;
    }

    return 0;
}

int app_write_data(void *app_ptr, u16 address, u8 *data, int len)
{
    /*
    Writes a number of bytes to memory
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> Write data(%d) addr: %hX", len, address);

    // Special-case of 1 byte
    if (len == 1) {
        result = link_st(LINK(app), address, data[0]);
        if (result) {
            _loginfo_i("link_st16 failed %d", result);
            return -2;
        }
    }

    // Range check
    if (len > UPDI_MAX_REPEAT_SIZE + 1) {
        _loginfo_i("Write data length out of size %d", len);
        return -3;
    }

    // Store the address
    result = link_st_ptr(LINK(app), address);
    if (result) {
        _loginfo_i("link_st_ptr failed %d", result);
        return -4;
    }

    //Fire up the repeat
    if (len > 1) {
        result = link_repeat(LINK(app), len);
        if (result) {
            _loginfo_i("link_repeat failed %d", result);
            return -5;
        }
    }

    result = link_st_ptr_inc(LINK(app), data, len);
    if (result) {
        _loginfo_i("link_st_ptr_inc16 failed %d", result);
        return -6;
    }

    return 0;
}

int _app_write_nvm(void *app_ptr, u16 address, u8 *data, int len, u8 nvm_command, bool use_word_access)
{
    /*
        Writes a page of data to NVM.
        By default the PAGE_WRITE command is used, which requires that the page is already erased.
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> Chip write nvm");

    // Check that NVM controller is ready
    result = app_wait_flash_ready(app, 1000);
    if (result) {
        _loginfo_i("app_wait_flash_ready timeout before page buffer clear failed %d", result);
        return -2;
    }

    //Clear the page buffer
    _loginfo_i("Clear page buffer");
    result = app_execute_nvm_command(app, UPDI_NVMCTRL_CTRLA_PAGE_BUFFER_CLR);
    if (result) {
        _loginfo_i("app_execute_nvm_command failed %d", UPDI_NVMCTRL_CTRLA_PAGE_BUFFER_CLR, result);
        return -3;
    }

    // Waif for NVM controller to be ready
    result = app_wait_flash_ready(app, 1000);
    if (result) {
        _loginfo_i("app_wait_flash_ready timeout after page buffer clear failed %d", result);
        return -4;
    }

    // Load the page buffer by writing directly to location
    if (use_word_access)
        result = app_write_data_words(app, address, data, len);
    else
        result = app_write_data(app, address, data, len);
    if (result) {
        _loginfo_i("app_write_data(%d) failed %d", use_word_access, result);
        return -5;
    }

    // Write the page to NVM, maybe erase first
    _loginfo_i("Committing page");
    result = app_execute_nvm_command(app, nvm_command);
    if (result) {
        _loginfo_i("app_execute_nvm_command(%d) failed %d", nvm_command, result);
        return -6;
    }

    // Waif for NVM controller to be ready again
    result = app_wait_flash_ready(app, 1000);
    if (result) {
        _loginfo_i("app_wait_flash_ready timeout after page write failed %d", result);
        return -7;
    }

    return 0;
}


int app_write_nvm(void *app_ptr, u16 address, u8 *data, int len)
{
    bool word_mode = true;

    if (len & 0x1)
        word_mode = false;

    return _app_write_nvm(app_ptr, address, data, len, UPDI_NVMCTRL_CTRLA_WRITE_PAGE, word_mode);
}

int app_read_data(void *app_ptr, u16 address, u8 *data, int len)
{
    /*
    Reads a number of bytes of data from UPDI
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> Read data(%d) addr: %hX", len, address);

    // Range check
    if (len > UPDI_MAX_REPEAT_SIZE + 1) {
        _loginfo_i("Read data length out of size %d", len);
        return -2;
    }

    // Store the address
    result = link_st_ptr(LINK(app), address);
    if (result) {
        _loginfo_i("link_st_ptr failed %d", result);
        return -3;
    }

    //Fire up the repeat
    if (len > 1) {
        result = link_repeat(LINK(app), len);
        if (result) {
            _loginfo_i("link_repeat failed %d", result);
            return -4;
        }
    }

    //Do the read(s)
    result = link_ld_ptr_inc(LINK(app), data, len);
    if (result) {
        _loginfo_i("link_ld_ptr_inc failed %d", result);
        return -5;
    }

    return 0;
}

int app_read_data_words(void *app_ptr, u16 address, u8 *data, int len)
{
    /*
    Reads a number of words of data from UPDI
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> Read words data(%d) addr: %hX", len, address);

    // Range check
    if (len > (UPDI_MAX_REPEAT_SIZE >> 1) + 1) {
        _loginfo_i("Read data length out of size %d", len);
        return -2;
    }

    // Store the address
    result = link_st_ptr(LINK(app), address);
    if (result) {
        _loginfo_i("link_st_ptr failed %d", result);
        return -3;
    }

    //Fire up the repeat
    if (len > 2) {
        result = link_repeat(LINK(app), len >> 1);
        if (result) {
            _loginfo_i("link_repeat failed %d", result);
            return -4;
        }
    }

    //Do the read(s)
    result = link_ld_ptr_inc16(LINK(app), data, len);
    if (result) {
        _loginfo_i("link_ld_ptr_inc16 failed %d", result);
        return -5;
    }

    return 0;
}

int _app_read_nvm(void *app_ptr, u16 address, u8 *data, int len, bool use_word_access)
{
    /*
       Read data from NVM.
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
    int result;

    if (!VALID_APP(app))
        return ERROR_PTR;

    _loginfo_i("<APP> Chip read nvm");

    // Load to buffer by reading directly to location
    if (use_word_access)
        result = app_read_data_words(app, address, data, len);
    else
        result = app_read_data(app, address, data, len);
    if (result) {
        _loginfo_i("app_read_data(%d) failed %d", use_word_access, result);
        return -5;
    }

    return 0;
}

int app_read_nvm(void *app_ptr, u16 address, u8 *data, int len)
{
    bool word_mode = true;

    if (len & 0x1)
        word_mode = false;

    return _app_read_nvm(app_ptr, address, data, len, word_mode);
}

int app_ld(void *app_ptr, u16 address, u8* data)
{
    /*
        Pack the link_ld
    */
    upd_application_t *app = (upd_application_t *)app_ptr;
  
    return _link_ld(LINK(app), address, data);
}