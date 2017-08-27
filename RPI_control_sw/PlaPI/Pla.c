// versione per Raspberry PI RV2014mar29
//modifica del 20gen2006 val max stelle in setup
//10MAR2006: eliminato il controllo con il mouse

//24NOV2013: modifiche per nuove transizioni a LED


#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>	// system()

#include "SerDefs.h"
#include "PlaDefs.h"
#include "Shared.h"


// prototipi locali
void setLight(LIGHT_ID lightToBeChanged, int lightLevel);
int setPlanets(int opModeSettings);
int run();
void stop();
int test();
int check_handle_abort();
int manageParams(int command);


void PlaMain()
{
	int rc;

	// inizializzazioni
	// intanto azzera tutto
	memset((void *)pSharedData, 0, sizeof(struct SHARED_DATA));

	// poi prova a caricare l'ultima configurazione salvata
	if (manageParams(PARAMS_LOAD) == -1)
	{
		// load default
		manageParams(PARAMS_DEFAULT);

		// segnala errore
		lock();
		pSharedData->idError = ERR_PARAMS_DEFAULTED;
		unlock();
	}

	// entra nel loop infinito dell'applicazione
	while (1)
	{
		MODE opMode;
		SETMODE opModeSettings;
		long ascretta_raw;

		lock();

		// se usciamo da un comando o riceviamo un abort, reset delle luci e dello stato
		// eccezioni: se usciamo da setPlanets mantiene le luci (fino ad un abort)
		//   dopo SAFEPOS spegne tutto (SHUTDOWN non piu' usato)
		//   solo dopo RUN lascia accesi i punti cardinali e il sole
		if (pSharedData->PlaMode != MODE_IDLE || pSharedData->bAbort)
		{
			switch (pSharedData->PlaMode)
			{
			//case MODE_OFFLINE:	// non raggiungibile
			//case MODE_SETPLANETS:	// uscendo da setPlanets preserva le luci e il SettingMode
			//case MODE_SHUTDOWN:	// non raggiungibile
			//case MODE_SAVE:		// non interagisce con le luci
			//case MODE_DEFAULT:	// non interagisce con le luci
			case MODE_IDLE:			// solo in caso di abort 
			case MODE_RUN:
			case MODE_ZERO:
			case MODE_TEST:
			case MODE_SAFEPOS:
				pthread_mutex_lock(&mutex_ser);
				// spegni le luci tranne alcune
				memset((void *)(&stato), 0, sizeof(struct SERVIZI));
				stato.giorno = (pSharedData->PlaMode == MODE_SAFEPOS) ? 0:250;
				stato.alba = 5;
				stato.tramonto = 5;
				stato.sole = (pSharedData->PlaMode == MODE_RUN) ? MAXSOLE:0;
				stato.p_cardinali = (pSharedData->PlaMode == MODE_RUN) ? ON:OFF;
				pthread_mutex_unlock(&mutex_ser);
				pSharedData->SettingMode = SET_SOLE;
				break;
			}

			// entriamo in MODE_IDLE, metti default sui sottocampi
			pSharedData->PlaMode = MODE_IDLE;
			pSharedData->PlaSubMode = SUB_NONE;
			pSharedData->lightToBeChanged = LIGHT_NONE;
		}

		// gestisci i comandi da operatore
		if (pSharedData->bResetError)
		{
			// azzera eventuale errore memorizzato
			pSharedData->idError = 0;
			pSharedData->bNAKascr = FALSE;
			pSharedData->bNAKlat = FALSE;
			pSharedData->bNAKprec = FALSE;
			pSharedData->bResetError = FALSE;
		}
		// aggiorna luci se richiesto
		setLight(pSharedData->lightToBeChanged, pSharedData->lightLevel);
		pSharedData->lightToBeChanged = LIGHT_NONE;
		// in questo punto abort, suspend, continue non hanno effetto
		pSharedData->bAbort = FALSE;
		pSharedData->bSuspend = FALSE;
		pSharedData->bContinue = FALSE;
		pSharedData->bPaused = FALSE;
		// vedi se è richiesto un nuovo modo operativo, altrimenti resta idle
		// non accetta comandi finche' ci sono errori non resettati
		if (pSharedData->NewMode != MODE_OFFLINE && pSharedData->NewMode != MODE_IDLE && !pSharedData->idError)
		{
			opMode = pSharedData->NewMode;
			// resetta la richiesta
			pSharedData->NewMode = MODE_IDLE;

			// copia per interfaccia remota
			pSharedData->PlaMode = opMode;

			// se setPlanets, leggi la sotto-modalita'
			if (opMode == MODE_SETPLANETS)
			{
				opModeSettings = pSharedData->NewSettingMode;
				pSharedData->SettingMode = opModeSettings;
			}
		}
		else
			opMode = MODE_IDLE;

		unlock();

		// copia ascensione retta dell'encoder (non compensata da offset)
		pthread_mutex_lock(&mutex_ser);
		ascretta_raw = motc1.encY;
		pthread_mutex_unlock(&mutex_ser);

		// esegui!
		switch (opMode)
		{
		case MODE_IDLE:
			// se siamo idle, aggiorna dati real-time e torna al loop con attesa 50 ms
			updateData();
			sleep_ms(50);
			break;

		case MODE_SETPLANETS:
			// se non c'e' stato un azzeramento prima, eseguilo adesso
			// azzera anche se AR superiore a 2 giri della macchina
			//   (non 4 come run, qui vogliamo alta precisione di posizionamento)
			if (!pSharedData->bZeroDone || labs(ascretta_raw) > 2*MAXARCNT)	// non serve lock, lettura di variabile non modificabile da fuori
				plazero();
			if (pSharedData->bZeroDone)
				// per setPlanets occorre un azzeramento ok
				// se non ha azzerato, il codice di errore e' gia' stato settato da plazero()
				// reitera se torna 1 (astro cambiato)
				while (setPlanets(opModeSettings) == 1)
				{
					// aggiorna opModeSettings
					lock();
					opModeSettings = pSharedData->NewSettingMode;
					pSharedData->SettingMode = opModeSettings;
					unlock();
				}
			break;

		case MODE_RUN:
			// se non c'e' stato un azzeramento prima, eseguilo adesso
			// azzera anche se AR superiore a 4 giri della macchina per evitare overflow del controllore
			//   ed errori dovuti alla tolleranza su MAXARCNT
			if (!pSharedData->bZeroDone || labs(ascretta_raw) > 4*MAXARCNT)	// non serve lock, lettura di variabile non modificabile da fuori
				plazero();
			if (pSharedData->bZeroDone)
				// per run occorre un azzeramento ok
				// se non ha azzerato, il codice di errore e' gia' stato settato da plazero()
				run();
			break;

		case MODE_ZERO:
			plazero();
			break;

		case MODE_SAFEPOS:
			if (pSharedData->bZeroDone)
				// per safepos occorre un azzeramento ok
				plasicur();
			else
			{
				// se non siamo azzerati, torna errore
				// sarebbe inutile forzare un azzeramento e poi un safe, e' molto
				//   probabile che se non siamo stati azzerati siamo gia' in posizione sicura
				lock();
				pSharedData->idError = ERR_PLAMAIN_NOTZEROED;
				unlock();
			}
			break;

		case MODE_TEST:
			test();
			break;

		case MODE_SHUTDOWN:
			// lascia solo luce giorno fioca
			pthread_mutex_lock(&mutex_ser);
			memset((void *)(&stato), 0, sizeof(struct SERVIZI));
			stato.giorno = 5;
			pthread_mutex_unlock(&mutex_ser);
			printf("System shutdown requested: wait one second...\n");
			fflush(stdout);	// altrim. non scrive su file con la redirezione
			// dai tempo 1 s per l'esecuzione e per la visualizzaz. del messaggio
			sleep_ms(1000);
			while ((rc=system("sudo init 0")) == -1)
			{
				printf("shutdown failed: will wait 2 s and retry...\n");
				fflush(stdout);	// altrim. non scrive su file con la redirezione
				sleep_ms(2000);
			}
			// esci
			return;

		case MODE_SAVE:
			manageParams(PARAMS_SAVE);
			break;

		case MODE_DEFAULT:
			manageParams(PARAMS_DEFAULT);
			break;
		}
	}
}


