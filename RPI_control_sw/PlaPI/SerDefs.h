// SerDefs.h

#ifndef _SERDEFS_
#define _SERDEFS_

#ifndef _TYP_
#define _TYP_

typedef enum { FALSE, TRUE } BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#endif // _TYP_

// prototipi

// SerPla.c
void *SerMain();
BOOL SerCanTx();
BOOL SerTxBuf(char *pMsg, int nMsgSize);	// da non usare normalmente
BOOL SerCmdZeroEnc(int nId, WORD wSpeed);
BOOL SerCmdGotoEnc(int nId, long lnTarget, WORD wSpeed);
BOOL SerCmdVer0();
BOOL SerCmdAck0();
int  SerGetCmdResult();

// SerSync.c
BOOL SerCmdZeroEncSync(int nId, WORD wSpeed, int Attempts);
BOOL SerCmdGotoEncSync(int nId, long lnTarget, WORD wSpeed, int Attempts);
BOOL SerCmdVer0Sync(int Attempts);
BOOL SerCmdAck0Sync(int Attempts);

// numero controllori
#define NCONTROLLER 3

// struttura con le uscite per il comando del planetario
struct SERVIZI
{
	WORD sole;
	WORD luna;
	WORD stelle;
	WORD cerchio_orario;
	WORD supernova;
	WORD p_nebula;
	WORD nuove_s;
	WORD a_libero;
	BYTE alba;
	BYTE giorno;
	BYTE tramonto;
	char pianeti;
	char declinazione;
	char p_cardinali;
	char luce_sala;
	char cerchio_polare;
	char d_libero1;
	char d_libero2;
	char d_libero3;
};

// struttura di stato di un controllore motori
struct MOTCONTRSTAT
{
	long encX;	// posizione encoder X
	long encY;	// posizione encoder Y
	BYTE flag;	// byte di stato
};

// struttura di stato riassuntiva per i controllori motori utilizzati
struct MOTSTAT
{
	long declinazione;		// posizione encoder X controllore 1
	long ascRetta;			// posizione encoder Y controllore 1
	long precessione;		// posizione encoder Y controllore 2
	long ascRettaOffset;	// offset ascensione retta per compensare giri interi
	BOOL bSearchZeroDecl;	// ricerca zero declinazione in corso
	BOOL bSearchZeroAscR;	// ricerca zero ascensione retta in corso
	BOOL bSearchZeroPrec;	// ricerca zero precessione in corso
};

// dichiarate in SerPla.c
extern volatile struct SERVIZI stato;
extern int nVersion0;
extern volatile struct MOTCONTRSTAT motc1, motc2;
extern volatile struct MOTSTAT motc;
extern long lnMessages[NCONTROLLER];
extern long lnErrors[NCONTROLLER];
extern pthread_mutex_t mutex_ser;

#endif	// _SERDEFS_
