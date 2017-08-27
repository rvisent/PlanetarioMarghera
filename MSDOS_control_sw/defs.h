#define	ESC					0x1B
#define	XESC				0x011B
#define SLETTER     0x1F73
#define	F1					0x3B00
#define	F2					0x3C00
#define	F3					0x3D00
#define	F4					0x3E00
#define	F5					0x3F00
#define	F6					0x4000
#define	F7					0x4100
#define	F8					0x4200
#define F9          0x4300
#define F10         0x4400
#define	UPARW				0x4800
#define	DNARW				0x5000
#define	LFARW				0x4B00
#define	RGARW				0x4D00
#define	CLFARW	  	0x7300
#define	CRGARW			0x7400
#define	TAB					0x0F09
#define SPACEBAR    0x3920

#define SYSON				0
#define SYSOFF			1
#define STARSON			2
#define STARSOFF		3

#define	RUN					0
#define	STOP				1
#define	END					2

#define	CNT_LATD		0
#define CNT_AR			1
#define	CNT_PREC		2

#define	mainwind		1,1,80,25
#define	analogwind	1,3,40,22
#define	digitalwind	41,3,80,22
#define	functwind   1,24,80,25
#define	titlewind		1,1,80,1
#define statuswind	1,23,80,23
#define	WINPUP			RED,BLACK,DARKGRAY,TWO_LINE

//setup per schermo in alternativa
//#define	FLASH				textcolor(BLINK|BLACK)
//#define NOFLASH			textcolor(BLACK);textbackground(WHITE)
//#define REVERSE			textcolor(WHITE);textbackground(BLACK)

//setup per schermo laptop
//#define	FLASH				textcolor(BLINK|WHITE)
//#define NOFLASH			textcolor(WHITE);textbackground(BLACK)
//#define REVERSE			textcolor(BLACK);textbackground(WHITE)

//setup per schermo a colori
#define	FLASH				textcolor(BLINK|RED);textbackground(DARKGRAY)
#define NOFLASH			textcolor(RED);textbackground(BLACK)
#define REVERSE			textcolor(BLACK);textbackground(RED)

#define	TSIM				3000			//in decimi di sec. - simulazione transizioni

#define MAXARCNT		233496		//conteggio della rotazione AR

#define ONETURN			220000		//conteggio di una rotazione simulata
#define	TOSUNSET		-30000		//conteggio fino al tramonto (Inverno)
#define TOSUNRISE		-175000		//conteggio fino all'alba successiva (Inverno)
#define	TONOON			-221821		//conteggio dall'alba a mezzogiorno (Inverno)

//#define	HTOSUNSET		4 				//ore da mezzogiorno al tramonto (inverno)
//#define	HTOSUNRISE	20				//ore da mezzogiorno all'alba (inverno)

#define MAXLATDCNT	22700			//conteggio della rotazione LATITUDINE
#define MAXPRECCNT	203000		//conteggio della rotazione PREC
#define THSNDYR			7894			//unita' di conteggio per 1000 anni di PRECESS.

#define AR_OFFSET 	20.13			//zero dell'ascensione retta [h.d]
#define LATD_OFFSET 15.0			//zero della latitudine [gr.d]
#define	LATD_SIC		70.0			//latitudine di sicurezza [gr.d]
#define PREC_OFFSET 130				//zero della precessione [gr]
#define TIME_OFFSET 12.0			//zero del tempo [h.d]
#define	ON					1
#define	OFF					0

#define	HTOSUNSET		0					//indice per il vettore dei crepuscoli
#define	HTOSUNRISE	1					//indice per il vettore dei crepuscoli
#define	HTOEND			24				//ore da mezzogiorno alla fine

//PROTOTIPI

void	updatescreen(void);
int 	read_setup(void);
void	setupscreen(void);
void	beep_high(void);
void	beep_low(void);
float d2hms(float dd);
float hms2d(float hms);
void	CalcTime(float ar,float arS,int *thr,int *tmn,float *tcalc);
int 	read_setup(void);
void 	init_popup(void);
int 	next_popup(int left,int top,int right,int bottom,int foreground,
				int background,int border,int bordertype);
int 	prev_popup(void);
void 	plazero(void);
int 	mousebt(void);
void 	sess_rec(int rec_mode);

#ifdef MAIN
	float		sun_ar,moon_ar,mercury_ar,venus_ar,mars_ar,
					jupiter_ar,saturn_ar,latitude;
	int			f_ver,horlight,maxstars,speed;
	struct	chs
	{
		int	x,y;
	}	STELLE_STAT,SOLE_STAT,LUNA_STAT,LUNA1_STAT,PIANETI_STAT,CERCHI_STAT,
	POLO_STAT,TIME_STAT,LATD_STAT,PREC_STAT,STAT_STAT,AR_STAT,SSET_STAT,
	SRISE_STAT,DAYL_STAT,RUN_STAT,YEAR_STAT;
#else
	extern float		sun_ar,moon_ar,mercury_ar,venus_ar,mars_ar,
									jupiter_ar,saturn_ar,latitude;
	extern int			f_ver,horlight,maxstars,speed;
	extern struct	chs
				 {
					 int	x,y;
				 } STELLE_STAT,SOLE_STAT,LUNA_STAT,LUNA1_STAT,PIANETI_STAT,
				 CERCHI_STAT,POLO_STAT,TIME_STAT,LATD_STAT,PREC_STAT,STAT_STAT,
				 AR_STAT,SSET_STAT,SRISE_STAT,DAYL_STAT,RUN_STAT,YEAR_STAT;
#endif
