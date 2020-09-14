#ifdef CUPDI

#include "hal_delay.h"

//delay millisecond here
void msleep(int ms)
{
    /*usleep(ms * 1000);*/
	delay_ms(ms);
}

#endif