/* seriale per comando controllori planetario Marghera */
/* RV 070400 */

#define _SERPLA_C_

// per debug (da infilare in giro)
//char far *pV = (char far *)0xb8000000L;
//static char cnt;
//if (++cnt==25) { cnt=0; (*pV)++; }


#include <dos.h>

#include <conio.h>	//-GP

#include "SerDefs.h"

// selezione porta e baud-rate
#define COM 2
#define BAUD BAUD4800

// definizione modi per RTS e DTR
#define MODERTSDTR (DTR_ON | RTS_OFF)

#if COM==1
 #define PORT 0x3f8
 #define IRQ  4
#elif COM==2
 #define PORT 0x2f8
 #define IRQ  3
#else
 #error COM=1 o 2
#endif

#define IRQTIMER 8

#define IRQVECT (IRQ+8) // per IRQ=0,7: su PIC1
#define PICBASE 0x20    // PIC1 base address

#define BAUD300  384
#define BAUD1200 96
#define BAUD2400 48
#define BAUD4800 24
#define BAUD9600 12
#define BAUD19200 6
#define BAUD38400 3

// modi per Modem Control Register (tutti con OUT2 on)
#define DTR_ON	9
#define DTR_OFF	8
#define RTS_ON	10
#define RTS_OFF	8

// vecchi interrupt della seriale e del timer
static void (_interrupt _far *oldvect_ser)();
static void (_interrupt _far *oldvect_tim)();

// variabili per la routine di interrupt seriali in ricezione
enum SER_STAT { WAIT_SYN, WAIT_LEN, GET_CHARS };	// valori per lo stato
static enum SER_STAT serStatus = WAIT_SYN;		// stato della macchina a stati
static int serNumCh, serCount;
static char serRxBuf[9];							// buffer di ricezione

// variabili per la routine di interrupt seriali in trasmissione
// buffer circolare di 256 caratteri con puntatori unsigned char
static char serTxBuf[256];
static unsigned char txHead, txTail;
static BOOL bTxOn;

// variabili per la gestione del polling automatico
static BOOL bPollActive;
static int nPollContr;		// controllore da interrogare/comandare (0-1-2)
static int nReplyTimeout;	// numero di tick per il timeout rx. 0 per i messaggi senza risposta
static int nFailCntArray[NCONTROLLER];	// contatori di errori consecutivi per un controllore
static int nSkipCntArray[NCONTROLLER];	// contatori di cicli da saltare per un controllore
#define MAX_CMD_LEN 32
static volatile BOOL bCmdReq;		// flag richiesta comando dal progr. principale
static volatile BOOL bCmdRunning;	// indica che il comando e' in esecuzione (TX o RX in corso)
static volatile BOOL bCmdSuccess;	// quando bCmdReq=FALSE indica il risultato dell'ultimo comando
static volatile char CmdBuf[MAX_CMD_LEN];	// buffer per il comando
static volatile int nCmdLen;				// lunghezza comando
static int nCmdId;					// identificatore di messaggio inviato al controllore
static char tempTxBuf[MAX_CMD_LEN];	// buffer di appoggio per tx

// iniziatore dei messaggi trasmessi/ricevuti
#define SYN 0x16

// SerPla.c -- uso interno
static void open_port();
static void _interrupt _far int_hand();
static void _interrupt _far timer_int_hand();
static void int_restore();
static void internal_poll();
static void internal_read();
static BYTE check_dval(BYTE x);
static WORD check_aval(WORD x);

static int iGira=1;

BOOL SerInit()
{
	// installa routine da chiamare all'uscita
	if (atexit(int_restore))
		return FALSE;  /* torna errore */

	// leggi vecchi vettori IRQ
	oldvect_ser=_dos_getvect(IRQVECT);		// seriale
	oldvect_tim=_dos_getvect(IRQTIMER);		// timer

	// inizializza la porta seriale
	open_port();

	// collega l'interrupt del timer
	_dos_setvect(IRQTIMER, timer_int_hand);  /* assegna nuovo vettore */

	return TRUE;
}

