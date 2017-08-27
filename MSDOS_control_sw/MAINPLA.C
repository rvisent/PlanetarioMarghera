//modifica del 20gen2006 val max stelle in setup
//10MAR2006: eliminato il controllo con il mouse

//24NOV2013: modifiche per nuove transizioni a LED

#define		TEST 0
#define		SPAZIO_RUN_STOP	0
#define		MAIN
#define   MAXSOLE 4000

#include	<conio.h>
#include	<stdio.h>
#include	<math.h>
#include	<bios.h>
#include	<time.h>

#include	"SerDefs.h"
#include	"defs.h"
#include 	"popup.h"

/******************************************************/
BOOL bCmdPending=FALSE;	// no comandi in attesa di risposta
// prototipi locali
void SendMessage(char c);
void Update();
/******************************************************/

void main()
{
	float			ar_now,ar_nowpr,latitude_now,prec_now,prec_now_year,tfloat;
	int				thr,tmin,TCounter,mstat,keypressed,EvtCnt=0,hindex,decl_idx=3;
	long int	EncToGo,PrecToGo;
	BOOL			onekeywait=FALSE,increase=FALSE,ENDPGM=FALSE,ENDCHOICE=FALSE,
						POSOK=FALSE,mouse_ldown=FALSE,first_run=TRUE,stop=FALSE,
						mon_blank=FALSE;
	clock_t 	start;
	char			c,*buffer;
	const int hrtogo[12][2]={{6,18},{7,17},{8,16},{8,16},{8,17},{7,17},{6,18},
													 {5,19},{4,19},{5,20},{6,19},{6,18}};
	const int decl_steps[6]={0,10,20,45,70,90};

	sess_rec(SYSON);
	buffer=malloc(80*25*2);		//spazio di buffer per il video (BLANK)

#if(!TEST)
	if (!SerInit())
	{
		printf("ERRORE DI SISTEMA: interruzioni non inizializate correttamente\n");
		return;						//errore di configurazione delle interruzioni
	}
#endif

	init_popup();

	if(read_setup())
	{
		printf("ERRORE: file di configurazione non presente o errato\n");
		return;						//errore nella lettura del file di configurazione
	}
	hindex=(int)((sun_ar+0.5)/2);
	if(hindex==12)
		hindex=0;

	clrscr();						//attiva i colori impostati
	setupscreen();
	updatescreen();

	stato.declinazione=OFF;
	stato.cerchio_polare=OFF;
	stato.sole=0;
	stato.stelle=0;
	stato.giorno=250;
	stato.p_cardinali=OFF;
	stato.pianeti=OFF;
	stato.luna=0;
	delay(500);			//aspetta che il comando sia eseguito

#if(!TEST)
	plazero();			//azzera la macchina
#endif

	next_popup(15,5,62,12,WINPUP);
	NOFLASH;
	cprintf("  P L A N E T A R I O   D I   M A R G H E R A\r\n"
					"              Versione 2.0-20131128\r\n");
	FLASH;
	cprintf("                Cosa vuoi fare?\n\n\r");
	NOFLASH;
	cprintf("               F1)  IMPOSTAZIONI\n\r"
					"               F2)  UTILIZZO\n\r"
					"               F3)  AZZERAMENTO\n\r"
					"               F4)  SICUREZZA\n\r"
					"               ESC) FINE");

	while(!ENDPGM)
	{
		if(_bios_keybrd(_KEYBRD_READY))
		{
			keypressed=_bios_keybrd(_KEYBRD_READ);
			switch(keypressed)
			{
				case F1:	//Impostazioni
					next_popup(20,6,56,15,WINPUP);
					FLASH;
					cprintf("      I M P O S T A Z I O N I\r\n");
					NOFLASH;
					cprintf("\r\n"
									"       F1)  Imposta il Sole\n\r"
									"       F2)  Imposta la Luna\n\r"
									"       F3)  Imposta Mercurio\n\r"
									"       F4)  Imposta Venere\n\r"
									"       F5)  Imposta Marte\n\r"
									"       F6)  Imposta Giove\n\r"
									"       F7)  Imposta Saturno\n\r"
									"       ESC) Uscita");
					stato.declinazione=ON;
					stato.cerchio_polare=ON;
//					stato.sole=4095;
					stato.sole=MAXSOLE;
					stato.stelle=500;
					stato.giorno=15;
					stato.p_cardinali=ON;
					stato.pianeti=ON;
					stato.luna=4095;
					ENDCHOICE=FALSE;
					while(!ENDCHOICE)
					{
						if(_bios_keybrd(_KEYBRD_READY))
						{
							keypressed=_bios_keybrd(_KEYBRD_READ);
							switch(keypressed)
							{
								case F1:
									next_popup(13,4,51,6,WINPUP);
									FLASH;
									cprintf("              S O L E\r\n");
									cprintf("            A.R.=%5.2f\n\r",d2hms(sun_ar));
									cprintf("  Premi un tasto quando hai terminato");
#if(!TEST)
									SerCmdGotoEnc(CNT_AR,(long)-MAXARCNT*(sun_ar-AR_OFFSET)/24,100);
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_LATD,(long)-MAXLATDCNT*(latitude-LATD_OFFSET)/360,5);
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_PREC,(long)-MAXPRECCNT*(PREC_OFFSET)/360,100);
									while(!SerGetCmdResult());
#endif
									while((fabs(ar_now-sun_ar)>0.15 || fabs(latitude_now-latitude)>0.15
										|| fabs(prec_now)>5) && !kbhit())
									{
										ar_now=(float)-motc.ascRetta/MAXARCNT*24+AR_OFFSET;
										latitude_now=(float)-motc.declinazione/MAXLATDCNT*360+LATD_OFFSET;
										prec_now=(float)motc.precessione/MAXPRECCNT*360+PREC_OFFSET;
									}
									prev_popup();
								break;

								case F2:
									next_popup(13,4,51,6,WINPUP);
									FLASH;
									cprintf("              L U N A\r\n");
									cprintf("            A.R.=%5.2f\n\r",d2hms(moon_ar));
									cprintf("  Premi un tasto quando hai terminato");
#if(!TEST)
									SerCmdGotoEnc(CNT_AR,(long)-MAXARCNT*(moon_ar-AR_OFFSET)/24,100);
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_LATD,(long)-MAXLATDCNT*(latitude-LATD_OFFSET)/360,5);
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_PREC,(long)-MAXPRECCNT*(PREC_OFFSET)/360,100);
									while(!SerGetCmdResult());
#endif
									while((fabs(ar_now-moon_ar)>0.15 || fabs(latitude_now-latitude)>0.15
										|| fabs(prec_now)>5) && !kbhit())
									{
										ar_now=(float)-motc.ascRetta/MAXARCNT*24+AR_OFFSET;
										latitude_now=(float)-motc.declinazione/MAXLATDCNT*360+LATD_OFFSET;
										prec_now=(float)motc.precessione/MAXPRECCNT*360+PREC_OFFSET;
									}
									prev_popup();
								break;

								case F3:
									next_popup(13,4,51,6,WINPUP);
									FLASH;
									cprintf("            M E R C U R I O\r\n");
									cprintf("            A.R.=%5.2f\n\r",d2hms(mercury_ar));
									cprintf("  Premi un tasto quando hai terminato");
#if(!TEST)
									SerCmdGotoEnc(CNT_AR,(long)-MAXARCNT*(mercury_ar-AR_OFFSET)/24,100);
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_LATD,(long)-MAXLATDCNT*(latitude-LATD_OFFSET)/360,5);
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_PREC,(long)-MAXPRECCNT*(PREC_OFFSET)/360,100);
									while(!SerGetCmdResult());
#endif
									while((fabs(ar_now-mercury_ar)>0.15 || fabs(latitude_now-latitude)>0.15
										|| fabs(prec_now)>5) && !kbhit())
									{
										ar_now=(float)-motc.ascRetta/MAXARCNT*24+AR_OFFSET;
										latitude_now=(float)-motc.declinazione/MAXLATDCNT*360+LATD_OFFSET;
										prec_now=(float)motc.precessione/MAXPRECCNT*360+PREC_OFFSET;
									}
									prev_popup();
								break;

								case F4:
									next_popup(13,4,51,6,WINPUP);
									FLASH;
									cprintf("             V E N E R E\r\n");
									cprintf("            A.R.=%5.2f\n\r",d2hms(venus_ar));
									cprintf("  Premi un tasto quando hai terminato");
#if(!TEST)
									SerCmdGotoEnc(CNT_AR,(long)-MAXARCNT*(venus_ar-AR_OFFSET)/24,100);
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_LATD,(long)-MAXLATDCNT*(latitude-LATD_OFFSET)/360,5);
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_PREC,(long)-MAXPRECCNT*(PREC_OFFSET)/360,100);
									while(!SerGetCmdResult());
