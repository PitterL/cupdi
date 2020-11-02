#ifndef __SWAP_H
#define __SWAP_H

#ifdef CUPDI

#include <stdint.h>

int16_t _swap_int16(int16_t val);
int16_t lt_int16_to_cpu(int16_t val);
int32_t _swap_int32(int32_t val);
int32_t lt_int32_to_cup(int32_t val);

#endif

#endif