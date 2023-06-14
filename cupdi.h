#ifndef __CUPDI_H
#define __CUPDI_H

int updi_erase(void *nvm_ptr);
int updi_program(void *nvm_ptr, const char *file, bool check);
int updi_compare(void *nvm_ptr, const char *file);
int updi_check(void *nvm_ptr);
int updi_update(void *nvm_ptr, const char *file, bool check);
int updi_save(void *nvm_ptr, const char *file);
int updi_dump(void *nvm_ptr, const char *file);
int updi_read(void *nvm_ptr, char *cmd);
int updi_write(void *nvm_ptr, char *cmd, bool check);
int updi_write_fuse(void *nvm_ptr, char *cmd, bool check);
int updi_page_erase(void *nvm_ptr, char *cmd);
int updi_show_ext_info(void *nvm_ptr);
int updi_show_fuse(void *nvm_ptr);
int updi_verifiy_infoblock(void *nvm_ptr);
int updi_reset(void *nvm_ptr);
int updi_debugview(void *nvm_ptr, char *cmd);
int updi_selftest(void *nvm_ptr, char *cmd);

int dev_pack_to_vcs_hex_file(const device_info_t * dev, const char *file, int pack);
int dev_vcs_hex_file_show_info(const device_info_t * dev, const char *file);


#ifdef INFO_STORAGE_EEPROM
#define INFO_BLOCK_STORAGE_TYPE   NVM_EEPROM
#define CFG_BLOCK_STORAGE_TYPE   NVM_USERROW
#else
#define INFO_BLOCK_STORAGE_TYPE	  NVM_USERROW
#define CFG_BLOCK_STORAGE_TYPE   NVM_EEPROM
#endif

#define INFO_BLOCK_ADDRESS_OFFSET   0
#define CFG_BLOCK_ADDRESS_OFFSET   0

#endif
