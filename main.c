#include <atmel_start.h>
#include "cupdi/cupdi.h"

int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();

#ifdef CUPDI
	cupdi_operate();
#endif

	/* Replace with your application code */
	while (1) {
	}
}
