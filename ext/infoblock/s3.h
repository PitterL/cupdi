#ifndef __IB_S3_H
#define __IB_S3_H

#define INFO_BLOCK_S3_VER_MAJOR INFO_BLOCK_S_VER_MAJOR
#define INFO_BLOCK_S3_VER_MINOR '3'
#define INFO_BLOCK_S3_VERSION L8_TO_LT16(INFO_BLOCK_S3_VER_MAJOR, INFO_BLOCK_S3_VER_MINOR)

int ib_create_information_block_s3(information_container_t *info, information_content_params_t *param, int len);
int ib_set_infoblock_data_ptr_s3(information_container_t *info, char *data, int len, unsigned short flag);
int ib_max_block_size_s3(void);
void ib_destory_s3(void *info_ptr);

#endif