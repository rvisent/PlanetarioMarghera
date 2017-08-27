// Shared.h
// RV2014mar29
// struttura scambiate tra eseguibile planetario e server FASTCGI

#ifndef SHARED_H
#define SHARED_H

#ifndef _TYP_
#define _TYP_

typedef enum { FALSE, TRUE } BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#endif // _TYP_

typedef union semun
{
	int val;
	struct semid_ds *buf;
	unsigned short * array;
	struct seminfo *__buf;
}semun_t;


#define SHARED_KEY 1997
#define SEM_KEY 1995

typedef enum { MODE_OFFLINE, MODE_IDLE, MODE_SETPLANETS, MODE_RUN, MODE_ZERO, MODE_SAFEPOS, MODE_TEST, MODE_SHUTDOWN,
			MODE_SAVE, MODE_DEFAULT } MODE;
typedef enum { SET_SOLE, SET_LUNA, SET_MERCURIO, SET_VENERE, SET_MARTE, SET_GIOVE, SET_SATURNO } SETMODE;
typedef enum { SUB_NONE, AZZERAMENTO, POSIZIONAMENTO_INIZ, PRONTO_POMERIGGIO,
			RUN_POMERIGGIO, RUN_TRAMONTO, PRONTO_NOTTE, RUN_NOTTE,
			PRONTO_ALBA, RUN_ALBA, POSIZIONAMENTO_FIN, PRONTO_TERMINARE} SUBMODE;
typedef enum {
	LIGHT_NONE, LIGHT_SOLE, LIGHT_LUNA, LIGHT_STELLE, LIGHT_CERCHIOORARIO, LIGHT_SUPERNOVA, LIGHT_PNEBULA,
	LIGHT_NUOVES, LIGHT_ALIBERO, LIGHT_ALBA, LIGHT_GIORNO, LIGHT_TRAMONTO, LIGHT_PIANETI,
	LIGHT_DECLINAZIONE, LIGHT_PCARDINALI, LIGHT_LUCESALA, LIGHT_CERCHIOPOLARE,
	LIGHT_DLIBERO1, LIGHT_DLIBERO2, LIGHT_DLIBERO3 } LIGHT_ID;

struct SHARED_DATA
{
	// variabili modificate con POST
	BOOL bAbort;	// se TRUE interrompe qualunque operazione in corso, poi Pla rimette a FALSE
	BOOL bSuspend;	// se TRUE sospende l'operazione (se applicabile), esce con bAbort
	BOOL bContinue;	// se TRUE riprende l'operazione (se applicabile), esce con bAbort
	BOOL bResetError; // se TRUE azzera eventuale nError
	MODE NewMode;	// se idle entra nel modo richiesto e mette MODE_IDLE
	SETMODE NewSettingMode;	// dettaglio per MODE_SETPLANETS

	LIGHT_ID lightToBeChanged; // resettato a LIGHT_NONE dopo l'esecuzione
	int lightLevel;	// nuovo livello per la luce
	double annoPrecessione;	// inizializzato a 2014
	int horlight;			// luce orizzonte durante la simulazione, inizializzata a 10
	int maxstars;			// massima luce stelle durante la simulazione (se non alterata dall'op), init 800
	int speed;				// velocita' simulazione, inizializzata a 100
	BOOL bAllLightsOff;		// modo test, spegne tutte le luci
	BOOL bAllLightsOn;		// modo test, tutte le luci al massimo
	int motorSpeed;			// modo test, velocità motore
	long motorTarget;		// modo test, setpoint motore
	int motorId;			// modo test, id motore da muovere
	BOOL bTestZero;			// modo test, avvia azzeramento
	BOOL bTestMove;			// modo test, avvia posizionamento
	double sun_ar, moon_ar, mercury_ar, venus_ar, mars_ar,
		jupiter_ar, saturn_ar, latitude;

	// variabili lette con GET
	MODE PlaMode;			// modo corrente
	SETMODE SettingMode;	// dettaglio se MODE_SETPLANETS
	SUBMODE PlaSubMode;		// fase operativa per azzeramento e run

	int idError;			// <>0: codice errore
	BOOL bPaused;			// TRUE: in pausa
	BOOL bZeroDone;			// segnala azzeramento macchina gia' effettuato 
	BOOL bNAKluci, bNAKlat, bNAKascr, bNAKprec;	// dettaglio errori NAK da seriali

	struct SERVIZI luci;
	struct MOTSTAT motori;
	double ar_now, ar_nowpr, latitude_now, prec_now, prec_now_year;
	int time_hour, time_min;
	int perc_action;		// percentuale avanzamento SUBMODE
};

extern volatile struct SHARED_DATA *pSharedData;	// pointer to shared memory

// methods
BOOL initSharedMemoryAndSemaphores();
int lock();
int unlock();
void updateData();

// definizioni codici idError
#define ERR_ABORTED 1
#define ERR_PLAZERO_ZEROENC 2
#define ERR_PLAZERO_ZEROENC2 3
#define ERR_PLAMAIN_NOTZEROED 4
#define ERR_PLASICUR_GOTOSICUR 5
#define ERR_SETTINGS_GOTOENC 6
#define ERR_SETTINGS_OUTOFRANGE 7
#define ERR_RUN_GOTOENC 8
#define ERR_TEST_BADMOTORID 9
#define ERR_TEST_ZEROENC 10
#define ERR_TEST_GOTOENC 11
#define ERR_PARAMS_DEFAULTED 12
#define ERR_PARAMS_WRITE 13

#endif	// SHARED_H