#include	<conio.h>
#include	"defs.h"
#include 	"SerDefs.h"

void	updatescreen(void)
{
	float prec_now_year;

	window(analogwind);
	NOFLASH;
//	textcolor(BLACK);
	gotoxy(1,1);

	cprintf("Sole        (0%-100%) =");
	SOLE_STAT.x=wherex();
	SOLE_STAT.y=wherey();
	cprintf(" %3d\n\r",stato.sole/4095);

	cprintf("Luna        (0%-100%) =");
	LUNA1_STAT.x=wherex();
	LUNA1_STAT.y=wherey();
	cprintf(" %3d\n\r",stato.luna/4095);

	cprintf("Stelle      (0%-100%) =");
	STELLE_STAT.x=wherex();
	STELLE_STAT.y=wherey();
	cprintf(" %3d\n\r",stato.stelle/2000);

	cprintf("Tramonto    (0%-100%) =");
	SSET_STAT.x=wherex();
	SSET_STAT.y=wherey();
	cprintf(" %3d\n\r",stato.tramonto/250);

	cprintf("Alba        (0%-100%) =");
	SRISE_STAT.x=wherex();
	SRISE_STAT.y=wherey();
	cprintf(" %3d\n\r",stato.alba/250);

	cprintf("Giorno      (0%-100%) =");
	DAYL_STAT.x=wherex();
	DAYL_STAT.y=wherey();
	cprintf(" %3d\n\r",stato.giorno/250);

	cprintf("\n\r\n\r\n\rFreccia a SX = Riduce il fondo cielo");
	cprintf("\n\rFreccia a DX = Aumenta il fondo cielo");
	cprintf("\n\rFreccia SU   = Aumenta la latitudine");
	cprintf("\n\rFreccia GIU' = Diminuisce la latitudine");
	cprintf("\n\r[-20,0,+20,(+45),+90]\n\r");
	cprintf("\n\rF5 = Precessione +1000 anni");
	cprintf("\n\rF6 = Precessione -1000 anni");

	window(digitalwind);
	gotoxy(1,1);
	cprintf("Tempo locale.............=");
	TIME_STAT.x=wherex();
	TIME_STAT.y=wherey();
	cprintf("  xx:xx\n\r");

	cprintf("A.R. in meridiano........=");
	AR_STAT.x=wherex();
	AR_STAT.y=wherey();
	cprintf(" %7.3f\n\r",(float)motc.ascRetta/MAXARCNT*24+AR_OFFSET);

	cprintf("Latitudine (0,+/-90).....=");
	LATD_STAT.x=wherex();
	LATD_STAT.y=wherey();
	cprintf(" %7.3f\n\r",(float)motc.declinazione/MAXLATDCNT*90+LATD_OFFSET);

	cprintf("Precessione (0-360)......=");
	PREC_STAT.x=wherex();
	PREC_STAT.y=wherey();
	cprintf(" %7.3f\n\r",(float)motc.precessione/MAXPRECCNT*360-PREC_OFFSET);

	cprintf("Anno (PRECESSIONE).......=");
	YEAR_STAT.x=wherex();
	YEAR_STAT.y=wherey();
	prec_now_year=((float)-motc.precessione/MAXPRECCNT*360+PREC_OFFSET)*0.014+2000;	//riferito all'anno 2000
	cprintf(" %7.3f\n\r",prec_now_year);
}
