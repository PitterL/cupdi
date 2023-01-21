#ifndef __UD_APPLICATION_H
#define __UD_APPLICATION_H

void *updi_application_init(const char *port, int baud, int guard, const void *dev);
void updi_application_deinit(void *app_ptr);
int app_device_info(void *app_ptr);
bool app_in_prog_mode(void *app_ptr);
int app_wait_unlocked(void *app_ptr, int timeout);
int app_unlock(void *app_ptr);
int app_enter_progmode(void *app_ptr);
int app_leave_progmode(void *app_ptr);
int app_reset(void *app_ptr, bool apply_reset);
int app_disable(void *app_ptr);
int app_toggle_reset(void *app_ptr, bool reset_or_halt);
int app_wait_flash_ready(void *app_ptr, int timeout);
int app_execute_nvm_command(void *app_ptr, u8 command);
int app_chip_erase(void *app_ptr);
int app_read_data_bytes(void *app_ptr, u32 address, u8 *data, int len);
int app_read_data_words(void *app_ptr, u32 address, u8 *data, int len);
int app_read_data(void *app_ptr, u32 address, u8 *data, int len);
int app_read_nvm(void *app_ptr, u8 blockid, u32 address, u8 *data, int len);
int app_write_data_words(void *app_ptr, u32 address, const u8 *data, int len);
int app_write_data_bytes(void *app_ptr, u32 address, const u8 *data, int len);
int app_write_data(void *app_ptr, u32 address, const u8 *data, int len, bool use_word_access);
int app_write_flash(void *app_ptr, u8 blockid, u32 address, const u8 *data, int len, bool use_word_access);
int app_erase_write_flash(void *app_ptr, u8 blockid, u32 address, const u8 *data, int len, bool use_word_access);
int app_erase_write_eeprom(void *app_ptr, u32 address, const u8 *data, int len);
int app_erase_write_userrow(void *app_ptr, u32 address, const u8 *data, int len);
int app_erase_flash_page(void *app_ptr, u8 blockid, u32 address, int pages);
int app_erase_eeprom(void *app_ptr, u32 address, int size);
int app_write_fuse(void *app_ptr, u32 address, const u8 value);
int app_ld_reg(void *app_ptr, u32 address, u8* data, int len);
int app_st_reg(void *app_ptr, u32 address, const u8 *data, int len);

/*
Max waiting time at flash programming
*/
#define TIMEOUT_WAIT_FLASH_READY 1000

#endif