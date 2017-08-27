// plasicur.c

#include <pthread.h>
#include <stdio.h>
#include <math.h>
#include "SerDefs.h"
#include "PlaDefs.h"
#include "Shared.h"

int plasicur(void)
{
	char c;
	int test_enc1_cnt = 0;

	// imposta offset dell'encoder di ascensione retta al giro piu' vicino allo zero (v. anche run, setPlanets)
	pthread_mutex_lock(&mutex_ser);
	motc.ascRettaOffset = lround((double)motc1.encY / MAXARCNT - AR_OFFSET / 24.0) * MAXARCNT;
	pthread_mutex_unlock(&mutex_ser);

	// lat sicura
	if (!SerCmdGotoEncSync(CNT_LATD, (long)(-MAXLATDCNT*(LATD_SIC-LATD_OFFSET)/360.), 5, 15))
	{
		lock();
		pSharedData->idError = ERR_PLASICUR_GOTOSICUR;
		unlock();
		return -1;
	}
	// ar zero
	if (!SerCmdGotoEncSync(CNT_AR, (long)(MAXARCNT*AR_OFFSET / 24.0), 150, 15))
	{
		lock();
		pSharedData->idError = ERR_PLASICUR_GOTOSICUR;
		unlock();
		return -1;
	}

	do
	{
		// cedi un po' di CPU
		sleep_ms(50);

		if (!test_enc1_cnt)
		{
			test_enc1_cnt = 20;	// stampa ogni 1 s per test
			printf("(SICUR) motc1.encY=%ld, ascRetta=%ld, ascRettaOffset=%ld\n", motc1.encY, motc.ascRetta, motc.ascRettaOffset);
			fflush(stdout);	// altrim. non scrive su file con la redirezione
		}
		else
			test_enc1_cnt--;

		// aggiorna dati in real-time in shared memory
		updateData();
	} while ((fabs(pSharedData->latitude_now - LATD_SIC)>0.5 || fabs(pSharedData->ar_now)>0.5) && !pSharedData->bAbort);

	if (pSharedData->bAbort)
	{
		SerCmdGotoEncSync(CNT_LATD, motc.declinazione, 2, 5);	// STOP!!
		SerCmdGotoEncSync(CNT_AR, motc.ascRetta, 10, 5);

		lock();
		pSharedData->bAbort = FALSE;	// azzera la richiesta
		pSharedData->bSuspend = FALSE;	// non ci interessa, ma torniamo azzerata
		pSharedData->idError = ERR_ABORTED;
		unlock();
	}

	// fatto!
	return 0;
}