#endif
									while((fabs(ar_now-venus_ar)>0.15 || fabs(latitude_now-latitude)>0.15
										|| fabs(prec_now)>5) && !kbhit())
									{
										ar_now=(float)-motc.ascRetta/MAXARCNT*24+AR_OFFSET;
										latitude_now=(float)-motc.declinazione/MAXLATDCNT*360+LATD_OFFSET;
										prec_now=(float)motc.precessione/MAXPRECCNT*360+PREC_OFFSET;
									}
									prev_popup();
								break;

								case F5:
									next_popup(13,4,51,6,WINPUP);
									FLASH;
									cprintf("             M A R T E\r\n");
									cprintf("            A.R.=%5.2f\n\r",d2hms(mars_ar));
									cprintf("  Premi un tasto quando hai terminato");
#if(!TEST)
									SerCmdGotoEnc(CNT_AR,(long)-MAXARCNT*(mars_ar-AR_OFFSET)/24,100);
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_LATD,(long)-MAXLATDCNT*(latitude-LATD_OFFSET)/360,5);
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_PREC,(long)-MAXPRECCNT*(PREC_OFFSET)/360,100);
									while(!SerGetCmdResult());
#endif
									while((fabs(ar_now-mars_ar)>0.15 || fabs(latitude_now-latitude)>0.15
										|| fabs(prec_now)>5) && !kbhit())
									{
										ar_now=(float)-motc.ascRetta/MAXARCNT*24+AR_OFFSET;
										latitude_now=(float)-motc.declinazione/MAXLATDCNT*360+LATD_OFFSET;
										prec_now=(float)motc.precessione/MAXPRECCNT*360+PREC_OFFSET;
									}
									prev_popup();
								break;

								case F6:
									next_popup(13,4,51,6,WINPUP);
									FLASH;
									cprintf("             G I O V E\r\n");
									cprintf("            A.R.=%5.2f\n\r",d2hms(jupiter_ar));
									cprintf("  Premi un tasto quando hai terminato");
#if(!TEST)
									SerCmdGotoEnc(CNT_AR,(long)-MAXARCNT*(jupiter_ar-AR_OFFSET)/24,100);
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_LATD,(long)-MAXLATDCNT*(latitude-LATD_OFFSET)/360,5);
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_PREC,(long)-MAXPRECCNT*(PREC_OFFSET)/360,100);
									while(!SerGetCmdResult());
#endif
									while((fabs(ar_now-jupiter_ar)>0.15 || fabs(latitude_now-latitude)>0.15
										|| fabs(prec_now)>5) && !kbhit())
									{
										ar_now=(float)-motc.ascRetta/MAXARCNT*24+AR_OFFSET;
										latitude_now=(float)-motc.declinazione/MAXLATDCNT*360+LATD_OFFSET;
										prec_now=(float)motc.precessione/MAXPRECCNT*360+PREC_OFFSET;
									}
									prev_popup();
								break;

								case F7:
									next_popup(13,4,51,6,WINPUP);
									FLASH;
									cprintf("          S A T U R N O\r\n");
									cprintf("            A.R.=%5.2f\n\r",d2hms(saturn_ar));
									cprintf("  Premi un tasto quando hai terminato");
