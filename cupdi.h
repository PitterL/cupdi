#ifndef __CUPDI_H
#define __CUPDI_H

int updi_erase(void *nvm_ptr);
int updi_flash(void *nvm_ptr, const char *file, int flag);
int updi_save(void *nvm_ptr, const char *file);
int updi_read(void *nvm_ptr, char *cmd);
int updi_write(void *nvm_ptr, char *cmd);
int updi_write_fuse(void *nvm_ptr, char *cmd);
int updi_read_infoblock(void *nvm_ptr);
int updi_verify_infoblock(void *nvm_ptr);
int updi_reset(void *nvm_ptr);
int updi_debugview(void *nvm_ptr, char *cmd);

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
#define INFO_BLOCK_SIZE 16
#define INFO_BLOCK_ADDRESS_IN_EEPROM 0

#endif
