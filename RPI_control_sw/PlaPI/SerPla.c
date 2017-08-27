/* seriale per comando controllori planetario Marghera */
/* versione Linux RV15mar2014, da versione DOS RV 070400 */

#include <termios.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>	// labs()
#include <fcntl.h>
#include <unistd.h> 
#include <pthread.h>

#include "SerDefs.h"

// selezione porta e baud-rate
#define COM_PORT "/dev/ttyAMA0" // on RPI
//#define COM_PORT "/dev/ttyS0" // on virtual machine
#define BAUD B4800

// variabili per la ricezione da seriale
static int serialPortId=-1;
static int serNumCh, serCount;
static char serRxBuf[11];	// buffer di ricezione

// variabili per la gestione del polling automatico
static int nPollContr;		// controllore da interrogare/comandare (0-1-2)
static int nFailCntArray[NCONTROLLER];	// contatori di errori consecutivi per un controllore
static int nSkipCntArray[NCONTROLLER];	// contatori di cicli da saltare per un controllore
#define MAX_CMD_LEN 32
static BOOL bCmdReq;		// flag richiesta comando dal progr. principale
static BOOL bCmdRunning;	// indica che il comando e' in esecuzione (TX o RX in corso)
static BOOL bCmdSuccess;	// quando bCmdReq=FALSE indica il risultato dell'ultimo comando
static char CmdBuf[MAX_CMD_LEN];	// buffer per il comando
static int nCmdLen;				// lunghezza comando
static int nCmdId;					// identificatore di messaggio inviato al controllore

// iniziatore dei messaggi trasmessi/ricevuti
#define SYN 0x16

// SerPla.c -- uso interno
static BOOL serialLoop();
static void timeout();
static BOOL internal_poll();
static void internal_read();
static BYTE check_dval(BYTE x);
static WORD check_aval(WORD x);
static BOOL serPortOpen(int baudrate, const char *szPortName);
static BOOL serPortTx(unsigned char *pBuf, int len);
static void serPortClose(void);

// variabili condivise con gli altri thread
volatile struct SERVIZI stato;
int nVersion0;						// versione sw del controllore planetario
volatile struct MOTCONTRSTAT motc1, motc2;
volatile struct MOTSTAT motc;
long lnMessages[NCONTROLLER];	// contatore di messaggi complessivi per controllore
long lnErrors[NCONTROLLER];		// contatore di errori complessivi per controllore
pthread_mutex_t mutex_ser = PTHREAD_MUTEX_INITIALIZER;		// mutex per la sincronizzazione


// main thread function
void *SerMain()
{
	// inizializza la porta seriale
	if (!serPortOpen(BAUD, COM_PORT))
		return;

	// entra nel loop principale
	serCount = 0;
	while (1)
	{
		if (!serialLoop())
			timeout();
	}
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

	// ascensione retta: compensa offset
	if (nId == 1)
		lnTarget += motc.ascRettaOffset;

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
	buf[4] = 0;		// checksum

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


static BOOL serialLoop()
{
	const char *szLoc = "serpla::serialLoop()";
	unsigned long lnBytes;
	int i;
	char ck;

	// leggi seriale con timeout, riprendi da dove eravamo arrivati
	// il timeout � 0.1 s (vedi serPortOpen)
	lnBytes = read(serialPortId, serRxBuf+serCount, sizeof(serRxBuf)-serCount);
	if (lnBytes == -1)
	{
		fprintf(stderr, "%s: errore=%s\n", szLoc, strerror(errno));
		fflush(stderr);	// altrim. non scrive su file con la redirezione
		serCount = 0;
		return FALSE;
	}

	if (!lnBytes)
	{
		// timeout o messaggio di cui abbiamo perso una parte
		// annulla quanto finora ricevuto e ritorna causando timeout
		serCount = 0;
		return FALSE;
	}
	else
	{
		// vedi se il messaggio � completo, altrimenti riprova
		serCount += lnBytes;
		if (serCount == 1)
		{
			// ricevuto solo iniziatore
			// verifica che sia corretto e ritorna
			if (serRxBuf[0] != SYN)
				// elimina il carattere ricevuto non valido
				serCount = 0;
			return TRUE;
		}

		// il secondo carattere � la lunghezza: vedi se � valido e se abbiamo abbastanza caratteri
		// i valori validi sono 1,2,8
		serNumCh = serRxBuf[1];
		if (serNumCh != 1 && serNumCh != 2 && serNumCh != 8)
		{
			// azzera tutto il buffer ricevuto e riprova
			serCount = 0;
			return TRUE;
		}

		// il valore ritornato in serNumCh � la lunghezza effettiva -2
		if (serCount < serNumCh + 2)
			return TRUE;
	}

	// se siamo arrivati qui, il numero di byte � corretto (o sovrabbondante)
	// controlla il checksum: solo l'iniziatore � escluso
	for (ck=0,i=1; i<serCount; i++)
		ck ^= serRxBuf[i];
	if (!ck)
	{
		// messaggio ok!
		// leggiamo il contenuto
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
		if (!internal_poll())
			// seconda chiamata se la prima era per il controllore zero
			internal_poll();
	}
	else
	{
		// errore
		// in questo caso ci limitiamo ad ignorarlo: il micro ha risposto, supponiamo dati corrotti
		// se era un comando bCmdReq resta TRUE e verr� ripetuto da internal_poll()
		if (!internal_poll())
			// seconda chiamata se la prima era per il controllore zero
			internal_poll();
	}

	// in ogni caso torniamo in attesa di un nuovo messaggio
	serCount = 0;
	return TRUE;
}


// gestione timeout in ricezione
static void timeout()
{
	static BOOL bPollActive=FALSE;

	// all'inizio attiva il polling
	if (!bPollActive)
	{
		bPollActive = TRUE;

		// partiamo dal controllore uno (internal_poll() fa un incremento)
		nPollContr = 0;

		// inizia la prima trasmissione (non serve doppia chiamata, non � controllore zero)
		internal_poll();
	}
	else
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
		if (!internal_poll())
			// seconda chiamata se la prima era per il controllore zero
			internal_poll();
	}
}