#if(!TEST)
									SerCmdGotoEnc(CNT_AR,(long)-MAXARCNT*(saturn_ar-AR_OFFSET)/24,100);
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_LATD,(long)-MAXLATDCNT*(latitude-LATD_OFFSET)/360,5);
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_PREC,(long)-MAXPRECCNT*(PREC_OFFSET)/360,100);
									while(!SerGetCmdResult());
#endif
									while((fabs(ar_now-saturn_ar)>0.15 || fabs(latitude_now-latitude)>0.15
										|| fabs(prec_now)>5) && !kbhit())
									{
										ar_now=(float)-motc.ascRetta/MAXARCNT*24+AR_OFFSET;
										latitude_now=(float)-motc.declinazione/MAXLATDCNT*360+LATD_OFFSET;
										prec_now=(float)motc.precessione/MAXPRECCNT*360+PREC_OFFSET;
									}
									prev_popup();
								break;

								case XESC:
									ENDCHOICE=TRUE;
									stato.declinazione=OFF;
									stato.cerchio_polare=OFF;
									stato.sole=0;
									stato.stelle=0;
									stato.giorno=250;
									stato.p_cardinali=OFF;
									stato.pianeti=OFF;
									stato.luna=0;
									delay(500);			//aspetta che il comando sia eseguito
								break;
							}
						}
					}
					prev_popup();
				break;

				case F2:	//Utilizzo
					if(!first_run)
					{
						motc.bSearchZeroAscR=FALSE;
						if (!SerCmdZeroEncSync(CNT_AR, 150, 5))
							cprintf("\r\nNAK(ascr) ");

						window(statuswind);
						gotoxy(STAT_STAT.x,STAT_STAT.y);
						FLASH;
						clreol();
						cprintf("Azzeramento in corso...");
						NOFLASH;
						while(motc.bSearchZeroAscR)
							if (kbhit())
							{
								getch();
								gotoxy(STAT_STAT.x,STAT_STAT.y);
								cprintf("Ricerca zero interrotta");
								//STOP a bassa velocita'
								SerCmdGotoEnc(CNT_AR,(long)-10,10);
								while(!SerGetCmdResult());
								return;
							}
					}
					first_run=FALSE;
					prev_popup(); //spegne finestra opzioni principali
					window(statuswind);
					gotoxy(STAT_STAT.x,STAT_STAT.y);
					clreol();
					FLASH;
					cprintf("Posizionamento in corso...");
					beep_low();
					beep_high();
					ar_now=AR_OFFSET;
					CalcTime(ar_now,sun_ar,&thr,&tmin,&tfloat);
					window(digitalwind);
					gotoxy(TIME_STAT.x,TIME_STAT.y);
					NOFLASH;
					clreol();
					cprintf("  %2d:%2d",thr,tmin);

#if(!TEST)
					EncToGo=(long)-MAXARCNT*(sun_ar-AR_OFFSET)/24;
					SerCmdGotoEnc(CNT_AR,EncToGo,100);
					while(!SerGetCmdResult());
					SerCmdGotoEnc(CNT_LATD,(long)-MAXLATDCNT*(latitude-LATD_OFFSET)/360,5);
					while(!SerGetCmdResult());
					SerCmdGotoEnc(CNT_PREC,(long)-MAXPRECCNT*(PREC_OFFSET)/360,100);
					while(!SerGetCmdResult());

					window(digitalwind);
					ar_now=(float)-motc.ascRetta/MAXARCNT*24+AR_OFFSET;
					latitude_now=(float)-motc.declinazione/MAXLATDCNT*360+LATD_OFFSET;
					prec_now=(float)motc.precessione/MAXPRECCNT*360+PREC_OFFSET;
					prec_now_year=prec_now*72.22+2000;	//riferito all'anno 2000
					stato.declinazione=OFF;
					stato.cerchio_polare=OFF;
					stato.sole=0;//4095; non mostrare il sole durante il posizionamento
					stato.stelle=0;
					stato.giorno=250;
					stato.p_cardinali=ON;
					stato.pianeti=OFF;
					stato.luna=0;
					delay(500);			//aspetta che il comando sia eseguito
					while(fabs(ar_now-sun_ar)>0.15 || fabs(latitude_now-latitude)>0.15
								|| fabs(prec_now)>5) 										//GRAZIE MAX
					{
						ar_now=(float)-motc.ascRetta/MAXARCNT*24+AR_OFFSET;
						if(ar_now>24)
							ar_now-=24;
						CalcTime(ar_now,sun_ar,&thr,&tmin,&tfloat);
						gotoxy(AR_STAT.x,AR_STAT.y);
						ar_nowpr=d2hms(ar_now);
						if(!mon_blank)
							cprintf(" %6.2f",ar_nowpr);
						gotoxy(LATD_STAT.x,LATD_STAT.y);
						latitude_now=(float)-motc.declinazione/MAXLATDCNT*360+LATD_OFFSET;
						if(!mon_blank)
							cprintf(" %6.2f",latitude_now);
						gotoxy(TIME_STAT.x,TIME_STAT.y);
						if(!mon_blank)
							cprintf("  %2d:%2d",thr,tmin);
						prec_now=(float)motc.precessione/MAXPRECCNT*360+PREC_OFFSET;
						prec_now_year=prec_now*72.22+2000;	//riferito all'anno 2000
						gotoxy(PREC_STAT.x,PREC_STAT.y);
						if(!mon_blank)
							cprintf(" %6.2f",prec_now);
						gotoxy(YEAR_STAT.x,YEAR_STAT.y);
						if(!mon_blank)
							cprintf(" %6.2f",prec_now_year);

						c='\0';
						if(kbhit())
						{
							if((c=getch())==ESC)
							{
								SerCmdGotoEnc(CNT_AR,motc.ascRetta,10);	//STOP!!
								while(!SerGetCmdResult());
								SerCmdGotoEnc(CNT_LATD,motc.declinazione,10);	//STOP!!
								while(!SerGetCmdResult());
								SerCmdGotoEnc(CNT_PREC,motc.precessione,10);	//STOP!!
								while(!SerGetCmdResult());
								goto rientro_da_errore;
							}
						}
					}
