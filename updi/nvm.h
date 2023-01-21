#ifndef __UD_NVM_H
#define __UD_NVM_H

void *updi_nvm_init(const char *port, int baud, int guard, const void *dev);
void updi_nvm_deinit(void *nvm_ptr);
int nvm_get_device_info(void *nvm_ptr);
int nvm_enter_progmode(void *nvm_ptr);
int nvm_leave_progmode(void *nvm_ptr);
int nvm_disable(void *nvm_ptr);
int nvm_unlock_device(void *nvm_ptr);
int nvm_chip_erase(void *nvm_ptr);
int nvm_read_flash(void *nvm_ptr, u32 address, u8 *data, int len);
int nvm_write_flash(void *nvm_ptr, u32 address, const u8 *data, int len, bool erased);
int nvm_erase_flash_page(void *nvm_ptr, u32 address, int count);
int nvm_read_eeprom(void *nvm_ptr, u32 address, u8 *data, int len);
int nvm_write_eeprom(void *nvm_ptr, u32 address, const u8 *data, int len, bool dummy);
int nvm_read_userrow(void *nvm_ptr, u32 address, u8 *data, int len);
int nvm_write_userrow(void *nvm_ptr, u32 address, const u8 *data, int len, bool dummy);
int nvm_read_fuse(void *nvm_ptr, u32 address, u8 *data, int len);
int nvm_write_fuse(void *nvm_ptr, u32 address, const u8 *data, int len, bool dummy);
int nvm_read_mem(void *nvm_ptr, u32 address, u8 *data, int len);
int nvm_write_mem(void *nvm_ptr, u32 address, const u8 *data, int len, bool dummy);

typedef int(*nvm_wop)(void *nvm_ptr, u32 address, const u8 *data, int len, bool /* some action */);
typedef int(*nvm_rop)(void *nvm_ptr, u32 address, u8 *data, int len);
typedef int(*nvm_eop)(void *nvm_ptr, u32 address, int count);

int nvm_read_auto(void *nvm_ptr, u32 address, u8 *data, int len);
int nvm_write_auto(void *nvm_ptr, u32 address, const u8 *data, int len, bool verify);
int nvm_erase_auto(void *nvm_ptr, u32 address, int count);
int nvm_reset(void *nvm_ptr, int delay_ms);
int nvm_halt(void *nvm_ptr);
int nvm_wait(void *nvm_ptr);
int nvm_get_block_info(void *nvm_ptr, /*NVM_TYPE_T*/int type, nvm_info_t *info);
int nvm_get_block_info_ext(void *nvm_ptr, int type, nvm_info_t *info, char **pname);
void *nvm_get_content(void *nvm_ptr, int type, int *req_size);

/*
Max waiting time for chip reset
*/
#define TIMEOUT_WAIT_CHIP_RESET 50

/* 
UPDI Max Transfer size
*/
#define UPDI_MAX_TRANSFER_SIZE (UPDI_MAX_REPEAT_SIZE + 1)
#endif