// torna TRUE se e' consentito trasmettere
// (no trasmissione di comando in corso)
BOOL SerCanTx()
{
	return !bCmdReq;
}

// torna il risultato dell'ultimo comando
// 0:  in corso di esecuzione
// 1:  OK
// -1: fallito (timeout)
int SerGetCmdResult()
{
	if (bCmdReq)
		return 0;

	if (bCmdSuccess)
		return 1;

	return -1;
}

// routine di trasmissione generica: accoda un messaggio
//   che sara' trasmesso prima possibile.
// Da non usare direttamente (usare SerCmdXxx())
// torna TRUE se la richiesta e' stata accolta
BOOL SerTxBuf(char *pMsg, int nMsgSize)
{
	int i;

	// messaggio nullo?
	if (!nMsgSize)
		// non fare nulla
		return FALSE;

	// eccede la lunghezza massima ammessa?
	if (nMsgSize > MAX_CMD_LEN)
		// torna errore
		return FALSE;

	// non possiamo accodare se c'e' gia' un comando in esecuzione
	if (bCmdReq)
		return FALSE;

	// copia il messaggio sul buffer di richiesta
	for (i=0; i<nMsgSize; i++)
	{
		CmdBuf[i]=pMsg[i];
	}

	// attiva la richiesta
	nCmdLen = nMsgSize;
	bCmdReq = TRUE;
	return TRUE;
}

// comando di zero per un encoder
// nId 0:declinazione, 1:asc. retta 2:precessione
// wSpeed velocita' di spostamento in termini di variazione
//   di posizione encoder in 65.536 ms (0.065 s)
// ritorna: come SerTxBuf()
BOOL SerCmdZeroEnc(int nId, WORD wSpeed)
{
	char buf[7], ck;
	int i;

	if (nId < 0 || nId > 2)
		return FALSE;

	buf[0] = SYN;
	buf[1] = (nId==2) ? 2 : 1;	// controllore 2 solo per la precessione
	buf[2] = 4;
	buf[3] = (nId==0) ? 0 : 1;	// encoder X solo per l'ascensione retta
	buf[4] = (wSpeed >> 8) & 255;
	buf[5] = wSpeed & 255;

	for (i=1, ck=0; i<6; i++)
		ck ^= buf[i];
	buf[6] = ck;

	return SerTxBuf(buf, 7);
}

// comando "goto" per un encoder
// nId 0:declinazione, 1:asc. retta 2:precessione
// target destinazione in termini di posizione encoder (24 bit C2)
// wSpeed velocita' di spostamento in termini di variazione
//   di posizione encoder in 65.536 ms (0.065 s)
// ritorna: come SerTxBuf()
BOOL SerCmdGotoEnc(int nId, long lnTarget, WORD wSpeed)
{
	char buf[10], ck;
	int i;

	if (nId < 0 || nId > 2)
		return FALSE;

	buf[0] = SYN;
	buf[1] = (nId==2) ? 2 : 1;	// controllore 2 solo per la precessione
	buf[2] = 7;
	buf[3] = (nId==0) ? 2 : 3;	// encoder X solo per l'ascensione retta
	buf[4] = (lnTarget >> 16) & 255;
	buf[5] = (lnTarget >> 8) & 255;
	buf[6] = lnTarget & 255;
	buf[7] = (wSpeed >> 8) & 255;
	buf[8] = wSpeed & 255;

	for (i=1, ck=0; i<9; i++)
		ck ^= buf[i];
	buf[9] = ck;

	return SerTxBuf(buf, 10);
}


// richiesta di acknowledge controllore zero
// ritorna: come SerTxBuf()
BOOL SerCmdAck0()
{
	char buf[5];

	buf[0] = SYN;
	buf[1] = 0;		// controllore zero
	buf[2] = 2;		// lunghezza da adesso
	buf[3] = 2;		// codice messaggio
	buf[4] = 1;		// checksum

	return SerTxBuf(buf, 5);
}


