#include	<conio.h>
#include	<stdio.h>
#include	<math.h>
#include	<bios.h>
#include	<time.h>

#include	"SerDefs.h"
#include	"defs.h"
#include 	"popup.h"

void plasicur(void)
{
	float			latitude_now;
	char 			c;

	plazero();
	window(statuswind);
	gotoxy(STAT_STAT.x,STAT_STAT.y);
	FLASH;
	clreol();
	cprintf("Sicurezza in corso...");
	NOFLASH;

	SerCmdGotoEnc(CNT_LATD,(long)-MAXLATDCNT*(LATD_SIC-LATD_OFFSET)/360,5);
	while(!SerGetCmdResult());

	do
	{
		latitude_now=(float)-motc.declinazione/MAXLATDCNT*360+LATD_OFFSET;
	}	while(fabs(latitude_now-LATD_SIC)>0.15 && !kbhit());

	c='\0';
	if(kbhit())
		if((c=getch())==ESC)
		{
			SerCmdGotoEnc(CNT_LATD,motc.declinazione,10);	//STOP!!
			while(!SerGetCmdResult());
		}

	beep_low();
	beep_high();
	gotoxy(STAT_STAT.x,STAT_STAT.y);
	clreol();
}