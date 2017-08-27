#include	<dos.h>

void beep_high(void)
{
	sound(1000);
	delay(100);
	nosound();
}
/******************************************************/
void beep_low(void)
{
	sound(500);
	delay(100);
	nosound();
}