#endif

					beep_low();
					beep_high();
//					stato.sole=4095;	//adesso accendi il Sole!
					stato.sole=MAXSOLE;	//adesso accendi il Sole! 21APR2006
					window(statuswind);
					gotoxy(STAT_STAT.x,STAT_STAT.y);
					FLASH;
					clreol();
					if(!mon_blank)
						cprintf("Premi SPAZIO per iniziare...");
					c='\0';
					while(c!=' ')
						if(kbhit())
							c=getch();

					gotoxy(STAT_STAT.x,STAT_STAT.y);
					clreol();
					if(!mon_blank)
						cprintf("Simulazione in corso...");

					beep_low();
					beep_high();

				//-------------------------------------------------
				//fino al tramonto!
#if(!TEST)
					EncToGo=(long)((float)-MAXARCNT*((sun_ar-AR_OFFSET+(float)hrtogo[hindex][HTOSUNSET])/24.0));
					SerCmdGotoEnc(CNT_AR,EncToGo,100);
					while(!SerGetCmdResult());
#endif
					window(digitalwind);
					NOFLASH;
					delay(100);
					gotoxy(AR_STAT.x,AR_STAT.y);
					clreol();
					gotoxy(TIME_STAT.x,TIME_STAT.y);
					clreol();

#if(!TEST)
					POSOK=FALSE;
					while(!POSOK)
					{
						gotoxy(AR_STAT.x,AR_STAT.y);
						ar_now=(float)-motc.ascRetta/MAXARCNT*24+AR_OFFSET;
						if(ar_now>24)
							ar_now-=24;
						CalcTime(ar_now,sun_ar,&thr,&tmin,&tfloat);
						ar_nowpr=d2hms(ar_now);
						if(!mon_blank)
							cprintf(" %6.2f",ar_nowpr);
						gotoxy(TIME_STAT.x,TIME_STAT.y);
						if(!mon_blank)
							cprintf("  %2d:%2d",thr,tmin);
						prec_now=(float)motc.precessione/MAXPRECCNT*360+PREC_OFFSET;
						prec_now_year=prec_now*72.22+2000;	//riferito all'anno 2000
						gotoxy(PREC_STAT.x,PREC_STAT.y);
						if(!mon_blank)
							cprintf(" %6.2f",prec_now);
						gotoxy(YEAR_STAT.x,YEAR_STAT.y);
						if(!mon_blank)
							cprintf(" %6.2f",prec_now_year);
//ho aumentato la soglia perche' non veniva riconosciuta (era 200)
//10GIU2001
//						if(labs(EncToGo-motc.ascRetta)<1000)
						if(labs(EncToGo-motc.ascRetta)<3000)	//MODIFICATO IL 30DIC2005
						{
							if(EvtCnt==3)
							{
								POSOK=TRUE;
								EvtCnt=0;
							}
							else
								EvtCnt++;
						}
						else
							EvtCnt=0;

						c='\0';
						if(kbhit())
						{
							if((c=getch())==ESC)
							{
								SerCmdGotoEnc(CNT_AR,motc.ascRetta,10);	//STOP!!
								while(!SerGetCmdResult());
								SerCmdGotoEnc(CNT_LATD,motc.declinazione,10);	//STOP!!
								while(!SerGetCmdResult());
								SerCmdGotoEnc(CNT_PREC,motc.precessione,10);	//STOP!!
								while(!SerGetCmdResult());
								goto rientro_da_errore;
							}
						}
					}
#endif
				//---------------------
				//SEQUENZA DEL TRAMONTO (rivista dopo l'installazione dei LED - 20131124)
				//---------------------
					window(statuswind);
					gotoxy(STAT_STAT.x,STAT_STAT.y);
					FLASH;
					clreol();
					if(!mon_blank)
						cprintf("Sequenza TRAMONTO...");

					TCounter=0;						//decimi di secondo di simulazione
					while(TCounter<TSIM) 			//secondi max = TSIM*0.01
					{
						//Il Sole si spegne in un quinto del tempo di transizione (TSIM)
						if(TCounter<=TSIM/5.0)
							stato.sole=MAXSOLE-(TCounter*5.0/TSIM)*MAXSOLE;

						//La luce del cielo si attenua impiegando tutto il tempo della transizione (TSIM)
						stato.giorno=250.0-TCounter*250.0/TSIM;
						
						//imposta il valore minimo del fondo cielo
						if(stato.giorno<horlight)
							stato.giorno=horlight;
						
						//--------------------
						//progressione della luce del tramonto
						// La luce del tramonto inizia da subito e raggiunge il massimo ad 1/4 del tempo
						//	rimane al massimo fino a metà del tempo e poi diminuisce fino a zero alla fine
						//	del tempo di transizione
						if(TCounter<(TSIM/4.0))
							stato.tramonto=(TCounter*4.0/TSIM)*250;

						if(TCounter>TSIM/2.0)
							stato.tramonto=250-((TCounter-TSIM/2.0)*2.0/TSIM)*250;
						//--------------------
						
						if(TCounter>(TSIM/2.0))
							stato.stelle=((TCounter-TSIM/2.0)*2.0/TSIM)*maxstars;
						
						//------------------------------------------------
						//STAMPA DEI VALORI A SCHERMO
						//------------------------------------------------
						window(analogwind);
						NOFLASH;
						gotoxy(SOLE_STAT.x,SOLE_STAT.y);
						if(!mon_blank)
//							cprintf("%4d",(int)(stato.sole*100.0/4095));
							cprintf("%4d",(int)(stato.sole*100.0/(MAXSOLE-1)));
						gotoxy(STELLE_STAT.x,STELLE_STAT.y);
						if(!mon_blank)
							cprintf("%4d",(int)(stato.stelle*100.0/1900));
						gotoxy(SSET_STAT.x,SSET_STAT.y);
						if(!mon_blank)
							cprintf("%4d",(int)(stato.tramonto*100.0/250));
						gotoxy(DAYL_STAT.x,DAYL_STAT.y);
						if(!mon_blank)
							cprintf("%4d",(int)(stato.giorno*100.0/250));

						TCounter++;
						delay(10);
					}
					sess_rec(STARSON);
					window(statuswind);
					gotoxy(STAT_STAT.x,STAT_STAT.y);
					FLASH;
					clreol();

					beep_low();
					beep_high();

					if(!mon_blank)
						cprintf("Premi SPAZIO per continuare...");
					c='\0';
					while(c!=' ' && c!=ESC)
						if(kbhit())
							c=getch();

					if(c==ESC)
					{
						SerCmdGotoEnc(CNT_AR,motc.ascRetta,10);	//STOP!!
						while(!SerGetCmdResult());
						SerCmdGotoEnc(CNT_LATD,motc.declinazione,10);	//STOP!!
						while(!SerGetCmdResult());
						SerCmdGotoEnc(CNT_PREC,motc.precessione,10);	//STOP!!
						while(!SerGetCmdResult());
						goto rientro_da_errore;
					}

					gotoxy(STAT_STAT.x,STAT_STAT.y);
					clreol();
					if(!mon_blank)
						cprintf("Notte...");

					beep_low();
					beep_high();
				//-------------------------------------------------
				//fino all'alba
				//-------------------------------------------------
