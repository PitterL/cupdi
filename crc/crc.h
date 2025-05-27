#ifndef _CRC_H
#define _CRC_H

#include <stdint.h>

uint8_t calc_crc8(const void *data_ptr, size_t size);

#define CRC_CRC24_INIT 0x0
uint32_t calc_crc24(const void *data_ptr, size_t size, uint32_t crc);

#define CRC_CRC16_CCITT_INIT 0xFFFF
uint16_t calc_crc16(const void *data_ptr, size_t length, uint16_t crc);

#endif