// richiesta di versione controllore zero
// ritorna: come SerTxBuf()
BOOL SerCmdVer0()
{
	char buf[5];

	buf[0] = SYN;
	buf[1] = 0;		// controllore zero
	buf[2] = 2;		// lunghezza da adesso
	buf[3] = 3;		// codice messaggio
	buf[4] = 1;		// checksum

	return SerTxBuf(buf, 5);
}

static void open_port()
{
	int i;

	/* line control register
	  . 1 . 0 . 00 . 0 . 0 . 11 .
		|   |   |___ |   |   |____ 8 bit
		|   |       \|   |_ 1 stop
  DLAB__|   |_setbrk |_ no parity     */
	outp(PORT+3,0x83);

	/* DLAB=1 -> accesso al baud generator */
	outp(PORT,BAUD & 255);  /* byte meno signif. */
	outp(PORT+1,BAUD >> 8); /* byte piu' signif. */
	outp(PORT+3,0x03);      /* DLAB=0 */
	outp(PORT+1,0);         /* disabilita interrupt */

	/* modem control register
	  . 000 . 0 . 1 . 0 . 0 . 0 .
		      |   |   |   |   |_ DTR
		LOOP _|   |   |   |__ RTS
		    OUT2 _|   |__ OUT1         */

	outp(PORT+4, MODERTSDTR);
	_dos_setvect(IRQVECT,int_hand);  /* assegna nuovo vettore */
	outp(PICBASE+1, inp(PICBASE+1) & (~(1<<IRQ)));
	outp(PICBASE, 0x60 | IRQ);  /* specific EOI command */
	inp(PORT);                  /* clear eventuali int pendenti */
	inp(PORT+5);
	outp(PORT+1, 3);            /* abilita interrupt TX & RX */
}

static void _interrupt _far int_hand()
{
	// leggi IIR, loopa finche' ci sono interrupt attivi
	char c, iir;
	while (!((iir=inp(PORT+2)) & 1))
	{
		switch (iir)
		{
		case 4:		// received data available
			c=inp(PORT);     /* leggi carattere (annulla l'interrupt) */

			//-GP
/*			window(1,1,80,25);
			gotoxy(1,3);
			switch(iGira)
			{
				case 1:
					cprintf("/");
					iGira++;
				break;
				case 2:
					cprintf("-");
					iGira++;
				break;
				case 3:
					cprintf("\\");
					iGira++;
				break;
				case 4:
					cprintf("-");
					iGira=1;
				break;
			}
*/
			switch(serStatus)
			{
			case WAIT_SYN:
				// siamo in attesa dell'iniziatore
				if (c == SYN)
				{
					// ignora gli altri caratteri
					serStatus = WAIT_LEN;
				}
				break;

			case WAIT_LEN:
				// numero byte rimanenti: deve essere > 0 e <= 10
				serNumCh = c;
				if (serNumCh > 0 && serNumCh <= 10)
				{
					serStatus = GET_CHARS;
					serCount = 0;
				}
				else
					// torna in WAIT_SYN, a meno che non sia c==SYN
					if (c != SYN)
						serStatus = WAIT_SYN;
				break;

			case GET_CHARS:
				if (serCount == serNumCh-1)
				{
					// controllo checksum
					int i;
					char ck;
					ck = c ^ serNumCh;	// lunghezza inclusa nel calcolo
					for (i=0; i<serNumCh-1; i++)
						ck ^= serRxBuf[i];
					if (!ck)
					{
						// messaggio ok!
						// torniamo in attesa di messaggi
						serStatus = WAIT_SYN;

						// annulliamo il timeout
						nReplyTimeout = 0;

						// leggiamo il messaggio
						internal_read();

						// se era un comando, e' terminato con successo
						if (bCmdReq && bCmdRunning)
						{
							bCmdReq = bCmdRunning = FALSE;
							bCmdSuccess = TRUE;
						}

						// resetta il contatore di errori per questo controllore,
						//   perche' sta funzionando correttamente
						nFailCntArray[nPollContr] = 0;

						// resetta anche il contatore di skip per sicurezza, ma dovrebbe
						//   essere gia' zero
						nSkipCntArray[nPollContr] = 0;

						// cicla al prossimo polling
						internal_poll();
					}
					else	//-GP
					{
						serStatus = WAIT_SYN;				//Gianpietro 10GEN2009
						// annulliamo il timeout
						nReplyTimeout = 0;      		//errori di ricezione non gestiti
						internal_poll();
					}			//-GP
				}
				else
					// accumula
					serRxBuf[serCount++] = c;
			}
			break;

		case 2:		// transmitter holding register empty
			// spedisci un byte se non e' finita la coda
			if (txTail != txHead)
				outp(PORT, serTxBuf[txTail++]);
			else
			{
				// trasmissione finita o quasi (coda UART non vuota)
				bTxOn = FALSE;

				// se il messaggio non prevede risposta dobbiamo attivare
				//   un nuovo polling
				// controlla bPollActive perche' c'e' un interrupt
				//   indesiderato alla partenza
				if (bPollActive && !nReplyTimeout)
					internal_poll();
			}
			break;
		}

		// EOI qui o prima dell'inp() o fuori dal loop ???
		outp(PICBASE,0x20);   /* non specific EOI command */
	}
}


