#ifndef __CUPDI_H
#define __CUPDI_H

#ifdef CUPDI
int cupdi_operate();
int updi_erase(void *nvm_ptr);
int updi_write_fuse(void *nvm_ptr);
int updi_program(void *nvm_ptr);
//int updi_reset(void *nvm_ptr);
#endif

#endif