// routine di polling
// torna TRUE se il messaggio prevede risposta, FALSE altrimenti
static BOOL internal_poll()
{
	const char *szLoc = "serpla::internal_poll()";
	int i;
	char ck;
	int nMsgLen;
	BOOL bAnswer = TRUE;
	char tempTxBuf[MAX_CMD_LEN];	// buffer di appoggio per tx

	// c'e' una richiesta esterna?
	if (bCmdReq)
	{
		// si', attivala
		bCmdRunning = TRUE;

		// trasmetti
		serPortTx(CmdBuf, nCmdLen);

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
			pthread_mutex_lock(&mutex_ser);
			stato.supernova = check_aval(stato.supernova);
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
			pthread_mutex_unlock(&mutex_ser);

			// no risposta
			bAnswer = FALSE;
			break;

		case 1:	// controllori motori
		case 2:	// semplice richiesta di stato
			nMsgLen = 5;
			tempTxBuf[3] = 4;	// stato -> codice 4
			break;

		default:	// solo per sicurezza
			fprintf(stderr, "%s: errore nPollContr=%d\n", szLoc, nPollContr);
			fflush(stderr);	// altrim. non scrive su file con la redirezione
		}

		// inserisci la lunghezza (ridotta) nel posto giusto
		tempTxBuf[2] = nMsgLen - 3;

		// calcola checksum
		for (i=1, ck=0; i<nMsgLen-1; i++)
			ck ^= tempTxBuf[i];
		tempTxBuf[nMsgLen-1] = ck;

		// trasmetti
		serPortTx(tempTxBuf, nMsgLen);
	}

	// incrementa il contatore di messaggi per questo controllore
	lnMessages[nPollContr]++;

	return bAnswer;
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
			nVersion0 = serRxBuf[2];
		}
	}
	else
	{
		// un controllore motori
		long encX, encY;

		encX = (((long)serRxBuf[3]&255)<<16) |
			(((long)serRxBuf[4]&255)<<8) | (serRxBuf[5]&255);
		encX = (encX<<8)>>8;	// aggiusta il segno
		encY = (((long)serRxBuf[6]&255)<<16) |
				(((long)serRxBuf[7]&255)<<8) | (serRxBuf[8]&255);
		encY = (encY<<8)>>8;	// aggiusta il segno

		pthread_mutex_lock(&mutex_ser);
		if (nPollContr == 1)
		{
			// aggiorna dati controllore 1
			motc1.encX = encX;
			motc1.encY = encY;
			motc1.flag = serRxBuf[2];

			// azzera l'offset ascensione retta se siamo vicino allo zero (azzeramento appena eseguito?)
			// gli altri aggiustamenti all'offset sono all'inizio di run()
			// RV30oct2016: NO, potrebbe causare un salto in un ciclo partito con offset!=0
			//if (labs(encY) < 10000L)
			//	motc.ascRettaOffset = 0;

			// aggiorna dati riassuntivi
			motc.declinazione = encX;
			motc.ascRetta = encY - motc.ascRettaOffset;
			motc.bSearchZeroDecl = (motc1.flag & 0x10)==0x10;
			motc.bSearchZeroAscR = (motc1.flag & 0x20)==0x20;
		}
		else
		{
			// aggiorna dati controllore 2
			motc2.encX = encX;
			motc2.encY = encY;
			motc2.flag = serRxBuf[2];

			// aggiorna dati riassuntivi
			motc.precessione = encY;
			motc.bSearchZeroPrec = (motc2.flag & 0x20)==0x20;
		}
		pthread_mutex_unlock(&mutex_ser);
	}
}


