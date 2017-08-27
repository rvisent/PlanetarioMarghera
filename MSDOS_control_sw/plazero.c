#include	<conio.h>
#include	<stdio.h>
#include	<math.h>
#include	<bios.h>
#include	<time.h>

#include	"SerDefs.h"
#include	"defs.h"
#include 	"popup.h"

void plazero(void)
{
	window(statuswind);
	gotoxy(STAT_STAT.x,STAT_STAT.y);
	FLASH;
	clreol();
	cprintf("Azzeramento in corso...");
	NOFLASH;

	window(statuswind);
	gotoxy(STAT_STAT.x,STAT_STAT.y);
	motc.bSearchZeroDecl = motc.bSearchZeroAscR = motc.bSearchZeroPrec = FALSE;

	if (!SerCmdZeroEncSync(CNT_LATD, 5, 15))
		cprintf("\r\nNAK1(latitude) ");
	if (!SerCmdZeroEncSync(CNT_AR, 150, 15))
		cprintf("\r\nNAK1(ascr) ");
	if (!SerCmdZeroEncSync(CNT_PREC, 150, 15))
		cprintf("\r\nNAK1(prec) ");

	window(statuswind);
	gotoxy(STAT_STAT.x,STAT_STAT.y);
	FLASH;
	clreol();
	cprintf("Azzeramento in corso...");
	NOFLASH;
	while(motc.bSearchZeroDecl || motc.bSearchZeroAscR || motc.bSearchZeroPrec)
	{
		if (kbhit())
		{
			getch();
			gotoxy(STAT_STAT.x,STAT_STAT.y);
			cprintf("Ricerca zero interrotta");
			//STOP a bassa velocita'
			SerCmdGotoEnc(CNT_AR,(long)-10,10);
			while(!SerGetCmdResult());
			SerCmdGotoEnc(CNT_LATD,(long)-5,2);
			while(!SerGetCmdResult());
			SerCmdGotoEnc(CNT_PREC,(long)-10,10);
			while(!SerGetCmdResult());
			return;
		}
	}

	// fase 2
	if (!SerCmdZeroEncSync(CNT_LATD, 1, 5))
		cprintf("NAK2(latitude) ");
	if (!SerCmdZeroEncSync(CNT_AR, 50, 5))
		cprintf("NAK2(ascr) ");
	if (!SerCmdZeroEncSync(CNT_PREC, 50, 15))
		cprintf("NAK2(prec) ");

	while(motc.bSearchZeroDecl || motc.bSearchZeroAscR || motc.bSearchZeroPrec)
	{
		if (kbhit())
		{
			getch();
			gotoxy(STAT_STAT.x,STAT_STAT.y);
			clreol();
			cprintf("ricerca zero interrotta");
			//STOP a bassa velocita'
			SerCmdGotoEnc(CNT_AR,(long)-10,10);
			while(!SerGetCmdResult());
			SerCmdGotoEnc(CNT_LATD,(long)-5,2);
			while(!SerGetCmdResult());
			SerCmdGotoEnc(CNT_PREC,(long)-10,10);
			while(!SerGetCmdResult());
			return;
		}
	}
	beep_low();
	beep_high();
	gotoxy(STAT_STAT.x,STAT_STAT.y);
	clreol();
}