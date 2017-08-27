/*
 * my_ajax.c --
 *
 *	Produce a page containing all FastCGI inputs
 *
 *
 * Copyright (c) 1996 Open Market, Inc.
 *
 * See the file "LICENSE.TERMS" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */
#ifndef lint
static const char rcsid[] = "$Id: my_ajax.c,v 2.0 2016oct29 17:17:00 RoV Exp $";
#endif /* not lint */

#include "fcgi_config.h"

#include <stdlib.h>
#include <string.h> // strlen
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>	// shm...
#include <sys/sem.h>	// sem...
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>
#include "../../PlaPI/SerDefs.h"
#include "../../PlaPI/Shared.h"
#include "../../PlaPI/PlaDefs.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#include <process.h>
#else
extern char **environ;
#endif

#include "fcgi_stdio.h"

// macro per costruire stringhe HTML-JSON
//#define JHEADER "Content-type: text/html\r\n\r\n{"
#define JHEADER "Cache-Control:no-cache, no-store\r\nContent-type: text/html\r\n\r\n{"
//#define JNAME(varName, varFormat) "\"varName\":\"varFormat\","
//#define JNAMELAST(varName, varFormat) "\"varName\":\"varFormat\"}\r\n"
#define STR(x) #x
#define JNAME(varName, varFormat) STR(#varName:varFormat) ","
#define JNAMELAST(varName, varFormat) STR(#varName:varFormat) "}\r\n"

// globals
int sharedId, semId;
volatile struct SHARED_DATA *pSharedData;	// pointer to shared memory

int assign_data(char *szIn);
double hms2d(double hms);
double d2hms(double hms);
double drange_limit(double in, double min, double max);
int range_limit(int in, int min, int max);