#if(!TEST)
					CicloNotte:		//ci arriva evitando l'alba
					EncToGo=(long)((float)-MAXARCNT*((sun_ar-AR_OFFSET+(float)hrtogo[hindex][HTOSUNRISE])/24.0));
					SerCmdGotoEnc(CNT_AR,EncToGo,speed);
					while(!SerGetCmdResult());
#endif

					mstat=RUN;
					onekeywait=FALSE;

					while(mstat!=END)
					{
						if(_bios_keybrd(_KEYBRD_READY))
						{
							onekeywait=TRUE;
							keypressed=_bios_keybrd(_KEYBRD_READ);
						}

						switch(mstat)
						{
							case	RUN:
								if(onekeywait && keypressed==SPACEBAR)
								{
									onekeywait=FALSE;
#if(!TEST)
									SerCmdGotoEnc(CNT_AR,motc.ascRetta,speed);
									while(!SerGetCmdResult());
#endif
									window(functwind);
									gotoxy(RUN_STAT.x,RUN_STAT.y);
									REVERSE;
									if(!mon_blank)
										cprintf("Sim STOP");
									mstat=STOP;
								}
							break;
							case	STOP:
								if(onekeywait && keypressed==SPACEBAR)
								{
									onekeywait=FALSE;
#if(!TEST)
									SerCmdGotoEnc(CNT_AR,EncToGo,speed);
									while(!SerGetCmdResult());
#endif
									window(functwind);
									gotoxy(RUN_STAT.x,RUN_STAT.y);
									NOFLASH;
									if(!mon_blank)
										cprintf("Sim RUN ");
									window(statuswind);
									FLASH;
									gotoxy(STAT_STAT.x,STAT_STAT.y);
									clreol();
									if(!mon_blank)
										cprintf("Notte...");
									mstat=RUN;
								}
							break;
						}

						switch(mon_blank)
						{
							case	FALSE:
								if(onekeywait && keypressed==SLETTER)
								{
									onekeywait=FALSE;
									window(mainwind);
									gettext(mainwind,buffer);
									textbackground(BLACK);
									textcolor(BLACK);
									clrscr();					//spegne tutto!
									mon_blank=TRUE;
								}
							break;
							case	TRUE:
								if(onekeywait && keypressed==SLETTER)
								{
									onekeywait=FALSE;
									window(mainwind);
									puttext(mainwind,buffer);	//RIACCENDI!
									mon_blank=FALSE;
								}
							break;
						}

						if(mstat==STOP && onekeywait)
						{
							switch(keypressed)
							{
								case F1:	//pianeti
									if(stato.pianeti==OFF)
									{
										stato.pianeti=ON;
										window(functwind);
										gotoxy(PIANETI_STAT.x,PIANETI_STAT.y);
										REVERSE;
										if(!mon_blank)
											cprintf("Pianeti ON ");
									}
									else
									{
										stato.pianeti=OFF;
										window(functwind);
										gotoxy(PIANETI_STAT.x,PIANETI_STAT.y);
										NOFLASH;
										if(!mon_blank)
											cprintf("Pianeti OFF");
									}
									onekeywait=FALSE;
								break;

								case F2:	//cerchi di latitudine e AR
									if(stato.declinazione==OFF)
									{
										stato.declinazione=ON;
										window(functwind);
										gotoxy(CERCHI_STAT.x,CERCHI_STAT.y);
										REVERSE;
										if(!mon_blank)
											cprintf("Cerchi ON ");
									}
									else
									{
										stato.declinazione=OFF;
										window(functwind);
										gotoxy(CERCHI_STAT.x,CERCHI_STAT.y);
										NOFLASH;
										if(!mon_blank)
											cprintf("Cerchi OFF");
									}
									onekeywait=FALSE;
								break;

								case F3:	//cerchio polare
									if(stato.cerchio_polare==OFF)
									{
										stato.cerchio_polare=ON;
										window(functwind);
										gotoxy(POLO_STAT.x,POLO_STAT.y);
										REVERSE;
										if(!mon_blank)
											cprintf("Polo ON ");
									}
									else
									{
										stato.cerchio_polare=OFF;
										window(functwind);
										gotoxy(POLO_STAT.x,POLO_STAT.y);
										NOFLASH;
										if(!mon_blank)
											cprintf("Polo OFF");
									}
									onekeywait=FALSE;
								break;

								case F4:	//Luna
									start=clock();
									if(!increase)
									{
										increase=TRUE;
										window(functwind);
										gotoxy(LUNA_STAT.x,LUNA_STAT.y);
										REVERSE;
										if(!mon_blank)
											cprintf("Luna ON ");
									}
									else
									{
										increase=FALSE;
										window(functwind);
										gotoxy(LUNA_STAT.x,LUNA_STAT.y);
										NOFLASH;
										if(!mon_blank)
											cprintf("Luna OFF");
									}
									onekeywait=FALSE;
								break;

								case F5:	//precessione +1000 anni
									window(statuswind);
									gotoxy(STAT_STAT.x,STAT_STAT.y);
									clreol();
									FLASH;
									if(!mon_blank)
										cprintf("Posizionamento in corso...");
									PrecToGo=(long)(motc.precessione+THSNDYR);
									SerCmdGotoEnc(CNT_PREC,(long)PrecToGo,100);
									while(!SerGetCmdResult());
									window(digitalwind);
									NOFLASH;
									while(labs(PrecToGo-motc.precessione)>20)
									{
										prec_now=(float)motc.precessione/MAXPRECCNT*360+PREC_OFFSET;
										prec_now_year=prec_now*72.22+2000;	//riferito all'anno 2000
										gotoxy(PREC_STAT.x,PREC_STAT.y);
										if(!mon_blank)
											cprintf(" %6.2f",prec_now);
										gotoxy(YEAR_STAT.x,YEAR_STAT.y);
										if(!mon_blank)
											cprintf(" %6.2f",prec_now_year);
									}
									window(statuswind);
									gotoxy(STAT_STAT.x,STAT_STAT.y);
									clreol();
									onekeywait=FALSE;
								break;

								case F6:	//precessione -1000 anni
									window(statuswind);
									gotoxy(STAT_STAT.x,STAT_STAT.y);
									clreol();
									FLASH;
									if(!mon_blank)
										cprintf("Posizionamento in corso...");
									PrecToGo=(long)(motc.precessione-THSNDYR);
									SerCmdGotoEnc(CNT_PREC,(long)PrecToGo,100);
									while(!SerGetCmdResult());
									window(digitalwind);
									NOFLASH;
									while(labs(PrecToGo-motc.precessione)>20)
									{
										prec_now=(float)motc.precessione/MAXPRECCNT*360+PREC_OFFSET;
										prec_now_year=prec_now*72.22+2000;	//riferito all'anno 2000
										gotoxy(PREC_STAT.x,PREC_STAT.y);
										if(!mon_blank)
											cprintf(" %6.2f",prec_now);
										gotoxy(YEAR_STAT.x,YEAR_STAT.y);
										if(!mon_blank)
											cprintf(" %6.2f",prec_now_year);
									}
									window(statuswind);
									gotoxy(STAT_STAT.x,STAT_STAT.y);
									clreol();
									onekeywait=FALSE;
								break;

								case UPARW:	//DECLINAZIONE, uno step in su
									window(statuswind);
									gotoxy(STAT_STAT.x,STAT_STAT.y);
									clreol();
									FLASH;
									if(!mon_blank)
										cprintf("Posizionamento in corso...");
									decl_idx++;
									if(decl_idx>5)
										decl_idx=5;
									else
									{
										SerCmdGotoEnc(CNT_LATD,(long)-MAXLATDCNT*
													(decl_steps[decl_idx]-LATD_OFFSET)/360,5);
										while(!SerGetCmdResult());
										window(digitalwind);
										NOFLASH;
										stop=FALSE;
										while(fabs(latitude_now-decl_steps[decl_idx])>0.15 && !stop)
										{
											if(kbhit())
											{
												if((c=getch())==ESC)
												{
													SerCmdGotoEnc(CNT_AR,motc.ascRetta,10);	//STOP!!
													while(!SerGetCmdResult());
													SerCmdGotoEnc(CNT_LATD,motc.declinazione,10);	//STOP!!
													while(!SerGetCmdResult());
													SerCmdGotoEnc(CNT_PREC,motc.precessione,10);	//STOP!!
													while(!SerGetCmdResult());
													stop=TRUE;
												}
											}
											gotoxy(LATD_STAT.x,LATD_STAT.y);
											latitude_now=(float)-motc.declinazione/MAXLATDCNT*360+LATD_OFFSET;
											if(!mon_blank)
												cprintf(" %6.2f",latitude_now);
										}
									}
									window(statuswind);
									gotoxy(STAT_STAT.x,STAT_STAT.y);
									clreol();
									onekeywait=FALSE;
								break;

								case DNARW:	//DECLINAZIONE, uno step in giu'
									window(statuswind);
									gotoxy(STAT_STAT.x,STAT_STAT.y);
									clreol();
									FLASH;
									if(!mon_blank)
										cprintf("Posizionamento in corso...");
									decl_idx--;
									if(decl_idx<0)
										decl_idx=0;
									else
									{
										SerCmdGotoEnc(CNT_LATD,(long)-MAXLATDCNT*
													(decl_steps[decl_idx]-LATD_OFFSET)/360,5);
										while(!SerGetCmdResult());
										window(digitalwind);
										NOFLASH;
										stop=FALSE;
										while(fabs(latitude_now-decl_steps[decl_idx])>0.15 && !stop)
										{
											if(kbhit())
											{
												if((c=getch())==ESC)
												{
													SerCmdGotoEnc(CNT_AR,motc.ascRetta,10);	//STOP!!
													while(!SerGetCmdResult());
													SerCmdGotoEnc(CNT_LATD,motc.declinazione,10);	//STOP!!
													while(!SerGetCmdResult());
													SerCmdGotoEnc(CNT_PREC,motc.precessione,10);	//STOP!!
													while(!SerGetCmdResult());
													stop=TRUE;
												}
											}
											gotoxy(LATD_STAT.x,LATD_STAT.y);
											latitude_now=(float)-motc.declinazione/MAXLATDCNT*360+LATD_OFFSET;
											if(!mon_blank)
												cprintf(" %6.2f",latitude_now);
										}
									}
									window(statuswind);
									gotoxy(STAT_STAT.x,STAT_STAT.y);
									clreol();
									onekeywait=FALSE;
								break;

								case LFARW:	//Cielo + buio
									if(stato.giorno>5)
									{
										stato.giorno--;
										window(analogwind);
										NOFLASH;
										gotoxy(DAYL_STAT.x,DAYL_STAT.y);
										if(!mon_blank)
											cprintf("%4d",(int)(stato.giorno*100.0/250));
									}
									onekeywait=FALSE;
								break;

								case RGARW:	//Cielo - buio
									if(stato.giorno<250)
									{
										stato.giorno++;
										window(analogwind);
										NOFLASH;
										gotoxy(DAYL_STAT.x,DAYL_STAT.y);
										if(!mon_blank)
											cprintf("%4d",(int)(stato.giorno*100.0/250));
									}
									onekeywait=FALSE;
								break;

								case F10: //XESC:	//EMERGENZA
#if(!TEST)
									SerCmdGotoEnc(CNT_AR,motc.ascRetta,10);	//STOP!!
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_LATD,motc.declinazione,10);	//STOP!!
									while(!SerGetCmdResult());
									SerCmdGotoEnc(CNT_PREC,motc.precessione,10);	//STOP!!
									while(!SerGetCmdResult());
#endif
									mstat=END;
									onekeywait=FALSE;
								break;
							}
						}