int setPlanets(int opModeSettings)
{
	double ar_target, lat_target, prec_target;
	BOOL bStop, bTargetChanged;
	int test_enc1_cnt = 0;

	// accendi le luci che ci possono servire
	pthread_mutex_lock(&mutex_ser);
	stato.declinazione = ON;
	stato.cerchio_polare = ON;
	stato.sole = MAXSOLE;
	stato.stelle = pSharedData->maxstars;
	stato.giorno = 15;
	stato.p_cardinali = ON;
	stato.pianeti = ON;
	stato.luna = 4095;

	// carica l'AR del corpo celeste che ci interessa
	lock();
	switch (opModeSettings)
	{
		case SET_SOLE: ar_target = pSharedData->sun_ar; break;
		case SET_LUNA: ar_target = pSharedData->moon_ar; break;
		case SET_MERCURIO: ar_target = pSharedData->mercury_ar; break;
		case SET_VENERE: ar_target = pSharedData->venus_ar; break;
		case SET_MARTE: ar_target = pSharedData->mars_ar; break;
		case SET_GIOVE: ar_target = pSharedData->jupiter_ar; break;
		case SET_SATURNO: ar_target = pSharedData->saturn_ar; break;
		default:
			// codice non valido
			pSharedData->idError = ERR_SETTINGS_OUTOFRANGE;
			unlock();
			return -1;
	}
	pSharedData->idError = 0;	// per sicurezza, ma e' gia' garantito in PlaMain()
	lat_target = pSharedData->latitude;
	prec_target = (pSharedData->annoPrecessione - 2000) / 72.22;

	// aggiorna modo operativo (può essere alterato da un plazero()
	pSharedData->PlaMode = MODE_SETPLANETS;
	pSharedData->PlaSubMode = SUB_NONE;
	unlock();

	// imposta offset dell'encoder di ascensione retta al giro piu' vicino al punto di partenza (come run)
	pthread_mutex_unlock(&mutex_ser);
	motc.ascRettaOffset = lround((double)motc1.encY / MAXARCNT + (ar_target - AR_OFFSET) / 24.0) * MAXARCNT;
	pthread_mutex_unlock(&mutex_ser);

	// posiziona
	if (!SerCmdGotoEncSync(CNT_AR, (long)(-MAXARCNT*(ar_target - AR_OFFSET) / 24.), 100, 5))
	{
		lock();
		pSharedData->bNAKascr = TRUE;
		pSharedData->idError = ERR_SETTINGS_GOTOENC;
		unlock();
	}
	if (!SerCmdGotoEncSync(CNT_LATD, (long)(-MAXLATDCNT*(lat_target - LATD_OFFSET) / 360.), 5, 5))
	{
		lock();
		pSharedData->bNAKlat = TRUE;
		pSharedData->idError = ERR_SETTINGS_GOTOENC;
		unlock();
	}
	if (!SerCmdGotoEncSync(CNT_PREC, (long)(MAXPRECCNT*(prec_target - PREC_OFFSET) / 360.), 100, 5))
	{
		lock();
		pSharedData->bNAKprec = TRUE;
		pSharedData->idError = ERR_SETTINGS_GOTOENC;
		unlock();
	}

	// esci in caso di errori
	if (pSharedData->idError)
	{
		stop();
		return -1;
	}

	bTargetChanged = FALSE;
	// loop interrompibile di raggiungimento posizione
	// bande ampie, tanto il movimento completa anche se usciamo
	while ((fabs(pSharedData->ar_now - ar_target) > 0.5 || fabs(pSharedData->latitude_now - lat_target) > 0.5
		|| fabs(pSharedData->prec_now) > 5) && !pSharedData->bAbort && !bTargetChanged)
	{
		// cedi un po' di CPU
		sleep_ms(50);

		if (!test_enc1_cnt)
		{
			test_enc1_cnt = 20;	// stampa ogni 1 s per test
			printf("(SET_PL) motc1.encY=%ld, ascRetta=%ld, ascRettaOffset=%ld\n", motc1.encY, motc.ascRetta, motc.ascRettaOffset);
			fflush(stdout);	// altrim. non scrive su file con la redirezione
		}
		else
			test_enc1_cnt--;

		// aggiorna dati in real-time in shared memory
		updateData();

		// vedi se nel frattempo è stato cambiato il target
		lock();
		bTargetChanged = (pSharedData->NewSettingMode != opModeSettings);
		unlock();
	}

	// se siamo usciti con abort prima del raggiungimento posizione, ritorna errore
	if (pSharedData->bAbort)
	{
		stop();
		lock();
		pSharedData->idError = ERR_ABORTED;
		unlock();
		// il flag abort viene cancellato all'uscita, nel main loop
		return -1;
	}

	// i flag abort/continue/paused vengono cancellati all'uscita, nel main loop
	// tutto ok o target cambiato
	return bTargetChanged ? 1:0;
}


