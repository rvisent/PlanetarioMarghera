#include <stdio.h>
#include <string.h>	// memset, strerror
#include <pthread.h>
#include <sys/ipc.h>	// shmget
#include <sys/shm.h>	// shmget
#include <sys/sem.h>	// sembuf
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "PlaDefs.h"
#include "SerDefs.h"
#include "Shared.h"

int sharedId;	// id of shared memory object
int semId;		// id of semaphore
volatile struct SHARED_DATA *pSharedData;	// pointer to shared memory

BOOL initSharedMemoryAndSemaphores();

int main()
{
	int rc;
	pthread_t thread_serial;

#if !ROOT_READONLY
	// metti il PI in runlevel 4, cosi' si libera la seriale se usata da getty
	system("sudo init 4");
#endif

	if (!initSharedMemoryAndSemaphores())
		return -1;

	// Create thread for serial port management
	if ((rc = pthread_create(&thread_serial, NULL, &SerMain, NULL)))
	{
		fprintf(stderr, "Errore creazione thread: %d\n", rc);
		return -1;
	}

	PlaMain();
}


BOOL initSharedMemoryAndSemaphores()
{
	const char *szLoc = "main::initSharedMemoryAndSemaphores()";
	semun_t arg;

	// create a new shared memory segment
	sharedId = shmget(SHARED_KEY, sizeof(struct SHARED_DATA), 0666 | IPC_CREAT | S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
	if (sharedId == -1)
	{
		fprintf(stderr, "%s: shared memory init error=%s\n", szLoc, strerror(errno));
		return FALSE;
	}

	// attach the shared memory segment
	pSharedData = (struct SHARED_DATA *)shmat(sharedId, 0, 0);
	// safety check
	if ((int)pSharedData == -1)
	{
		fprintf(stderr, "%s: shared memory initialization error=%s\n", szLoc, strerror(errno));
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
		fprintf(stderr, "%s: semaphore allocation error=%s\n", szLoc, strerror(errno));
		return FALSE;
	}

	arg.val = 1;
	if (semctl(semId, 0, SETVAL, arg) == -1)
	{
		fprintf(stderr, "%s: binary_semaphore_initialization: error=%s\n", szLoc, strerror(errno));
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