int main ()
{
	char szLoc[] = "my_ajax::main";
    char **initialEnv = environ;
    int count = 0;
	int rc;

	openlog("my_ajax", LOG_CONS | LOG_NDELAY, LOG_USER);
	//syslog(LOG_INFO, "starting log\n");

	// Connect shared memory
	if (!initSharedMemoryAndSemaphores())
		return -1;

	//syslog(LOG_INFO, "init shared memory OK\n");

	// inizializza stato planetario offline
	lock();
	pSharedData->PlaMode = MODE_OFFLINE;
	pSharedData->PlaSubMode = SUB_NONE;
	unlock();

	//syslog(LOG_INFO, "entering while loop\n");

    while (FCGI_Accept() >= 0)
	{
		//syslog(LOG_INFO, "processing message\n");
		if (!strncmp(getenv("REQUEST_METHOD"), "POST", 4))
		{
			// POST
			char *scriptName = getenv("SCRIPT_NAME");
			char *contentLength = getenv("CONTENT_LENGTH");
			int len=0;
			if (contentLength != NULL)
				len = strtol(contentLength, NULL, 10);

			if (strcmp(scriptName, "/planetario.fcgi") || len==0)
			{
				// script sbagliato o lunghezza nulla o non definita, ignoriamo
				printf(JHEADER JNAME(ScriptName, "%s") JNAMELAST(Result, "bad script name"), scriptName);
			}
			else
			{
				// script corretto, leggi il contenuto
				char szIn[80];
				int bFound = 0;
				// gestisci POST
				// leggere da stdin
				while (len)
				{
					gets(szIn);
					// cerca almeno un "=" nella riga
					if (strchr(szIn, '='))
					{
						bFound = 1;
						break;
					}
					len -= strlen(szIn) + 2;	// 2 for \r\n
				}
				if (!bFound)
				{
					// stringa con parametri assente
					printf(JHEADER JNAME(ScriptName, "%s") JNAMELAST(Result, "POST parameters missing"), scriptName);
				}
				else
				{
					int nRead;
					// copia i parametri disponibili
					lock();
					nRead = assign_data(szIn);
					unlock();
					printf(JHEADER JNAME(ScriptName, "%s") JNAME(nRead, %d) JNAMELAST(Result, "OK"), scriptName, nRead);
				}
			}
		}
		else
		{
			// assumiamo sia GET
			// potremmo avere argomento dopo "?"
			char *queryString = getenv("QUERY_STRING");
			char *scriptName = getenv("SCRIPT_NAME");

			if (strcmp(scriptName, "/planetario.fcgi"))
			{
				// altro script, non ci interessa più di tanto...
				printf(JHEADER JNAME(ScriptName, "%s") JNAMELAST(Result, "bad script name"), scriptName);
			}
			else
			{
				// ok, script corretto
				// fai una copia locale della struttura shared per minimizzare il tempo di accesso
				struct SHARED_DATA data;
				lock();
				memcpy(&data, (void *)pSharedData, sizeof(data));
				unlock();

				// rispondi secondo la query string
				// ignora caratteri dopo la stringa attesa, es. &_xxx
				//   che arrivano impostando ajax cache:off
				// mainset: tutto tranne luci e auxiliary
				if (!strncmp(queryString, "mainset", 7))
				{
					// scrivi in JSON
					/*
					printf("Content-type: text/html\r\n\r\n"
						"{\"ScriptName\":\"%s\",\"QueryString\":\"%s\",",
						scriptName, queryString);
					printf("\"declinazione\":\"%ld\",\"ascRetta\":\"%ld\",\"precessione\":\"%ld\",",
						cmotc.declinazione, cmotc.ascRetta, cmotc.precessione);
					printf("\"bSearchZeroDecl\":\"%1d\",\"bSearchZeroAscR\":\"%1d\",\"bSearchZeroPrec\":\"%1d\"}\r\n",
						cmotc.bSearchZeroDecl, cmotc.bSearchZeroAscR, cmotc.bSearchZeroPrec);
					*/
					printf(JHEADER JNAME(ScriptName, "%s") JNAME(QueryString, "%s") JNAME(Result, "OK"), scriptName, queryString);
					printf(JNAME(PlaMode, %d) JNAME(SettingMode, %d) JNAME(PlaSubMode, %d), data.PlaMode, data.SettingMode, data.PlaSubMode);
					printf(JNAME(ar_now, %f) JNAME(ar_nowpr, %f) JNAME(latitude_now, %f), d2hms(data.ar_now), data.ar_nowpr, d2hms(data.latitude_now));
					printf(JNAME(prec_now, %f) JNAME(prec_now_year, %f) JNAME(annoPrecessione, %f), data.prec_now, data.prec_now_year, data.annoPrecessione);
					printf(JNAME(time_hour, %d) JNAME(time_min, %d), data.time_hour, data.time_min);
					printf(JNAME(idError, %d) JNAME(bPaused, %d) JNAME(bZeroDone, %d), data.idError, data.bPaused, data.bZeroDone);
					printf(JNAME(bNAKlat, %d) JNAME(bNAKascr, %d), JNAME(bNAKprec, %d), data.bNAKlat, data.bNAKascr, data.bNAKprec);
					printf(JNAMELAST(perc_action, %d), data.perc_action);
				}
				else if (!strncmp(queryString, "luci", 4))
				{
					printf(JHEADER JNAME(ScriptName, "%s") JNAME(QueryString, "%s") JNAME(Result, "OK"), scriptName, queryString);
					printf(JNAME(sole, %d) JNAME(luna, %d) JNAME(stelle, %d), data.luci.sole, data.luci.luna, data.luci.stelle);
					printf(JNAME(cerchio_orario, %d) JNAME(supernova, %d) JNAME(p_nebula, %d), data.luci.cerchio_orario, data.luci.supernova, data.luci.p_nebula);
					printf(JNAME(nuove_s, %d) JNAME(a_libero, %d) JNAME(alba, %d), data.luci.nuove_s, data.luci.a_libero, data.luci.alba);
					printf(JNAME(giorno, %d) JNAME(tramonto, %d) JNAME(pianeti, %d), data.luci.giorno, data.luci.tramonto, data.luci.pianeti);
					printf(JNAME(declinazione, %d) JNAME(p_cardinali, %d) JNAME(luce_sala, %d), data.luci.declinazione, data.luci.p_cardinali, data.luci.luce_sala);
					printf(JNAME(cerchio_polare, %d) JNAME(d_libero1, %d) JNAME(d_libero2, %d), data.luci.cerchio_polare, data.luci.d_libero1, data.luci.d_libero2);
					printf(JNAMELAST(d_libero3, %d), data.luci.d_libero3);
				}
				else if (!strncmp(queryString, "auxiliary", 9))
				{
					printf(JHEADER JNAME(ScriptName, "%s") JNAME(QueryString, "%s") JNAME(Result, "OK"), scriptName, queryString);
					printf(JNAME(sun_ar, %f) JNAME(moon_ar, %f) JNAME(mercury_ar, %f), d2hms(data.sun_ar), d2hms(data.moon_ar), d2hms(data.mercury_ar));
					printf(JNAME(venus_ar, %f) JNAME(mars_ar, %f) JNAME(jupiter_ar, %f), d2hms(data.venus_ar), d2hms(data.mars_ar), d2hms(data.jupiter_ar));
					printf(JNAME(saturn_ar, %f) JNAME(latitude, %f), d2hms(data.saturn_ar), d2hms(data.latitude));
					printf(JNAME(horlight, %d) JNAME(maxstars, %d) JNAMELAST(speed, %d), data.horlight, data.maxstars, data.speed);
				}
				else if (!strncmp(queryString, "test", 4))
				{
					printf(JHEADER JNAME(ScriptName, "%s") JNAME(QueryString, "%s") JNAME(Result, "OK"), scriptName, queryString);
					printf(JNAME(declinazione, %ld) JNAME(ascRetta, %ld) JNAME(ascRettaOffset, %ld) JNAME(precessione, %ld), data.motori.declinazione, data.motori.ascRetta, data.motori.ascRettaOffset, data.motori.precessione);
					printf(JNAME(bSearchZeroDecl, %d) JNAME(bSearchZeroAscR, %d) JNAME(bSearchZeroPrec, %d), data.motori.bSearchZeroDecl, data.motori.bSearchZeroAscR, data.motori.bSearchZeroPrec);
					printf(JNAME(motorSpeed, %d) JNAME(motorTarget, %ld) JNAMELAST(motorId, %d), data.motorSpeed, data.motorTarget, data.motorId);
				}
				else
				{
					printf(JHEADER JNAME(ScriptName, "%s") JNAME(QueryString, "%s") JNAMELAST(Result, "bad query string"), scriptName, queryString);
				}
			}
		}

    } /* while */

	closelog();
    return 0;
}