int run()
{
	double ar_target, lat_target, prec_target, ar_stored, ar_initial;
	double annoCorrente;
	int runStatus, finishCounter, hindex;
	long TCounter;
	WORD ar_speed;
	BOOL bMotion, bPauseAtEnd, bsetAR, bsetLAT, bsetPREC;
	int test_enc1_cnt = 0;

	const int hrtogo[12][2] = { { 6, 18 }, { 7, 17 }, { 8, 16 }, { 8, 16 }, { 8, 17 }, { 7, 17 }, { 6, 18 },
	{ 5, 19 }, { 4, 19 }, { 5, 20 }, { 6, 19 }, { 6, 18 } };

	// imposta offset dell'encoder di ascensione retta al giro piu' vicino al punto di partenza (v. anche setPlanets)
	// attenzione, encY ha segno opposto rispetto al comando AR, per cui la differenza va fatta col '+'
	pthread_mutex_lock(&mutex_ser);
	motc.ascRettaOffset = lround((double)motc1.encY / MAXARCNT + (pSharedData->sun_ar - AR_OFFSET) / 24.0) * MAXARCNT;
	pthread_mutex_unlock(&mutex_ser);

	// aggiorna variabili allo stato corrente
	updateData();

	// il ciclo run consiste in 6 stati: posizionamento iniziale, pomeriggio, tramonto, notte, alba, posiz. finale
	for (runStatus = 0; runStatus < 6; runStatus++)
	{
		// imposta lo stato visibile all'esterno + attributi del ciclo
		// comunque solo attivita' veloci compatibili col lock()
		lock();
		switch (runStatus)
		{
		case 0:
			pSharedData->PlaMode = MODE_RUN;	// potrebbe essere stato alterato da plazero()
			pSharedData->PlaSubMode = POSIZIONAMENTO_INIZ;
			ar_target = pSharedData->sun_ar;
			lat_target = pSharedData->latitude;
			annoCorrente = pSharedData->annoPrecessione;
			prec_target = (annoCorrente - 2000) / 72.22;
			ar_speed = 100;
			bMotion = TRUE;
			bPauseAtEnd = TRUE;
			bsetAR = bsetLAT = bsetPREC = TRUE;
			// calcola hindex (qui e non all'inizio, potrebbe essere variata sun_ar)
			hindex = (int)((pSharedData->sun_ar + 0.5) / 2);
			if (hindex == 12)
				hindex = 0;
			// imposta stato iniziale luci
			pthread_mutex_lock(&mutex_ser);
			memset((void *)(&stato), 0, sizeof(struct SERVIZI));
			stato.giorno = 250;
			stato.p_cardinali = ON;
			pthread_mutex_unlock(&mutex_ser);
			break;
		case 1:
			pSharedData->PlaSubMode = RUN_POMERIGGIO;
			ar_target = pSharedData->sun_ar + (double)hrtogo[hindex][HTOSUNSET];
			ar_speed = 100;
			bMotion = TRUE;
			bPauseAtEnd = FALSE;
			bsetAR = TRUE;
			break;
		case 2:
			pSharedData->PlaSubMode = RUN_TRAMONTO;
			bMotion = FALSE;
			bPauseAtEnd = TRUE;
			break;
		case 3:
			pSharedData->PlaSubMode = RUN_NOTTE;
			ar_target = pSharedData->sun_ar + (double)hrtogo[hindex][HTOSUNRISE];
			ar_speed = pSharedData->speed;
			bMotion = TRUE;
			bPauseAtEnd = TRUE;
			bsetAR = TRUE;
			break;
		case 4:
			pSharedData->PlaSubMode = RUN_ALBA;
			bMotion = FALSE;
			bPauseAtEnd = FALSE;
			break;
		case 5:
			pSharedData->PlaSubMode = POSIZIONAMENTO_FIN;
			ar_target = pSharedData->sun_ar + HTOEND;
			ar_speed = 100;
			bMotion = TRUE;
			bPauseAtEnd = FALSE;	// cambiato RV21feb2015, PRONTO_TERMINARE era una pausa inutile
			bsetAR = TRUE;
			// spegni le luci tranne giorno, sole e punti cardinali
			pthread_mutex_lock(&mutex_ser);
			memset((void *)(&stato), 0, sizeof(struct SERVIZI));
			stato.giorno = 250;
			stato.sole = MAXSOLE;
			stato.p_cardinali = ON;
			pthread_mutex_unlock(&mutex_ser);
			break;
		}
		pSharedData->perc_action = 0;	// azzera percentuale azione in corso
		ar_stored = ar_target;			// valore finale del posizionamento
		unlock();

		// attiva motori se richiesto
		if (bMotion)
		{
			ar_initial = pSharedData->ar_now;	// per aggiornamento %

			// loop interrompibile di aggiornamento posizione assi fino alla fine dello stato
			for (finishCounter = 0; finishCounter < 3;)
			{
				// mostra info a console (temporaneo)
				if (bsetAR)
				{
					printf("RUNSTATUS=%d, hindex=%d, AR_TARGET = %f\n", runStatus, hindex, ar_target);
					fflush(stdout);	// altrim. non scrive su file con la redirezione
				}
				if (!test_enc1_cnt)
				{
					test_enc1_cnt = 20;	// stampa ogni 1 s per test
					printf("(RUN) motc1.encY=%ld, ascRetta=%ld, ascRettaOffset=%ld\n", motc1.encY, motc.ascRetta, motc.ascRettaOffset);
					fflush(stdout);	// altrim. non scrive su file con la redirezione
				}
				else
					test_enc1_cnt--;

				if (bsetAR && !SerCmdGotoEncSync(CNT_AR, (long)(-MAXARCNT*(ar_target - AR_OFFSET) / 24.0), ar_speed, 15))
				{
					lock();
					pSharedData->bNAKascr = TRUE;
					pSharedData->idError = ERR_RUN_GOTOENC;
					unlock();
				}
				bsetAR = FALSE;
				if (bsetLAT && !SerCmdGotoEncSync(CNT_LATD, (long)(-MAXLATDCNT*(lat_target - LATD_OFFSET) / 360.), 5, 15))
				{
					lock();
					pSharedData->bNAKlat = TRUE;
					pSharedData->idError = ERR_RUN_GOTOENC;
					unlock();
				}
				bsetLAT = FALSE;
				if (bsetPREC && !SerCmdGotoEncSync(CNT_PREC, (long)(MAXPRECCNT*(prec_target - PREC_OFFSET) / 360.), 100, 15))
				{
					lock();
					pSharedData->bNAKprec = TRUE;
					pSharedData->idError = ERR_RUN_GOTOENC;
					unlock();
				}
				bsetPREC = FALSE;

				// esci in caso di errori
				if (pSharedData->idError)
				{
					stop();
					return -1;
				}

				// cedi un po' di CPU
				sleep_ms(50);

				// aggiorna dati in real-time in shared memory
				updateData();

				// se abort, esci e ritorna errore
				if (pSharedData->bAbort)
				{
					stop();
					lock();
					pSharedData->idError = ERR_ABORTED;
					unlock();
					// il flag abort viene cancellato all'uscita, nel main loop
					return -1;
				}

				// se pausa, salva il target AR e forza la posizione corrente
				if (pSharedData->bSuspend)
				{
					lock();
					if (!pSharedData->bPaused)
					{
						//ar_stored = ar_target;
						ar_target = pSharedData->ar_now;
						bsetAR = TRUE;
						// aggiorna stato macchina in pausa
						pSharedData->bSuspend = FALSE;
						pSharedData->bPaused = TRUE;
					}
					else
						// ignora se siamo gia' in pausa
						pSharedData->bSuspend = FALSE;
					unlock();
				}

				// uscita da pausa
				if (pSharedData->bContinue)
				{
					lock();
					if (pSharedData->bPaused)
					{
						ar_target = ar_stored;
						bsetAR = TRUE;
						// aggiorna stato macchina uscita da pausa
						pSharedData->bContinue = FALSE;
						pSharedData->bPaused = FALSE;
					}
					else
						// ignora se non siamo in pausa
						pSharedData->bContinue = FALSE;
					unlock();
				}

				// se latitudine e/o anno di precessione sono cambiati, comanda movimento
				if (fabs(pSharedData->latitude - lat_target) > 0.2)
				{
					lat_target = pSharedData->latitude;
					bsetLAT = TRUE;
				}
				if (fabs(pSharedData->annoPrecessione - annoCorrente) > 10)
				{
					annoCorrente = pSharedData->annoPrecessione;
					prec_target = (annoCorrente - 2000) / 72.22;
					bsetPREC = TRUE;
				}

				// vedi se richiesta modifica ad una luce durante il ciclo
				if (pSharedData->lightToBeChanged != LIGHT_NONE)
				{
					setLight(pSharedData->lightToBeChanged, pSharedData->lightLevel);
					lock();
					pSharedData->lightToBeChanged = LIGHT_NONE;
					unlock();
				}

				// aggiorna % avanzamento SUBMODE (con arrotondamento)
				lock();
				pSharedData->perc_action = (int)(100*(pSharedData->ar_now - ar_initial) / (ar_stored - ar_initial)+0.5);
				unlock();

				// vedi se siamo alla fine del movimento
				// non in pausa, perche' usciremmo subito o quasi
				if (!pSharedData->bPaused && (fabs(pSharedData->ar_now - ar_target) < 0.5
					&& fabs(pSharedData->latitude_now - lat_target) < 0.5 && fabs(pSharedData->prec_now - prec_target) < 5))
					// richiede almeno 3 cicli con la condizione soddisfatta per affinare la posizione (vedi for)
					finishCounter++;
			} // for (finishCounter)
		} // if (bMotion)
		else
		{
			// !bMotion, ciclo luci
			// durata secondi max = TSIM*0.01
			for (TCounter = 0; TCounter <= TSIM; TCounter += (pSharedData->bPaused ? 0 : 1))
			{
				lock();
				// aggiorna % SUBMODE (con arrotondamento)
				pSharedData->perc_action = (int)((TCounter*100+TSIM/2)/TSIM);
				unlock();
				pthread_mutex_lock(&mutex_ser);

				// sono due casi, tramonto e alba
				if (runStatus == 2)
				{
					// tramonto
					// Il sole si spegne in un quinto del tempo di transizione (TSIM)
					if (TCounter < TSIM / 5)
						stato.sole = (WORD)(MAXSOLE - (TCounter*5*MAXSOLE/TSIM));
					else
						stato.sole = 0;

					// La luce del cielo si attenua impiegando tutto il tempo della transizione (TSIM)
					stato.giorno = (BYTE)(250 - TCounter*250 / TSIM);

					// imposta il valore minimo del fondo cielo
					if (stato.giorno < pSharedData->horlight)
						stato.giorno = (BYTE)(pSharedData->horlight);

					// progressione della luce del tramonto
					// La luce del tramonto inizia da subito e raggiunge il massimo ad 1/4 del tempo
					//	rimane al massimo fino a metà del tempo e poi diminuisce fino a zero alla fine
					//	del tempo di transizione
					if (TCounter <= (TSIM / 4))
						stato.tramonto = (BYTE)(TCounter*4*250 / TSIM);

					if (TCounter > TSIM / 2)
						stato.tramonto = (BYTE)(250 - (TCounter*2 - TSIM)*250 / TSIM);

					if (TCounter > (TSIM / 2))
						stato.stelle = (WORD)(((TCounter*2 - TSIM)*pSharedData->maxstars)/TSIM);
				}
				else
				{
					BYTE tempGiorno;

					// alba
					if (TCounter >= TSIM / 2)
						stato.sole = (WORD)((TCounter*2 - TSIM)*MAXSOLE/TSIM);

					tempGiorno = (BYTE)(TCounter*245 / TSIM + 5);
					if (tempGiorno > pSharedData->horlight)
						stato.giorno = tempGiorno;
					else
						stato.giorno = pSharedData->horlight;

					if (TCounter < TSIM / 2)
						// @@@@@@ lasciamo 245 e +5? Vedi soglie PWM su SerPla.c
						stato.alba = (BYTE)(TCounter*2*245 / TSIM + 5);
					else
						stato.alba = (BYTE)(250 - (TCounter*2 - TSIM)*250 / TSIM);

					stato.stelle = (WORD)(pSharedData->maxstars*(TSIM - TCounter) / TSIM);
				}
				pthread_mutex_unlock(&mutex_ser);

				// pausa 10 ms per temporizzare
				sleep_ms(10);

				// aggiorna dati in real-time in shared memory
				updateData();

				// se abort, esci e ritorna errore
				if (pSharedData->bAbort)
				{
					stop();
					lock();
					pSharedData->idError = ERR_ABORTED;
					unlock();
					// il flag abort viene cancellato all'uscita, nel main loop
					return -1;
				}

				// se pausa, setta il flag che blocca il conteggio TCounter
				if (pSharedData->bSuspend)
				{
					lock();
					if (!pSharedData->bPaused)
					{
						pSharedData->bSuspend = FALSE;
						pSharedData->bPaused = TRUE;
					}
					else
						// ignora se siamo gia' in pausa
						pSharedData->bSuspend = FALSE;
					unlock();
				}

				// uscita da pausa
				if (pSharedData->bContinue)
				{
					lock();
					if (pSharedData->bPaused)
					{
						pSharedData->bContinue = FALSE;
						pSharedData->bPaused = FALSE;
					}
					else
						// ignora se non siamo in pausa
						pSharedData->bContinue = FALSE;
					unlock();
				}

				// vedi se richiesta modifica ad una luce durante il ciclo
				if (pSharedData->lightToBeChanged != LIGHT_NONE)
				{
					setLight(pSharedData->lightToBeChanged, pSharedData->lightLevel);
					lock();
					pSharedData->lightToBeChanged = LIGHT_NONE;
					unlock();
				}
			} // for (TCounter)
		} // if (!bMotion)

		// richiesta pausa in uscita?
		if (bPauseAtEnd)
		{
			lock();
			// imposta lo stato intermedio
			switch (runStatus)
			{
			case 0:
				pSharedData->PlaSubMode = PRONTO_POMERIGGIO;
				// accendi il sole
				pthread_mutex_lock(&mutex_ser);
				stato.sole = MAXSOLE;
				pthread_mutex_unlock(&mutex_ser);
				break;
			case 2:
				pSharedData->PlaSubMode = PRONTO_NOTTE;
				break;
			case 3:
				pSharedData->PlaSubMode = PRONTO_ALBA;
				break;
			case 5:
				// non piu' usato RV21feb2015
				pSharedData->PlaSubMode = PRONTO_TERMINARE;
				break;
			}
			// segnala stato di pausa
			pSharedData->bPaused = TRUE;
			unlock();

			// attendi continue
			while (!pSharedData->bContinue && !pSharedData->bAbort)
			{
				sleep_ms(50);

				// aggiorna dati in real-time in shared memory
				updateData();

				// vedi se richiesta modifica ad una luce durante la pausa
				if (pSharedData->lightToBeChanged != LIGHT_NONE)
				{
					setLight(pSharedData->lightToBeChanged, pSharedData->lightLevel);
					lock();
					pSharedData->lightToBeChanged = LIGHT_NONE;
					unlock();
				}

			}

			// se abort dobbiamo uscire
			if (check_handle_abort())
			{
				lock();
				pSharedData->idError = ERR_ABORTED;
				unlock();
				return;
			}

			// cancella i flag di pausa e continuazione
			lock();
			pSharedData->bPaused = FALSE;
			pSharedData->bContinue = FALSE;
			unlock();
		}
	}
}


