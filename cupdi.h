#ifndef __CUPDI_H
#define __CUPDI_H

int updi_erase(void *nvm_ptr);
int updi_fuse(void *nvm_ptr, char *fuses);
int updi_flash(void *nvm_ptr, const char *file);
void unload_hex(hex_info_t *ihex);

#endif
