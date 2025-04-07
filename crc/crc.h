#ifndef _CRC_H
#define _CRC_H

#include <stdint.h>

unsigned char calc_crc8(const unsigned char *base, int size);

#define CRC_CRC24_INIT 0x0
unsigned int calc_crc24(const unsigned char *base, int size, uint32_t crc);

#define CRC_CRC16_CCITT_INIT 0xFFFF
uint16_t crc16_ccitt(const uint8_t *data, size_t length, uint16_t crc);


#endif