// varia intensita' di una luce se richiesto
void setLight(LIGHT_ID lightToBeChanged, int lightLevel)
{
	if (lightToBeChanged == LIGHT_NONE)
		return;

	pthread_mutex_lock(&mutex_ser);

	switch (lightToBeChanged)
	{
		case LIGHT_SOLE: stato.sole = (WORD)lightLevel; break;
		case LIGHT_LUNA: stato.luna = (WORD)lightLevel; break;
		case LIGHT_STELLE: stato.stelle = (WORD)lightLevel; break;
		case LIGHT_CERCHIOORARIO: stato.cerchio_orario = (WORD)lightLevel; break;
		case LIGHT_SUPERNOVA: stato.supernova = (WORD)lightLevel; break;
		case LIGHT_PNEBULA: stato.p_nebula = (WORD)lightLevel; break;
		case LIGHT_NUOVES: stato.nuove_s = (WORD)lightLevel; break;
		case LIGHT_ALIBERO: stato.a_libero = (WORD)lightLevel; break;
		case LIGHT_ALBA: stato.alba = (BYTE)lightLevel; break;
		case LIGHT_GIORNO: stato.giorno = (BYTE)lightLevel; break;
		case LIGHT_TRAMONTO: stato.tramonto = (BYTE)lightLevel; break;
		case LIGHT_PIANETI: stato.pianeti = (lightLevel) > 0 ? ON : OFF; break;
		case LIGHT_DECLINAZIONE: stato.declinazione = (lightLevel) > 0 ? ON : OFF; break;
		case LIGHT_PCARDINALI: stato.p_cardinali = (lightLevel) > 0 ? ON : OFF; break;
		case LIGHT_LUCESALA: stato.luce_sala = (lightLevel) > 0 ? ON : OFF; break;
		case LIGHT_CERCHIOPOLARE: stato.cerchio_polare = (lightLevel) > 0 ? ON : OFF; break;
		case LIGHT_DLIBERO1: stato.d_libero1 = (lightLevel) > 0 ? ON : OFF; break;
		case LIGHT_DLIBERO2: stato.d_libero2 = (lightLevel) > 0 ? ON : OFF; break;
		case LIGHT_DLIBERO3: stato.d_libero3 = (lightLevel) > 0 ? ON : OFF; break;
	}

	pthread_mutex_unlock(&mutex_ser);
}


