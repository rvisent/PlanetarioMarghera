#include <stdio.h>
#include <conio.h>
#include <dos.h>

#define MOUSE 0x33

int mousebt(void)
{
	 union REGS regs;

	 regs.x.ax=0x03;  /* get mouse buttons */
	 int86(MOUSE, &regs, &regs);
	 return(regs.x.bx);
}
