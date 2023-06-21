#ifndef __CB_C1_H
#define __CB_C1_H

#define CONFIG_BLOCK_C1_VER_MAJOR CONFIG_BLOCK_C_VER_MAJOR
#define CONFIG_BLOCK_C1_VER_MINOR '1'
#define CONFIG_BLOCK_C1_VERSION L8_TO_LT16(CONFIG_BLOCK_C1_VER_MAJOR, CONFIG_BLOCK_C1_VER_MINOR)

typedef uint16_t s_elem_t;

#pragma pack(1)
typedef struct {
    // key count
    s_elem_t count;
    //  Cap value Low limit, unit 1/10 pf
    s_elem_t siglo;
    // Cap value High limit, unit 1/10 pf
    s_elem_t sighi;
    // reference error with stand base line (with digital gain 1x)
    s_elem_t range;
} signal_limit_t;
#pragma pack()
enum { SIGLIM_CNT, SIGLIM_SIGLO, SIGLIM_SIGHI, SIGLIM_RANGE, NUM_SIGLIM_TYPES };

typedef union {
    signal_limit_t limit;
    s_elem_t val[NUM_SIGLIM_TYPES];
} signal_limit_data_t;

typedef signal_limit_data_t config_body_elem_c1_t;

typedef struct {
    config_body_elem_c1_t one;
} config_body_c1_t;

int cb_create_configure_block_c1(config_container_t *info, char *data, int len);
int cb_set_configure_block_ptr_c1(config_container_t *info, char *data, int len, unsigned short flag);
int cb_max_block_size_c1(void);
void cb_destory_c1(void *info_ptr);
#endif