double d2hms(double dd)
{
	int		hours, mins, secs;
	double	decimals;

	hours = (int)dd;
	decimals = (dd - hours) * 60;
	mins = (int)(decimals + EPS);
	secs = (int)((decimals - mins) * 60 + EPS);
	return hours + mins/100.0 + secs/10000.0;
}


double hms2d(double hms)
{
	int 	hours, mins, secs;
	double	decimals;

	hours = (int)hms;
	decimals = (hms - hours) * 100;
	mins = (int)(decimals + EPS);
	secs = (int)((decimals - mins) * 100 + EPS);
	return hours + mins/60.0 + secs/3600.0;
}


void CalcTime(double ar, double arS, volatile int *thr, volatile int *tmn)
{
	double tcalc;
	tcalc = ar - arS + 12;
	if (tcalc >= 24)
		tcalc -= 24;
	if (tcalc<0)
		tcalc += 24;
	*thr = (int)tcalc;
	*tmn = (int)((tcalc - *thr) * 60);
}


// arresto motori (senza controlli sul ritorno delle funzioni, usato in caso di precedenti errori)
void stop()
{
	long ar, latd, prec;
	
	// leggi posizione corrente
	pthread_mutex_lock(&mutex_ser);
	ar = motc.ascRetta;
	latd = motc.declinazione;
	prec = motc.precessione;
	pthread_mutex_unlock(&mutex_ser);

	// congela la posizione corrente con un "goto"
	SerCmdGotoEncSync(CNT_AR, ar, 10, 5);
	SerCmdGotoEncSync(CNT_LATD, latd, 2, 5);
	SerCmdGotoEncSync(CNT_PREC, prec, 10, 5);
}


