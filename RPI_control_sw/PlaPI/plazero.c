// plazero.c

#include <pthread.h>
#include "SerDefs.h"
#include "PlaDefs.h"
#include "Shared.h"

void slowstop();

// arresto macchina a bassa velocita' se errori o interruzione azzeramento
void slowstop()
{
	//SerCmdGotoEncSync(CNT_AR, -10L, 10, 5);
	//SerCmdGotoEncSync(CNT_LATD, -5L, 2, 5);
	//SerCmdGotoEncSync(CNT_PREC, -10L, 10, 5);

	// @@@@@@ verificare se ha senso con macchina non azzerata
	// se i feedback tornati hanno senso (anche se relativi all'accensione) dovrebbe fermarsi
	SerCmdGotoEncSync(CNT_LATD, motc.declinazione, 2, 5);	// STOP!!
	SerCmdGotoEncSync(CNT_AR, motc.ascRetta, 10, 5);
	SerCmdGotoEncSync(CNT_PREC, motc.precessione, 10, 5);
}

int plazero(void)
{
	// imposta stato iniziale
	lock();
	pSharedData->PlaMode = MODE_ZERO;
	pSharedData->PlaSubMode = AZZERAMENTO;
	pSharedData->bZeroDone = FALSE;
	unlock();
	pthread_mutex_lock(&mutex_ser);
	motc.bSearchZeroDecl = motc.bSearchZeroAscR = motc.bSearchZeroPrec = TRUE;
	pthread_mutex_unlock(&mutex_ser);

	if (!SerCmdZeroEncSync(CNT_LATD, 5, 15))
	{
		lock();
		pSharedData->bNAKlat = TRUE;
		pSharedData->idError = ERR_PLAZERO_ZEROENC;
		unlock();
	}
	if (!SerCmdZeroEncSync(CNT_AR, 150, 15))
	{
		lock();
		pSharedData->bNAKascr = TRUE;
		pSharedData->idError = ERR_PLAZERO_ZEROENC;
		unlock();
	}
	if (!SerCmdZeroEncSync(CNT_PREC, 150, 15))
	{
		lock();
		pSharedData->bNAKprec = TRUE;
		pSharedData->idError = ERR_PLAZERO_ZEROENC;
		unlock();
	}

	// non serve lock per leggere idError, lo scriviamo solo noi
	if (pSharedData->idError)
	{
		slowstop();
		return -1;
	}

	// loop in attesa di raggiungere lo zero
	while (motc.bSearchZeroDecl || motc.bSearchZeroAscR || motc.bSearchZeroPrec)
	{
		// cedi un po' di CPU
		sleep_ms(50);

		// aggiorna dati real-time
		updateData();

		// vedi se richiesti abort o pausa (pausa non ha senso in questo caso)
		// non serve lock, sono di sola lettura
		if (pSharedData->bAbort)
		{
			lock();
			pSharedData->bAbort = FALSE;	// azzera la richiesta
			pSharedData->bSuspend = FALSE;	// non ci interessa, ma torniamo azzerata
			pSharedData->idError = ERR_ABORTED;
			unlock();
			slowstop();
			return -1;
		}
	}

	// fase 2 - ripeti a velocita' piu' bassa per affinare l'azzeramento
	pthread_mutex_lock(&mutex_ser);
	motc.bSearchZeroDecl = motc.bSearchZeroAscR = motc.bSearchZeroPrec = TRUE;
	pthread_mutex_unlock(&mutex_ser);
	if (!SerCmdZeroEncSync(CNT_LATD, 1, 15))
	{
		lock();
		pSharedData->bNAKlat = TRUE;
		pSharedData->idError = ERR_PLAZERO_ZEROENC2;
		unlock();
	}
	if (!SerCmdZeroEncSync(CNT_AR, 50, 15))
	{
		lock();
		pSharedData->bNAKascr = TRUE;
		pSharedData->idError = ERR_PLAZERO_ZEROENC2;
		unlock();
	}
	if (!SerCmdZeroEncSync(CNT_PREC, 50, 15))
	{
		lock();
		pSharedData->bNAKprec = TRUE;
		pSharedData->idError = ERR_PLAZERO_ZEROENC2;
		unlock();
	}

	// non serve lock per leggere idError, lo scriviamo solo noi
	if (pSharedData->idError)
	{
		slowstop();
		return -1;
	}

	while (motc.bSearchZeroDecl || motc.bSearchZeroAscR || motc.bSearchZeroPrec)
	{
		// cedi un po' di CPU
		sleep_ms(50);

		// vedi se richiesti abort o pausa (pausa non ha senso in questo caso)
		// non serve lock, sono di sola lettura
		if (pSharedData->bAbort)
		{

			lock();
			pSharedData->bAbort = FALSE;	// azzera la richiesta
			pSharedData->bSuspend = FALSE;	// non ci interessa, ma torniamo azzerata
			pSharedData->idError = ERR_ABORTED;
			unlock();
			slowstop();
			return -1;
		}

		// aggiorna dati in real-time in shared memory
		updateData();
	}

	// aggiorna posizione e ora in memoria condivisa
	lock();
	pSharedData->ar_now = AR_OFFSET;
	pSharedData->latitude_now = LATD_OFFSET;
	pSharedData->prec_now = PREC_OFFSET;
	pSharedData->prec_now_year = pSharedData->prec_now*72.22 + 2000;	//riferito all'anno 2000
	CalcTime(pSharedData->ar_now, pSharedData->sun_ar, &pSharedData->time_hour, &pSharedData->time_min);
	unlock();

	// fatto!
	pthread_mutex_lock(&mutex_ser);
	pSharedData->bZeroDone = TRUE;
	// azzera anche offset per giri multipli
	motc.ascRettaOffset = 0;
	pthread_mutex_unlock(&mutex_ser);
	return 0;
}
