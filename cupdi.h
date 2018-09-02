#ifndef __CUPDI_H
#define __CUPDI_H

int updi_erase(void *nvm_ptr);
int updi_fuse(void *nvm_ptr, char *fuses);
int updi_flash(void *nvm_ptr, const char *file, bool prog);
int updi_save(void *nvm_ptr, const char *file);

#endif
