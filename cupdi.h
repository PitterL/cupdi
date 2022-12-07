#ifndef __CUPDI_H
#define __CUPDI_H

int updi_erase(void *nvm_ptr);
int updi_program(void *nvm_ptr, const char *file, bool check);
int updi_compare(void *nvm_ptr, const char *file);
int updi_verifiy_infoblock(void *nvm_ptr);
int updi_update(void *nvm_ptr, const char *file, bool check);
int updi_save(void *nvm_ptr, const char *file);
int updi_dump(void *nvm_ptr, const char *file);
int updi_read(void *nvm_ptr, char *cmd);
int updi_write(void *nvm_ptr, char *cmd, bool check);
int updi_write_fuse(void *nvm_ptr, char *cmd, bool check);
int updi_page_erase(void *nvm_ptr, char *cmd);
int updi_show_infoblock(void *nvm_ptr);
int updi_show_fuse(void *nvm_ptr);
int updi_verifiy_infoblock(void *nvm_ptr);
int updi_reset(void *nvm_ptr);
int updi_debugview(void *nvm_ptr, char *cmd);

int dev_pack_to_vcs_hex_file(const device_info_t * dev, const char *file);
int dev_vcs_hex_file_show_info(const device_info_t * dev, const char *file);

/*
InfoBlock:  This is firmware infomation this store in eeprom, each time firmware updated, the infoblock is created
size:	INFO_BLOCK_SIZE(16 bytes)
address:    0 address at eeprom
Layout:
0-3:	Fw ver[0]		Fw ver[1]		Fw ver[1]		Build number
4-7:	Fw size[0:7]	Fw size[8:15]	Fw size[16:23]	Fw size[24:31]
8~11:	Rsv		        Rsv		        Rsv		        Rsv
12-15:	Fw crc[0:7]	    Fw crc[8:15]	Fw crc[16:23]	Info crc[0:7]
*/

/*
Info block length
*/
#define INFO_BLOCK_STORAGE_TYPE   /* NVM_EEPROM */ NVM_USERROW
#define INFO_BLOCK_ADDRESS_OFFSET   0


#define CFG_BLOCK_STORAGE_TYPE   NVM_EEPROM

#endif