// interrupt del timer
static void _interrupt _far timer_int_hand()
{
	// all'inizio attiva il polling
	if (!bPollActive)
	{
		bPollActive = TRUE;

		// partiamo dal controllore uno (internal_poll() fa un incremento)
		nPollContr = 0;

		// inizia la prima trasmissione
		internal_poll();
	}
	else
	{
		// aggiorna il contatore timeout se abilitato e se non siamo in trasmissione
		if (!bTxOn && nReplyTimeout > 0)
		{
			nReplyTimeout--;

			// vedi se ora e' scaduto
			if (!nReplyTimeout)
			{
				// era un comando esterno?
				if (bCmdReq && bCmdRunning)
				{
					// si', marcalo terminato e fallito
					bCmdReq = bCmdRunning = bCmdSuccess = FALSE;
				}

				// ora incrementa il contatore di errori per il controllore
				nFailCntArray[nPollContr]++;

				// in funzione del valore, programma dei salti del polling
				switch (nFailCntArray[nPollContr])
				{
				case 1:		// primo errore, salta un solo ciclo
					nSkipCntArray[nPollContr] = 1;
					break;
				case 2:		// secondo errore, salta 4 cicli
					nSkipCntArray[nPollContr] = 4;
					break;
				default:	// tre errori o piu', salta 16 cicli
					nSkipCntArray[nPollContr] = 16;
				}

				// incrementa i contatori di errore complessivi
				//   (nPollContr e' valido anche per i comandi)
				lnErrors[nPollContr]++;

				// esegui il polling
				internal_poll();
			}
		}
	}

	// esci tramite la routine originale
	_chain_intr(oldvect_tim);
}


