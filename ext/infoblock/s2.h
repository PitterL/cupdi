#ifndef __IB_S2_H
#define __IB_S2_H

#define INFO_BLOCK_S2_VER_MAJOR INFO_BLOCK_S_VER_MAJOR
#define INFO_BLOCK_S2_VER_MINOR '2'
#define INFO_BLOCK_S2_VERSION L8_TO_LT16(INFO_BLOCK_S2_VER_MAJOR, INFO_BLOCK_S2_VER_MINOR)

int ib_create_information_block_s2(information_container_t *info, int fw_crc24, int fw_size, int fw_version, int dsdr_addr);
int ib_set_infoblock_data_ptr_s2(information_container_t *info, char *data, int len, unsigned short flag);
int ib_max_block_size_s2(void);
void ib_destory_s2(void *info_ptr);

#endif