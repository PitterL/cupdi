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

unsigned int calc_crc8(unsigned char *base, int end_off)
{
    unsigned char crc = 0;
    unsigned char *ptr = base;
    unsigned char *last_val = base + end_off - 1;

    while (ptr < last_val) {
        crc8(crc, *ptr);
        ptr++;
    }

    return crc;
}

void crc24(unsigned int *crc, unsigned char firstbyte, unsigned char secondbyte)
{
    static const unsigned int crcpoly = 0x80001B;
    unsigned int result;
    unsigned int data_word;

    data_word = (secondbyte << 8) | firstbyte;
    result = ((*crc << 1) ^ data_word);

    if (result & 0x1000000)
        result ^= crcpoly;

    *crc = result;
}

unsigned int calc_crc24(unsigned char *base, int end_off)
{
    unsigned int crc = 0;
    unsigned char *ptr = base;
    unsigned char *last_val = base + end_off - 1;

    while (ptr < last_val) {
        crc24(&crc, *ptr, *(ptr + 1));
        ptr += 2;
    }

    /* if len is odd, fill the last byte with 0 */
    if (ptr == last_val)
        crc24(&crc, *ptr, 0);

    /* Mask to 24-bit */
    crc &= 0x00FFFFFF;

    return crc;
}