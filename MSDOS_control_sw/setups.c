#include	<conio.h>
#include	"defs.h"

void setupscreen(void)
{
	window(mainwind);
	NOFLASH;
	_setcursortype(_NOCURSOR);
//	textbackground(WHITE);
	clrscr();
	window(functwind);
//	textcolor(BLACK);
	clrscr();
	cprintf("F1-");
	PIANETI_STAT.x=wherex();
	PIANETI_STAT.y=wherey();
	cprintf("Pianeti OFF");
	gotoxy(17,1);
	cprintf("F2-");
	CERCHI_STAT.x=wherex();
	CERCHI_STAT.y=wherey();
	cprintf("Cerchi OFF");
	gotoxy(32,1);
	cprintf("F3-");
	POLO_STAT.x=wherex();
	POLO_STAT.y=wherey();
	cprintf("Polo OFF");
	gotoxy(45,1);
	cprintf("F4-");
	LUNA_STAT.x=wherex();
	LUNA_STAT.y=wherey();
	cprintf("Luna OFF");
	gotoxy(70,1);
	cprintf("F10-EXIT");
	gotoxy(1,2);
	cprintf("Spazio-");
	RUN_STAT.x=wherex();
	RUN_STAT.y=wherey();
	cprintf("Sim RUN ");

	window(titlewind);
	clrscr();
	cprintf("CANALI");
	gotoxy(40,1);
	cprintf(" COORDINATE");

	window(mainwind);
	gotoxy(1,2);
	cprintf("--------------------------------------------------------------------------------");

	window(statuswind);
	clrscr();
	cprintf("STATUS: ");
	STAT_STAT.x=wherex();
	STAT_STAT.y=wherey();
}