//se non prendo questa precauzione, possono verificarsi alcuni problemi
//perch� l'aggiornamento delle variabili � asincrono

//ho aumentato la soglia perche' non veniva riconosciuta (era 200)
//10GIU2001
//						if(labs(EncToGo-motc.ascRetta)<1000)
						if(labs(EncToGo-motc.ascRetta)<3000)	//30DIC2005
						{                                       //occorre che la
							if(EvtCnt==3)													//condizione si sia
							{                                   	//verificata almeno
								EvtCnt=0;														//tre volte
								mstat=END;
							}
							else
								EvtCnt++;
						}
							else
								EvtCnt=0;

						if((clock()-start)>1)
						{
							if(increase && stato.luna<4000)
								stato.luna+=40;

							if(!increase && stato.luna>=50)
								stato.luna-=40;
							start=clock();
						}

						window(analogwind);
						NOFLASH;
						gotoxy(LUNA1_STAT.x,LUNA1_STAT.y);
						if(!mon_blank)
							cprintf("%4d",(int)(stato.luna*100.0/4095));

						window(digitalwind);
						NOFLASH;
						gotoxy(AR_STAT.x,AR_STAT.y);
						ar_now=(float)-motc.ascRetta/MAXARCNT*24+AR_OFFSET;
						if(ar_now>24)
							ar_now-=24;
						CalcTime(ar_now,sun_ar,&thr,&tmin,&tfloat);
						ar_nowpr=d2hms(ar_now);
						if(!mon_blank)
							cprintf(" %6.2f",ar_nowpr);
						gotoxy(TIME_STAT.x,TIME_STAT.y);
						if(!mon_blank)
							cprintf("  %2d:%2d",thr,tmin);
						prec_now=(float)motc.precessione/MAXPRECCNT*360+PREC_OFFSET;
						prec_now_year=prec_now*72.22+2000;	//riferito all'anno 2000
						gotoxy(PREC_STAT.x,PREC_STAT.y);
						if(!mon_blank)
							cprintf(" %6.2f",prec_now);
						gotoxy(YEAR_STAT.x,YEAR_STAT.y);
						if(!mon_blank)
							cprintf(" %6.2f",prec_now_year);
					}
					window(statuswind);
					gotoxy(STAT_STAT.x,STAT_STAT.y);
					FLASH;
					clreol();
					sess_rec(STARSOFF);
					beep_low();
					beep_high();
					mon_blank=FALSE;		//mostra il menu incondizionatamente
					if(!mon_blank)
						cprintf("Premi SPAZIO per l'alba...");
					c='\0';
					while(c!=' ' && c!=ESC)
						if(kbhit())
							c=getch();
					beep_low();
					beep_high();

					if(c==ESC)
					{
						SerCmdGotoEnc(CNT_AR,motc.ascRetta,10);	//STOP!!
						while(!SerGetCmdResult());
						SerCmdGotoEnc(CNT_LATD,motc.declinazione,10);	//STOP!!
						while(!SerGetCmdResult());
						SerCmdGotoEnc(CNT_PREC,motc.precessione,10);	//STOP!!
						while(!SerGetCmdResult());
						goto rientro_da_errore;
					}

				//---------------------
				//SEQUENZA DELL'ALBA
				//---------------------
					window(statuswind);
					gotoxy(STAT_STAT.x,STAT_STAT.y);
					FLASH;
					clreol();
					if(!mon_blank)
						cprintf("Sequenza ALBA...");

					TCounter=0;						//decimi di secondo di simulazione
					while(TCounter<TSIM) 	//10 secondi max TSIM=1000
					{
						if(TCounter>=TSIM/2)
//							stato.sole=(TCounter*2.0/TSIM-1)*4095;
							stato.sole=(TCounter*2.0/TSIM-1)*(MAXSOLE-1);

						stato.giorno=TCounter*245.0/TSIM+5;

						if(stato.giorno<horlight)
							stato.giorno=horlight;

						if(TCounter<TSIM/2)
							stato.alba=(int)((TCounter*2.0/TSIM)*245.0+5);
							else
							stato.alba=(int)(250.0-(TCounter*2.0/TSIM-1)*245.0);

						stato.stelle=(int)(maxstars-(float)TCounter*maxstars/TSIM);

						window(analogwind);
						NOFLASH;
						gotoxy(SOLE_STAT.x,SOLE_STAT.y);
						if(!mon_blank)
//							cprintf("%4d",(int)(stato.sole*100.0/4095));
							cprintf("%4d",(int)(stato.sole*100.0/(MAXSOLE-1)));
						gotoxy(STELLE_STAT.x,STELLE_STAT.y);
						if(!mon_blank)
							cprintf("%4d",(int)(stato.stelle*100.0/1900));
						gotoxy(SRISE_STAT.x,SRISE_STAT.y);
						if(!mon_blank)
							cprintf("%4d",(int)(stato.alba*100.0/250));
						gotoxy(DAYL_STAT.x,DAYL_STAT.y);
						if(!mon_blank)
							cprintf("%4d",(int)(stato.giorno*100.0/250));

						TCounter++;
						delay(10);
					}