// routine di polling chiamata dalle routine di interrupt
static void internal_poll()
{
	int i;
	char ck;
	int nMsgLen;

	// c'e' una richiesta esterna?
	if (bCmdReq)
	{
		// si', attivala
		bCmdRunning = TRUE;

		// i comandi esterni prevedono sempre risposta
		nReplyTimeout = 3;

		// copia il messaggio sul buffer di trasmissione
		for (i=0; i<nCmdLen; i++)
			serTxBuf[txHead++]=CmdBuf[i];

		// estrai il codice del controllore e l'identificatore del
		//   messaggio, che ci servono per decodificare la risposta
		// ... purche' il codice del controllore sia valido
		if (CmdBuf[1]>=0 && CmdBuf[1]<=2)
			nPollContr = CmdBuf[1];
		nCmdId = CmdBuf[3];
	}
	else
	{
		// non e' richiesta esterna
		// cerca il prossimo candidato per il polling
		// while (1) perche' potrebbero avere tutti dei conteggi SKIP
		while (1)
		{
			if (++nPollContr >= NCONTROLLER)
				nPollContr = 0;

			// esci sul primo che ha conteggio skip nullo
			if (!nSkipCntArray[nPollContr])
				break;

			// se non era nullo, decrementa il conteggio skip
			nSkipCntArray[nPollContr]--;
		}
		
		// costruisci il messaggio su un buffer temporaneo
		tempTxBuf[0] = SYN;
		tempTxBuf[1] = nPollContr;	// id del controllore

		switch(nPollContr)
		{
		case 0:	// controllore planetario
			// messaggio di aggiornamento completo
			nMsgLen = 25;
			tempTxBuf[3] = 1;	// aggiornamento uscite -> codice 1

			// estrai i dati dalla struttura, correggendo il range ove necessario
			stato.supernova=check_aval(stato.supernova);
			tempTxBuf[4] = (stato.supernova&0xFF00)>>8;
			tempTxBuf[5] = stato.supernova&0xFF;

			stato.p_nebula=check_aval(stato.p_nebula);
			tempTxBuf[6] = (stato.p_nebula&0xFF00)>>8;
			tempTxBuf[7] = stato.p_nebula&0xFF;

			stato.nuove_s=check_aval(stato.nuove_s);
			tempTxBuf[8] = (stato.nuove_s&0xFF00)>>8;
			tempTxBuf[9] = stato.nuove_s&0xFF;

			stato.a_libero=check_aval(stato.a_libero);
			tempTxBuf[10] = (stato.a_libero&0xFF00)>>8;
			tempTxBuf[11] = stato.a_libero&0xFF;

			stato.stelle=check_aval(stato.stelle);
			tempTxBuf[12] = (stato.stelle&0xFF00)>>8;
			tempTxBuf[13] = stato.stelle&0xFF;

			stato.sole=check_aval(stato.sole);
			tempTxBuf[14] = (stato.sole&0xFF00)>>8;
			tempTxBuf[15] = stato.sole&0xFF;

			stato.luna=check_aval(stato.luna);
			tempTxBuf[16] = (stato.luna&0xFF00)>>8;
			tempTxBuf[17] = stato.luna&0xFF;

			stato.cerchio_orario=check_aval(stato.cerchio_orario);
			tempTxBuf[18] = (stato.cerchio_orario&0xFF00)>>8;
			tempTxBuf[19] = stato.cerchio_orario&0xFF;

			tempTxBuf[20] = (stato.d_libero3<<7) | (stato.d_libero2<<6) |
							(stato.p_cardinali<<5) | (stato.d_libero1<<4) |
							(stato.luce_sala<<3) | (stato.cerchio_polare<<2) |
							(stato.declinazione<<1) | stato.pianeti;

			stato.alba=check_dval(stato.alba);
			tempTxBuf[21] = stato.alba;

			stato.tramonto=check_dval(stato.tramonto);
			tempTxBuf[22] = stato.tramonto;

			stato.giorno=check_dval(stato.giorno);
			tempTxBuf[23] = stato.giorno;

			// no risposta
			nReplyTimeout = 0;
			break;

		case 1:	// controllori motori
		case 2:	// semplice richiesta di stato
			nMsgLen = 5;
			tempTxBuf[3] = 4;	// stato -> codice 4

			// risposta richiesta
			nReplyTimeout = 3;

			break;

		default:	// solo per sicurezza
			nMsgLen = 5;
			nReplyTimeout = 0;
		}

		// inserisci la lunghezza (ridotta) nel posto giusto
		tempTxBuf[2] = nMsgLen - 3;

		// calcola checksum
		for (i=1, ck=0; i<nMsgLen-1; i++)
			ck ^= tempTxBuf[i];
		tempTxBuf[nMsgLen-1] = ck;

		// copia il messaggio in coda circolare
		for (i=0; i<nMsgLen; i++)
			serTxBuf[txHead++]=tempTxBuf[i];
	}

	// incrementa il contatore di messaggi per questo controllore
	lnMessages[nPollContr]++;

	// accendi flag per evitare di conteggiare timeout in trasmissione
	bTxOn = TRUE;

	// trasmetti il primo byte per attivare l'interrupt tx
	outp(PORT, serTxBuf[txTail++]);
}