// function to open the serial port 
// 	parameters are: baudrate and portname
//
// RETURN: TRUE if all went ok, FALSE otherwise
static BOOL serPortOpen(int baudrate, const char *szPortName)
{
	const char *szLoc = "serpla::serPortOpen()";
	struct termios termio;
	// Try to open the specified serial device 
	// open for reading and writing
	if ((serialPortId = open(szPortName, O_RDWR | O_NOCTTY)) == -1)
	{
		// open return a nonnegative integer representing the lowest numbered unused file descriptor. 
		// Otherwise, -1 is returned and errno is set 
		fprintf(stderr, "%s : errore apertura porta seriale '%s': codice=%s\n",
			szLoc, szPortName, strerror(errno));
		fflush(stderr);	// altrim. non scrive su file con la redirezione
		return FALSE;
	}

	// tcgetattr is the function used to read the termios structure 
	// Get current settings of the terminal device. the attributes are returned by means of "termio" struct 
	if (tcgetattr(serialPortId, &termio) == -1)
	{
		fprintf(stderr, "%s: tcgetattr() porta '%s' errore %s\n",
			szLoc, szPortName, strerror(errno));
		fflush(stderr);	// altrim. non scrive su file con la redirezione
		serPortClose();
		return FALSE;
	}

	// non canonical input processing 
	termio.c_cflag = baudrate | CS8 | CLOCAL | CREAD;
	termio.c_iflag = IGNPAR;
	termio.c_oflag = 0;
	termio.c_lflag = 0;		// non canonical mode ICANON=0;
	// VTIME sets the character timer and VMIN sets the minimum number of characters to receive before satisfying the read 
	termio.c_cc[VMIN] = 0;	// no min chars, otherwise at least one is required
	termio.c_cc[VTIME] = 2; // inter-character timer 0.2 s (far more than longest message)
	// in this case the read will be satisfied if a single character is read or TIME is exceeded (t = TIME*0.1s).
	//   If time is exceeded, no characters will be returned
	if (tcsetattr(serialPortId, TCSANOW, &termio) == -1)
	{
		fprintf(stderr, "%s: tcsetattr() porta '%s' errore %s\n",
			szLoc, szPortName, strerror(errno));
		fflush(stderr);	// altrim. non scrive su file con la redirezione
		serPortClose();
		return FALSE;
	}

	// flush port (don't abort on error)
	if (tcflush(serialPortId, TCIOFLUSH) == -1)
	{
		fprintf(stderr, "%s: WARNING tcflush() porta '%s' errore %s\n",
			szLoc, szPortName, strerror(errno));
		fflush(stderr);	// altrim. non scrive su file con la redirezione
	}

	// all went ok 
	return TRUE;
}


// function used to write on the serial port the messages
//	parameters are: pointer to the buffer that contains the bytes, number of bytes that must be written
//
// RETURN TRUE if all bytes are written correctly , FALSE otherwise
static BOOL serPortTx(unsigned char *pBuf, int len)
{
	const char *szLoc = "serpla::serComTx()";
	unsigned long bytesWritten; // number of bytes written 
	// write to device 
	// check if all bytes are written 
	if ((bytesWritten = write(serialPortId, pBuf, len)) != len)
	{
		fprintf(stderr, "%s: errore scrittura su porta seriale. Non tutti i caratteri inviati.\n", szLoc);
		fflush(stderr);	// altrim. non scrive su file con la redirezione
		return FALSE;
	}

	// all went ok
	return TRUE;
}


static void serPortClose(void)
{
	if (serialPortId != -1)
		close(serialPortId);
	serialPortId = -1;
}