//A questo punto spegni tutte le uscite ON/OFF
//(10GIU2001)

				stato.declinazione=OFF;
				stato.cerchio_polare=OFF;
				stato.pianeti=OFF;
				stato.luna=0;

				//vai alla fine
					window(statuswind);
					gotoxy(STAT_STAT.x,STAT_STAT.y);
					FLASH;
					clreol();
					if(!mon_blank)
						cprintf("Posizionamento finale...");

#if(!TEST)
					EncToGo=(long)((float)-MAXARCNT*((float)(sun_ar-AR_OFFSET+HTOEND)/24.0));
					SerCmdGotoEnc(CNT_AR,EncToGo,100);
					while(!SerGetCmdResult());
#endif
					window(digitalwind);
					NOFLASH;
					POSOK=FALSE;
					while(!POSOK)
					{
						gotoxy(AR_STAT.x,AR_STAT.y);
						ar_now=(float)-motc.ascRetta/MAXARCNT*24+AR_OFFSET;
						if(ar_now>24)
							ar_now-=24;
						CalcTime(ar_now,sun_ar,&thr,&tmin,&tfloat);
						ar_nowpr=d2hms(ar_now);
						if(!mon_blank)
							cprintf(" %6.2f",ar_nowpr);
						gotoxy(TIME_STAT.x,TIME_STAT.y);
						if(!mon_blank)
							cprintf("  %2d:%2d",thr,tmin);
//ho aumentato la soglia perche' non veniva riconosciuta (era 100)
//10GIU2001
//						if(labs(EncToGo-motc.ascRetta)<1000)
						if(labs(EncToGo-motc.ascRetta)<3000)		//30DIC2005
						{
							if(EvtCnt==3)
							{
								POSOK=TRUE;
								EvtCnt=0;
							}
							else
								EvtCnt++;
						}
						else
							EvtCnt=0;

						c='\0';
						if(kbhit())
						{
							if((c=getch())==ESC)
							{
								SerCmdGotoEnc(CNT_AR,motc.ascRetta,10);	//STOP!!
								while(!SerGetCmdResult());
								SerCmdGotoEnc(CNT_LATD,motc.declinazione,10);	//STOP!!
								while(!SerGetCmdResult());
								SerCmdGotoEnc(CNT_PREC,motc.precessione,10);	//STOP!!
								while(!SerGetCmdResult());
								goto rientro_da_errore;
							}
						}
					}