// sleep per un tempo in ms<1000
void sleep_ms(int ms)
{
	struct timespec t;
	t.tv_sec = 0;
	t.tv_nsec = ms*1000000L;
	nanosleep(&t, NULL);
}


// controlla e gestisci condizione abort nel modo run
// torna 1 se abort
int check_handle_abort()
{
	if (!pSharedData->bAbort)
		// no abort
		return 0;

	// azzera i flag di sospensione e abort ed eventuale flag
	//   di pausa, poi esci
	lock();
	pSharedData->bAbort = FALSE;
	pSharedData->bSuspend = FALSE;
	pSharedData->bContinue = FALSE;
	pSharedData->bPaused = FALSE;
	unlock();

	// ferma i motori se erano in movimento
	stop();

	// torna flag abort
	return 1;
}


// aggiorna i campi luci e motori della struttura condivisa
void updateData()
{
	lock();
	// aggiorna posizione e ora in memoria condivisa
	pthread_mutex_lock(&mutex_ser);
	pSharedData->ar_now = AR_OFFSET - (double)motc.ascRetta / MAXARCNT * 24.;
	//if (pSharedData->ar_now >= 24)	// no! ar_now e' usata nel loop di controllo motori!
	//	pSharedData->ar_now -= 24;		// togliamo 24 solo nel calcolo di ar_nowpr
	pSharedData->ar_nowpr = d2hms(pSharedData->ar_now >= 24 ? pSharedData->ar_now-24 : pSharedData->ar_now);
	pSharedData->latitude_now = LATD_OFFSET - (double)motc.declinazione / MAXLATDCNT * 360.;
	pSharedData->prec_now = PREC_OFFSET + (double)motc.precessione / MAXPRECCNT * 360.;
	pthread_mutex_unlock(&mutex_ser);
	pSharedData->prec_now_year = pSharedData->prec_now*72.22 + 2000;	//riferito all'anno 2000
	CalcTime(pSharedData->ar_now, pSharedData->sun_ar, &pSharedData->time_hour, &pSharedData->time_min);

	// aggiorna luci e stato motori
	pSharedData->luci = stato;
	pSharedData->motori = motc;
	pthread_mutex_unlock(&mutex_ser);
	unlock();
}


