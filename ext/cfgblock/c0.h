#ifndef __CB_C0_H
#define __CB_C0_H

#define CONFIG_BLOCK_C0_VER_MAJOR CONFIG_BLOCK_C_VER_MAJOR
#define CONFIG_BLOCK_C0_VER_MINOR '0'
#define CONFIG_BLOCK_C0_VERSION L8_TO_LT16(CONFIG_BLOCK_C0_VER_MAJOR, CONFIG_BLOCK_C0_VER_MINOR)

typedef unsigned char config_body_elem_c0_t;

typedef struct {
    config_body_elem_c0_t one;
} config_body_c0_t;

int cb_create_configure_block_c0(config_container_t *info, char *data, int len);
int cb_set_configure_block_ptr_c0(config_container_t *info, char *data, int len, unsigned short flag);
int cb_max_block_size_c0(void);
void cb_destory_c0(void *info_ptr);

#endif