// controllo range per gli analog output del controllore zero
static WORD check_aval(WORD x)
{
	if (x>4050)
		return 4050;
	else
		return x;
}


// controllo range per gli output PWM del controllore zero
static BYTE check_dval(BYTE x)
{
	if (x<5)
		return 5;
	else if (x>250)
		return 250;
	else
		return x;
}


// elabora messaggio ricevuto
static void internal_read()
{
	// dipende dal tipo di messaggio e dal controllore coinvolto
	// sono possibili:
	//   solo messaggi di stato dai controllori motori (per qualunque comando)
	//   messaggi ack e di versione dal controllore 0
	if (nPollContr == 0)
	{
		// controllore planetario
		// consideriamo solo il codice 3 (firmware version). L'unico
		//   altro possibile e' 2 (ack), che non porta informazione:
		//   l'importante e' che vada a buon fine (no timeout). Il
		//   messaggio di codice 1 (aggiornamento uscite) non prevede
		//   risposta
		if (nCmdId == 3)
		{
			// leggi la versione senza troppi controlli
			//   (dovrebbe essere serCount==1)
			nVersion0 = serRxBuf[0];
		}
	}
	else
	{
		// un controllore motori
		long encX, encY;

		encX = (((long)serRxBuf[1]&255)<<16) |
			(((long)serRxBuf[2]&255)<<8) | (serRxBuf[3]&255);
		encX = (encX<<8)>>8;	// aggiusta il segno
		encY = (((long)serRxBuf[4]&255)<<16) |
				(((long)serRxBuf[5]&255)<<8) | (serRxBuf[6]&255);
		encY = (encY<<8)>>8;	// aggiusta il segno

		if (nPollContr==1)
		{
			// aggiorna dati controllore 1
			motc1.encX = encX;
			motc1.encY = encY;
			motc1.flag = serRxBuf[0];

			// aggiorna dati riassuntivi
			motc.declinazione = encX;
			motc.ascRetta = encY;
			motc.bSearchZeroDecl = (motc1.flag & 0x10)==0x10;
			motc.bSearchZeroAscR = (motc1.flag & 0x20)==0x20;

			//-GP
/*			window(1,1,80,25);
			gotoxy(1,2);
			switch(iGira)
			{
				case 1:
					cprintf("/");
					iGira++;
				break;
				case 2:
					cprintf("-");
					iGira++;
				break;
				case 3:
					cprintf("\\");
					iGira++;
				break;
				case 4:
					cprintf("-");
					iGira=1;
				break;
			}*/
			//-GP

		}
		else
		{
			// aggiorna dati controllore 2
			motc2.encX = encX;
			motc2.encY = encY;
			motc2.flag = serRxBuf[0];

			// aggiorna dati riassuntivi
			motc.precessione = encY;
			motc.bSearchZeroPrec = (motc2.flag & 0x20)==0x20;

			//-GP
/*			window(1,1,80,25);
			gotoxy(1,3);
			switch(iGira)
			{
				case 1:
					cprintf("/");
					iGira++;
				break;
				case 2:
					cprintf("-");
					iGira++;
				break;
				case 3:
					cprintf("\\");
					iGira++;
				break;
				case 4:
					cprintf("-");
					iGira=1;
				break;
			}*/
			//-GP

		}
	}
}


static void int_restore()
{
	outp(PICBASE+1,inp(PICBASE+1) | (1<<IRQ));  /* clear IRQ */
	_dos_setvect(IRQVECT,oldvect_ser);   /* ripristina vecchi vettori */
	_dos_setvect(IRQTIMER,oldvect_tim);
}