BOOL initSharedMemoryAndSemaphores()
{
	const char *szLoc = "my_ajax::initSharedMemoryAndSemaphores()";
	semun_t arg;

	// create a new shared memory segment
	sharedId = shmget(SHARED_KEY, sizeof(volatile struct SHARED_DATA), 0666 | IPC_CREAT | S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
	if (sharedId == -1)
	{
		syslog(LOG_INFO, "%s: shared memory init error=%s\n", szLoc, strerror(errno));
		return FALSE;
	}

	// attach the shared memory segment
	pSharedData = (volatile struct SHARED_DATA *)shmat(sharedId, 0, 0);
	// safety check
	if ((int)pSharedData == -1)
	{
		syslog(LOG_INFO, "%s: shared memory initialization error=%s\n", szLoc, strerror(errno));
		// this is a pointer, set it to NULL
		pSharedData = 0;
		return FALSE;
	}

	// reset shared structure
	memset((void *)pSharedData, 0, sizeof(struct SHARED_DATA));

	// create binary semaphore to manage the accesses at the shared memory
	semId = semget(SEM_KEY, 3, 0666 | IPC_CREAT);
	if (semId == -1)
	{
		syslog(LOG_INFO, "%s: semaphore allocation error=%s\n", szLoc, strerror(errno));
		return FALSE;
	}

	arg.val = 1;
	if (semctl(semId, 0, SETVAL, arg) == -1)
	{
		syslog(LOG_INFO, "%s: binary_semaphore_initialization: error=%s\n", szLoc, strerror(errno));
		return FALSE;
	}

	return TRUE;
}


int lock()
{
	struct sembuf op;
	op.sem_num = 0;
	op.sem_op = -1;
	op.sem_flg = SEM_UNDO;
	return semop(semId, &op, 1);
}


int unlock()
{
	struct sembuf op;
	op.sem_num = 0;
	op.sem_op = 1;
	op.sem_flg = SEM_UNDO;
	return semop(semId, &op, 1);
}


// leggi i parametri dalla stringa, torna il numero convertito
// formato <nome>=<valore>&<nome>=<valore>&...<nome>=<valore>\r\n
// attenzione: modifica la stringa!
// torna il numero di parametri convertiti
int assign_data(char *szIn)
{
	char *ps, *pEq;
	int np = 0;
	ps = strtok(szIn, "&\r");
	while (ps)
	{
		// deve esserci un '='
		pEq = strchr(ps, '=');
		if (pEq)
		{
			np++;
			pEq++;	// punta all'argomento dopo '='
			if (strstr(ps, "bAbort"))
				pSharedData->bAbort = (atoi(pEq) != 0);
			else if (strstr(ps, "bSuspend"))
				pSharedData->bSuspend = (atoi(pEq) != 0);
			else if (strstr(ps, "bContinue"))
				pSharedData->bContinue = (atoi(pEq) != 0);
			else if (strstr(ps, "bResetError"))
				pSharedData->bResetError = (atoi(pEq) != 0);
			else if (strstr(ps, "NewMode"))
				pSharedData->NewMode = atoi(pEq);
			else if (strstr(ps, "NewSettingMode"))
				pSharedData->NewSettingMode = atoi(pEq);
			else if (strstr(ps, "lightToBeChanged"))
				pSharedData->lightToBeChanged = atoi(pEq);
			else if (strstr(ps, "lightLevel"))
				pSharedData->lightLevel = atoi(pEq);
			else if (strstr(ps, "annoPrecessione"))
				pSharedData->annoPrecessione = drange_limit(atof(pEq), 0, 10000);
			else if (strstr(ps, "latitude"))
				pSharedData->latitude = drange_limit(hms2d(atof(pEq)), -90, 90);
			else if (strstr(ps, "horlight"))
				pSharedData->horlight = range_limit(atoi(pEq), 0, 255);
			else if (strstr(ps, "maxstars"))
				pSharedData->maxstars = range_limit(atoi(pEq), 50, 1000);
			else if (strstr(ps, "speed"))
				pSharedData->speed = range_limit(atoi(pEq), 10, 500);
			else if (strstr(ps, "sun_ar"))
				pSharedData->sun_ar = drange_limit(hms2d(atof(pEq)), 0, 24);
			else if (strstr(ps, "moon_ar"))
				pSharedData->moon_ar = drange_limit(hms2d(atof(pEq)), 0, 24);
			else if (strstr(ps, "mercury_ar"))
				pSharedData->mercury_ar = drange_limit(hms2d(atof(pEq)), 0, 24);
			else if (strstr(ps, "venus_ar"))
				pSharedData->venus_ar = drange_limit(hms2d(atof(pEq)), 0, 24);
			else if (strstr(ps, "mars_ar"))
				pSharedData->mars_ar = drange_limit(hms2d(atof(pEq)), 0, 24);
			else if (strstr(ps, "jupiter_ar"))
				pSharedData->jupiter_ar = drange_limit(hms2d(atof(pEq)), 0, 24);
			else if (strstr(ps, "saturn_ar"))
				pSharedData->saturn_ar = drange_limit(hms2d(atof(pEq)), 0, 24);
			else if (strstr(ps, "bAllLightsOff"))
				pSharedData->bAllLightsOff = (atoi(pEq) != 0);
			else if (strstr(ps, "bAllLightsOn"))
				pSharedData->bAllLightsOn = (atoi(pEq) != 0);
			else if (strstr(ps, "bTestZero"))
				pSharedData->bTestZero = (atoi(pEq) != 0);
			else if (strstr(ps, "bTestMove"))
				pSharedData->bTestMove = (atoi(pEq) != 0);
			else if (strstr(ps, "motorSpeed"))
				pSharedData->motorSpeed = atoi(pEq);
			else if (strstr(ps, "motorTarget"))
				pSharedData->motorTarget = atol(pEq);
			else if (strstr(ps, "motorId"))
				pSharedData->motorId = atoi(pEq);
			// solo per test: scrive una variabile che possiamo anche leggere
//			else if (strstr(ps, "time_hour"))
//				pSharedData->time_hour = atoi(pEq);
			else
				np--;
		}
		// prossimo token
		ps = strtok(NULL, "&\r");
	}

	return np;
}


double d2hms(double dd)
{
	int		hours, mins, secs;
	double	decimals;

	hours = (int)dd;
	decimals = (dd - hours) * 60;
	mins = (int)(decimals + EPS);
	secs = (int)((decimals - mins) * 60 + EPS);
	return hours + mins / 100.0 + secs / 10000.0;
}


double hms2d(double hms)
{
	int 	hours, mins, secs;
	double	decimals;

	hours = (int)hms;
	decimals = (hms - hours) * 100;
	mins = (int)(decimals + EPS);
	secs = (int)((decimals - mins) * 100 + EPS);
	return hours + mins / 60.0 + secs / 3600.0;
}


double drange_limit(double in, double min, double max)
{
	if (in < min)
		return min;
	if (in > max)
		return max;
	return in;
}

int range_limit(int in, int min, int max)
{
	if (in < min)
		return min;
	if (in > max)
		return max;
	return in;
}