#ifndef __UDPI_DEVICE
#define __UDPI_DEVICE

typedef struct _nvm_info
{
    unsigned int nvm_start;
    unsigned int nvm_size;
    unsigned short nvm_pagesize;
    unsigned int nvm_blocksize;
    unsigned short nvm_mapped_start;
    unsigned char nvm_magicoff;
} nvm_info_t;

typedef struct _sys_info
{
    unsigned short syscfg_address;
    unsigned short nvmctrl_address;
    unsigned short sigrow_address;
} sys_info_t;

typedef struct {
    /* crc page size */
    unsigned short page_size;

    /* append of append in fuse offset */
    unsigned char append;

    /* append of bootend in fuse offset */
    unsigned char bootend;
} crc_src_t;

enum { UPDIPIN_CFG_GPIO = 0, UPDIPIN_CFG_UPDI, UPDIPIN_CFG_RESET, UPDIPIN_CFG_RSV };
typedef uint8_t UPDI_PINCFG_T;
typedef struct {
    unsigned char field;
    unsigned char bitshift;
    unsigned char bitmask;
} updi_pincfg_t;

typedef enum {
    TINY41x,
    TINY81x,
    TINY161x,
    TINY321x,
    AVRDA,
    AVRDU,
} DEV_TYPE_T;

#define MAGIC_ID_FUSE 0x82
#define MAGIC_ID_EEPROM 0x81
#define MAGIC_ID_USER 0x85
#define MAGIC_ID_BOOTROW 0x86
#define MAGIC_ID_LOCKBITS 0x83

typedef enum _NVM_TYPE
{
    NVM_FLASH,
    NVM_FUSES,
    NVM_USERROW,
    NVM_EEPROM,
    MEM_SRAM,
    NUM_NVM_TYPES
} NVM_TYPE_T;

typedef enum _NVM_TYPE_EX
{
    NVM_LOCKBITS = NUM_NVM_TYPES,
    NVM_BOOTROW,
    NUM_NVM_EX_TYPES
} NVM_TYPE_EX_T;

typedef struct _chip_info
{
    const char *dev_name;
    nvm_info_t nvms[NUM_NVM_EX_TYPES];
    sys_info_t reg;
    crc_src_t crc;
    updi_pincfg_t updipin;
} chip_mem_t;

typedef struct _device_info
{
    const char *name;
    DEV_TYPE_T type;
    const chip_mem_t *mmap;
} device_info_t;

const device_info_t *get_chip_info(const char *dev_name);
int dev_get_nvm_info(const void *dev, NVM_TYPE_EX_T type, nvm_info_t *inf);
int dev_get_nvm_info_ext(const void *dev_ptr, NVM_TYPE_EX_T type, nvm_info_t *info, const char **pname);
int dev_get_crc_info(const void *dev_ptr, crc_src_t *src);
int dev_get_updi_pincfg_info(const void *dev_ptr, updi_pincfg_t *src);

#endif