// modo test, supporta comando di luci e singoli motori
int test()
{
	// rimani in modo test fino alla ricezione di abort
	while (!pSharedData->bAbort)
	{
		sleep_ms(50);

		if (pSharedData->bAllLightsOff)
		{
			pthread_mutex_lock(&mutex_ser);
			memset((void *)&stato, 0, sizeof(stato));
			pthread_mutex_unlock(&mutex_ser);
			lock();
			pSharedData->bAllLightsOff = FALSE;
			unlock();
		}
		else if (pSharedData->bAllLightsOn)
		{
			pthread_mutex_lock(&mutex_ser);
			stato.sole = MAXSOLE;
			stato.luna = 4095;
			stato.stelle = pSharedData->maxstars;
			stato.cerchio_orario = 4095;
			stato.supernova = 4095;
			stato.p_nebula = 4095;
			stato.nuove_s = 4095;
			stato.a_libero = 4095;
			stato.alba = 255;
			stato.giorno = 255;
			stato.tramonto = 255;
			stato.pianeti = ON;
			stato.declinazione = ON;
			stato.p_cardinali = ON;
			stato.luce_sala = ON;
			stato.cerchio_polare = ON;
			stato.d_libero1 = ON;
			stato.d_libero2 = ON;
			stato.d_libero3 = ON;
			pthread_mutex_unlock(&mutex_ser);
			lock();
			pSharedData->bAllLightsOn = FALSE;
			unlock();
		}
		else if (pSharedData->lightToBeChanged != LIGHT_NONE)
		{
			setLight(pSharedData->lightToBeChanged, pSharedData->lightLevel);
			pSharedData->lightToBeChanged = LIGHT_NONE;
		}
		else if (pSharedData->bTestZero)
		{
			if (pSharedData->motorId < 0 || pSharedData->motorId > 2)
			{
				lock();
				pSharedData->idError = ERR_TEST_BADMOTORID;
				unlock();
				// il flag abort viene cancellato all'uscita, nel main loop
				return -1;
			}
			pthread_mutex_lock(&mutex_ser);
			switch (pSharedData->motorId)
			{
			case CNT_LATD:
				motc.bSearchZeroDecl = TRUE;
				break;
			case CNT_AR:
				motc.bSearchZeroAscR = TRUE;
				break;
			case CNT_PREC:
				motc.bSearchZeroPrec = TRUE;
				break;
			}
			pthread_mutex_unlock(&mutex_ser);
			if (!SerCmdZeroEncSync(pSharedData->motorId, pSharedData->motorSpeed, 15))
			{
				lock();
				switch (pSharedData->motorId)
				{
				case CNT_LATD:
					pSharedData->bNAKlat = TRUE;
					break;
				case CNT_AR:
					pSharedData->bNAKascr = TRUE;
					break;
				case CNT_PREC:
					pSharedData->bNAKprec = TRUE;
					break;
				}
				pSharedData->idError = ERR_TEST_ZEROENC;
				unlock();
			}
			lock();
			pSharedData->bTestZero = FALSE;
			unlock();
		}
		else if (pSharedData->bTestMove)
		{
			if (pSharedData->motorId < 0 || pSharedData->motorId > 2)
			{
				lock();
				pSharedData->idError = ERR_TEST_BADMOTORID;
				unlock();
				// il flag abort viene cancellato all'uscita, nel main loop
				return -1;
			}
			if (!SerCmdGotoEncSync(pSharedData->motorId, pSharedData->motorTarget, pSharedData->motorSpeed, 15))
			{
				lock();
				switch (pSharedData->motorId)
				{
				case CNT_LATD:
					pSharedData->bNAKlat = TRUE;
					break;
				case CNT_AR:
					pSharedData->bNAKascr = TRUE;
					break;
				case CNT_PREC:
					pSharedData->bNAKprec = TRUE;
					break;
				}
				pSharedData->idError = ERR_TEST_GOTOENC;
				unlock();
			}
			lock();
			pSharedData->bTestMove = FALSE;
			unlock();
		}

		updateData();
	}

	check_handle_abort();
	return 0;
}

