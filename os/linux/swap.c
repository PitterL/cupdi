#include "swap.h"

/*! Byte swap short, 'tiny' chip is little endian, if not match of OS cpu, switch it*/

int16_t swap_int16(int16_t val)
{
#if (BYTE_ORDER == BIG_ENDIAN)
    return (val << 8) | ((val >> 8) & 0xFF);
#else
    return val;
#endif
}

int32_t swap_int32(int32_t val)
{
#if (BYTE_ORDER == BIG_ENDIAN)
    return ((val >> 24) & 0xff) | // move byte 3 to byte 0
        ((val << 8) & 0xff0000) | // move byte 1 to byte 2
        ((val >> 8) & 0xff00) | // move byte 2 to byte 1
        ((val << 24) & 0xff000000); // byte 0 to byte 3
#else
    return val;
#endif
}