#ifndef __UD_NVM_H
#define __UD_NVM_H

void *updi_nvm_init(const char *port, int baud, void *dev);
void updi_nvm_deinit(void *nvm_ptr);
int nvm_get_device_info(void *nvm_ptr);
int nvm_enter_progmode(void *nvm_ptr);
int nvm_leave_progmode(void *nvm_ptr);
int nvm_unlock_device(void *nvm_ptr);
int nvm_chip_erase(void *nvm_ptr);
int nvm_read_flash(void *nvm_ptr, u16 address, u8 *data, int len);
int nvm_write_flash(void *nvm_ptr, u16 address, const u8 *data, int len);
int nvm_write_eeprom(void *nvm_ptr, u16 address, const u8 *data, int len);
int nvm_write_userrow(void *nvm_ptr, u16 address, const u8 *data, int len);
int nvm_write_fuse(void *nvm_ptr, int fusenum, u8 fuseval);
int nvm_read_mem(void *nvm_ptr, u16 address, u8 *data, int len);
int nvm_write_mem(void *nvm_ptr, u16 address, const u8 *data, int len);
int nvm_write_auto(void *nvm_ptr, u16 address, const u8 *data, int len);
int nvm_get_flash_info(void *nvm_ptr, nvm_info_t *info);

#endif