int manageParams(int command)
{
	int rc = 0, rcs, i;
	FILE *f;

	switch (command)
	{
	case PARAMS_DEFAULT:
		lock();

		// carica setup di default, puo' essere cambiato dal terminale di controllo
		pSharedData->horlight = 50;				// luce di fondo del cielo di notte 5-250
		pSharedData->maxstars = 400;			// massima luminosita' stelle
		pSharedData->speed = 100;				// velocita' simulazione
		pSharedData->annoPrecessione = 2015;	// anno di riferimento per la precessione
		pSharedData->latitude = hms2d(45.00);	// latitudine osservatore
		pSharedData->sun_ar = hms2d(14.23);		// ascensione retta sole
		pSharedData->moon_ar = hms2d(3.22);		// ascensione retta luna
		pSharedData->mercury_ar = hms2d(2.38);	// ascensione retta mercurio
		pSharedData->venus_ar = hms2d(4.35);	// ascensione retta venere
		pSharedData->mars_ar = hms2d(12.09);	// ascensione retta marte
		pSharedData->jupiter_ar = hms2d(11.21);	// ascensione retta giove
		pSharedData->saturn_ar = hms2d(16.16);	// ascensione retta saturno
		unlock();
		break;

	case PARAMS_LOAD:
		f = fopen("PlaPI.txt", "rt");
		if (!f)
		{
			// mettiamo sul terminale Linux, perchè probabilmente il web sarà ancora spento
			printf("PARAMS_LOAD: PlaPI.txt open error\n");
			fflush(stdout);	// altrim. non scrive su file con la redirezione
			return -1;
		}

		// leggi una riga alla volta, assumendo che l'ordine sia giusto
		for (i = 0; i < NPARAMS; i++)
		{
			char szParam[256];
			double param;

			// leggi prima una riga intera (per ignorare i campi a dx del numero)
			//   e poi un double all'inizio
			if (!fgets(szParam, sizeof(szParam), f) || sscanf(szParam,"%lf", &param) != 1)
			{
				printf("PARAMS_LOAD: PlaPI.txt error readine line %d\n", i+1);
				fflush(stdout);	// altrim. non scrive su file con la redirezione
				fclose(f);
				return -1;
			}

			// copia nella struttura
			lock();
			switch (i)
			{
			case 0:  pSharedData->horlight = (int)param; break;
			case 1:  pSharedData->maxstars = (int)param; break;
			case 2:  pSharedData->speed = (int)param; break;
			case 3:  pSharedData->annoPrecessione = param; break;
			case 4:  pSharedData->latitude = hms2d(param); break;
			case 5:  pSharedData->sun_ar = hms2d(param); break;
			case 6:  pSharedData->moon_ar = hms2d(param); break;
			case 7:  pSharedData->mercury_ar = hms2d(param); break;
			case 8:  pSharedData->venus_ar = hms2d(param); break;
			case 9:  pSharedData->mars_ar = hms2d(param); break;
			case 10: pSharedData->jupiter_ar = hms2d(param); break;
			case 11: pSharedData->saturn_ar = hms2d(param); break;
			}
			unlock();
		}
		fclose(f);
		break;

	case PARAMS_SAVE:
#if ROOT_READONLY
		// rimonta filesystem read/write
		rcs = system("mount -o remount,rw /");
#endif
		f = fopen("PlaPI.txt", "wt");
		if (!f)
		{
			// mettiamo anche sul terminale Linux
			lock();
			pSharedData->idError = ERR_PARAMS_WRITE;
			unlock();
			printf("PARAMS_SAVE: PlaPI.txt open error\n");
			fflush(stdout);	// altrim. non scrive su file con la redirezione
			rc = -1;
		}

		// scrivi una riga alla volta, assumendo che l'ordine sia giusto
		for (i = 0; i < NPARAMS && rc==0; i++)
		{
			char szParam[256];

			// copia nella struttura
			lock();
			switch (i)
			{
			case 0:  sprintf(szParam, "%d // luce di fondo del cielo di notte 5-250", pSharedData->horlight); break;
			case 1:  sprintf(szParam, "%d // massima luminosita' stelle", pSharedData->maxstars); break;
			case 2:  sprintf(szParam, "%d // velocita' simulazione", pSharedData->speed); break;
			case 3:  sprintf(szParam, "%.2f // anno di riferimento per la precessione", pSharedData->annoPrecessione); break;
			case 4:  sprintf(szParam, "%.4f // latitudine osservatore hh.mmss", d2hms(pSharedData->latitude)); break;
			case 5:  sprintf(szParam, "%.4f // ascensione retta sole hh.mmss", d2hms(pSharedData->sun_ar)); break;
			case 6:  sprintf(szParam, "%.4f // ascensione retta luna hh.mmss", d2hms(pSharedData->moon_ar)); break;
			case 7:  sprintf(szParam, "%.4f // ascensione retta mercurio hh.mmss", d2hms(pSharedData->mercury_ar)); break;
			case 8:  sprintf(szParam, "%.4f // ascensione retta venere hh.mmss", d2hms(pSharedData->venus_ar)); break;
			case 9:  sprintf(szParam, "%.4f // ascensione retta marte hh.mmss", d2hms(pSharedData->mars_ar)); break;
			case 10: sprintf(szParam, "%.4f // ascensione retta giove hh.mmss", d2hms(pSharedData->jupiter_ar)); break;
			case 11: sprintf(szParam, "%.4f // ascensione retta saturno hh.mmss", d2hms(pSharedData->saturn_ar)); break;
			}
			unlock();

			// scrivi sul file
			if (fprintf(f, "%s\n", szParam) <= 0)
			{
				lock();
				pSharedData->idError = ERR_PARAMS_WRITE;
				unlock();

				printf("PARAMS_SAVE: PlaPI.txt error writing line %d\n", i + 1);
				fflush(stdout);	// altrim. non scrive su file con la redirezione
				fclose(f);
				rc = -1;
			}

		}

		// se errore il file è già chiuso o non è stato aperto
		if (!rc)
			fclose(f);
		break;
	}

#if ROOT_READONLY
	// rimonta filesystem read-only
	rcs = system("mount -o remount,ro /");
#endif
	return rc;
}