rientro_da_errore:					//ETICHETTA DI GESTIONE DELL'ERRORE!!

					window(statuswind);
					gotoxy(STAT_STAT.x,STAT_STAT.y);
					clreol();
					FLASH;

					if(!mon_blank)
						cprintf("Premi SPAZIO per terminare...");
					beep_high();
					beep_low();
					c='\0';
					while(c!=' ' && !(mousebt()&0x02))
						if(kbhit())
							c=getch();
					beep_high();
					beep_low();
					stato.declinazione=OFF;
					stato.cerchio_polare=OFF;
					stato.sole=0;
					stato.stelle=0;
					stato.giorno=250;
					stato.p_cardinali=ON;
					stato.pianeti=OFF;
					stato.luna=0;
					delay(500);			//aspetta che il comando sia eseguito
					NOFLASH;
					window(statuswind);
					gotoxy(STAT_STAT.x,STAT_STAT.y);
					clreol();
					next_popup(15,5,62,12,WINPUP);
					NOFLASH;
					cprintf("  P L A N E T A R I O   D I   M A R G H E R A\r\n"
									"              Versione 2.0-20131128\r\n");
					FLASH;
					cprintf("                Cosa vuoi fare?\n\n\r");
					NOFLASH;
					cprintf("               F1)  IMPOSTAZIONI\n\r"
									"               F2)  UTILIZZO\n\r"
									"               F3)  AZZERAMENTO\n\r"
									"               F4)  SICUREZZA\n\r"
									"               ESC) FINE");
				break;
				case F3:	//azzeramento del planetario
					stato.declinazione=OFF;
					stato.cerchio_polare=OFF;
					stato.sole=0;
					stato.stelle=0;
					stato.giorno=250;
					stato.alba=5;
					stato.tramonto=5;
					stato.p_cardinali=OFF;
					stato.pianeti=OFF;
					stato.luna=0;
					delay(500);			//aspetta che il comando sia eseguito
					beep_high();
					beep_low();
#if(!TEST)
					plazero();			//azzera la macchina
#endif
				break;

				case F4:	//planetario in posizione di sicurezza
					stato.declinazione=OFF;
					stato.cerchio_polare=OFF;
					stato.sole=0;
					stato.stelle=0;
					stato.giorno=250;
					stato.alba=5;
					stato.tramonto=5;
					stato.p_cardinali=OFF;
					stato.pianeti=OFF;
					stato.luna=0;
					delay(500);			//aspetta che il comando sia eseguito
					beep_high();
					beep_low();
#if(!TEST)
					plasicur();			//macchina in sicurezza
#endif
				break;

				case XESC://Fine
					ENDPGM=TRUE;
					prev_popup();
					stato.declinazione=OFF;
					stato.cerchio_polare=OFF;
					stato.sole=0;
					stato.stelle=0;
					stato.giorno=5;
					stato.p_cardinali=OFF;
					stato.pianeti=OFF;
					stato.luna=0;
					delay(500);
					beep_high();
					beep_low();
				break;
			}
		}
	}

	window(mainwind);
	NOFLASH;
	clrscr();
	_setcursortype(_NORMALCURSOR);
	sess_rec(SYSOFF);
}
