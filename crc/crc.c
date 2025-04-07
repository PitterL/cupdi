
#include "crc.h"

/*
calculate one byte input value with CRC 8 Bit
    @crc: last crc value
    @data: data input
    @returns calculated crc value
*/
unsigned char crc8(unsigned char crc, unsigned char data)
{
    static const unsigned char crcpoly = 0x8C;
    unsigned char index;
    unsigned char fb;
    index = 8;
    
    do
    {
        fb = (crc ^ data) & 0x01;
        data >>= 1;
        crc >>= 1;
        if (fb)
            crc ^= crcpoly;
    } while (--index);

    return crc;
}

/*
Calculate buffer with crc8
    @base: buffer input
    @size: data size
    @returns calculated crc value
*/
unsigned char calc_crc8(const unsigned char *base, int size)
{
    unsigned char crc = 0;
    const unsigned char *ptr = base;
    const unsigned char *last_val = base + size - 1;

    while (ptr <= last_val) {
        crc = crc8(crc, *ptr);
        ptr++;
    }

    return crc;
}

/*
calculate two byte input value with CRC 24 Bit
    @crc: last crc value
    @firstbyte: byte 1
    @secondbyte: byte 2
    @returns calculated crc value
*/
unsigned int crc24(unsigned int crc, unsigned char firstbyte, unsigned char secondbyte)
{
    static const unsigned int crcpoly = 0x80001B;
    unsigned int data_word;

    data_word = ((unsigned short)secondbyte << 8) | firstbyte;
    crc = ((crc << 1) ^ data_word);

    if (crc & 0x1000000)
        crc ^= crcpoly;

    return crc;
}

/*
Calculate buffer with crc24
    @base: buffer input
    @size: data size
    @returns calculated crc value, only bit[0~23] is valid
*/
unsigned int calc_crc24(const unsigned char *base, int size, uint32_t crc)
{
    const unsigned char *ptr = base;
    const unsigned char *last_val = base + size - 1;

    while (ptr < last_val) {
        crc = crc24(crc, *ptr, *(ptr + 1));
        ptr += 2;
    }

    /* if len is odd, fill the last byte with 0 */
    if (ptr == last_val)
        crc = crc24(crc, *ptr, 0);

    /* Mask to 24-bit */
    crc &= 0x00FFFFFF;

    return crc;
}

// CRC-16-CCITT 算法实现
uint16_t crc16_ccitt(const uint8_t *data, size_t length, uint16_t crc) {
    uint16_t polynomial = 0x1021; // 多项式 x^16 + x^12 + x^5 + 1

    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8; // 将当前字节与 CRC 的高字节异或

        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ polynomial; // 如果最高位为 1，左移并与多项式异或
            }
            else {
                crc <<= 1; // 否则直接左移
            }
        }
    }

    return crc;
}