#include <unistd.h>

//delay millisecond here
void msleep(int ms)
{
    usleep(ms * 1000);
}