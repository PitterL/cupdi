
#include "crc.h"

/*
calculate one byte input value with CRC 8 Bit
    @crc: last crc value
    @data: data input
    @returns calculated crc value
*/
uint8_t __crc8(uint8_t crc, uint8_t data)
{
    static const uint8_t crcpoly = 0x8C;
    uint8_t index;
    uint8_t fb;
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
uint8_t calc_crc8(const void *data_ptr, size_t size)
{
    const uint8_t *data = (const uint8_t *)data_ptr;
    uint8_t crc = 0;
    const uint8_t *ptr = data;
    const uint8_t *last_val = data + size - 1;

    while (ptr <= last_val) {
        crc = __crc8(crc, *ptr);
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
uint32_t __crc24(uint32_t crc, uint8_t firstbyte, uint8_t secondbyte)
{
    static const uint32_t crcpoly = 0x80001B;
    uint32_t data_word;

    data_word = ((uint16_t)secondbyte << 8) | firstbyte;
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
uint32_t calc_crc24(const void *data_ptr, size_t len, uint32_t crc) {
    const uint8_t *data = (const uint8_t *)data_ptr;
    const uint8_t *end = data + len;  // 指向末尾后一个字节
    const uint8_t *ptr = data;

    if (len == 0) return crc;  // 处理 len=0

    // 每次处理 2 字节，确保 ptr + 1 < end
    while (ptr + 1 < end) {
        crc = __crc24(crc, *ptr, *(ptr + 1));
        ptr += 2;
    }

    // 处理剩余 1 字节（如果有）
    if (ptr < end) {
        crc = __crc24(crc, *ptr, 0);
    }

    return crc & 0x00FFFFFF;
}

// CRC-16-CCITT 算法实现 (Big end)
uint16_t calc_crc16(const void *data_ptr, size_t length, uint16_t crc) {
    const uint8_t *data = (const uint8_t *)data_ptr;
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