// versioni sincrone delle funzioni SerCmdXxx
// RV080400

#include <pthread.h>
#include "SerDefs.h"

// il parametro nAttempts e' il numero di tentativi
//   di ritrasmissione in caso di timeout
BOOL SerCmdZeroEncSync(int nId, WORD wSpeed, int nAttempts)
{
	int nResult;

	while (nAttempts--)
	{
		// aspetta che si liberi la "porta comandi"
		while (!SerCanTx());

		// invia
		if (!SerCmdZeroEnc(nId, wSpeed))
			// uscita per problemi sui parametri
			return FALSE;

		// aspetta che il comando termini
		while (!(nResult = SerGetCmdResult()));

		// se ok, esci
		if (nResult == 1)
			return TRUE;
	}

	return FALSE;
}


BOOL SerCmdGotoEncSync(int nId, long lnTarget, WORD wSpeed, int nAttempts)
{
	int nResult;

	while (nAttempts--)
	{
		// aspetta che si liberi la "porta comandi"
		while (!SerCanTx());

		// invia
		if (!SerCmdGotoEnc(nId, lnTarget, wSpeed))
			// uscita per problemi sui parametri
			return FALSE;

		// aspetta che il comando termini
		while (!(nResult = SerGetCmdResult()));

		// se ok, esci
		if (nResult == 1)
			return TRUE;
	}

	return FALSE;
}


// richiesta di acknowledge controllore zero
// ritorna: come SerTxBuf()
BOOL SerCmdAck0Sync(int nAttempts)
{
	int nResult;

	while (nAttempts--)
	{
		// aspetta che si liberi la "porta comandi"
		while (!SerCanTx());

		// invia
		if (!SerCmdAck0())
			// uscita per problemi sui parametri
			return FALSE;

		// aspetta che il comando termini
		while (!(nResult = SerGetCmdResult()));

		// se ok, esci
		if (nResult == 1)
			return TRUE;
	}

	return FALSE;
}


// richiesta di versione controllore zero
// ritorna: come SerTxBuf()
BOOL SerCmdVer0Sync(int nAttempts)
{
	int nResult;

	while (nAttempts--)
	{
		// aspetta che si liberi la "porta comandi"
		while (!SerCanTx());

		// invia
		if (!SerCmdVer0())
			// uscita per problemi sui parametri
			return FALSE;

		// aspetta che il comando termini
		while (!(nResult = SerGetCmdResult()));

		// se ok, esci
		if (nResult == 1)
			return TRUE;
	}

	return FALSE;
}

