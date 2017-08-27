// PlaDefs.h

// root and read-only FS version? (1:RPI, 0:VM)
#define ROOT_READONLY	1			// 1:root+readonly; 0:normal PI

#define SYSON			0
#define SYSOFF			1
#define STARSON			2
#define STARSOFF		3

#define	RUN				0
#define	STOP			1
#define	END				2

#define	CNT_LATD		0
#define CNT_AR			1
#define	CNT_PREC		2

#define	TSIM			3000		// in centesimi di s - simulazione transizioni

#define MAXARCNT		233496L		// conteggio della rotazione AR

#define ONETURN			220000L		// conteggio di una rotazione simulata
#define	TOSUNSET		-30000L		// conteggio fino al tramonto (Inverno)
#define TOSUNRISE		-175000L	// conteggio fino all'alba successiva (Inverno)
#define	TONOON			-221821L	// conteggio dall'alba a mezzogiorno (Inverno)

//#define	HTOSUNSET	4			// ore da mezzogiorno al tramonto (inverno)
//#define	HTOSUNRISE	20			// ore da mezzogiorno all'alba (inverno)

#define MAXLATDCNT	22700L			// conteggio della rotazione LATITUDINE
#define MAXPRECCNT	203000L			// conteggio della rotazione PREC
#define THSNDYR		7894			// unita' di conteggio per 1000 anni di PRECESS.

#define AR_OFFSET 	20.13			// zero dell'ascensione retta [h.d]
#define LATD_OFFSET 15.0			// zero della latitudine [gr.d]
#define	LATD_SIC	70.0			// latitudine di sicurezza [gr.d]
#define PREC_OFFSET 130.0			// zero della precessione [gr]
#define TIME_OFFSET 12.0			// zero del tempo [h.d]
#define	ON			1
#define	OFF			0

#define	HTOSUNSET	0				// indice per il vettore dei crepuscoli
#define	HTOSUNRISE	1				// indice per il vettore dei crepuscoli
#define	HTOEND		24				// ore da mezzogiorno alla fine

#define	MAXSOLE		4000			// massima luminosita' sole

#define PARAMS_DEFAULT 0			// gestione parametri di sistema
#define PARAMS_LOAD 1
#define PARAMS_SAVE 2
#define NPARAMS 12

#define EPS 0.00005					// < 1/(60*60) gradi = 0.000277 1/(100*100) dTog = 0.0001

// PROTOTIPI
void PlaMain();
double hms2d(double hms);
double d2hms(double dd);
int plazero(void);
int plasicur(void);
void sleep_ms(int ms);
void CalcTime(double ar, double arS, volatile int *thr, volatile int *tmn);
