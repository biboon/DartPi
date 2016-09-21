/* Ce fichier contient des fonctions de thread */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

#include "libthrd.h"

#define MAX_THREADS 100

/* Global variables */
int livingThreads = 0;

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};


/* ---------- */
/* Semaphores */
/* ---------- */
/* Requests a semaphore with R/W of nb elements */
static int sem_alloc(int nb) {
	#ifdef DEBUG
		fprintf(stderr, "Creating %d semaphores\n", nb);
	#endif
	int semid = semget(IPC_PRIVATE, nb, 0666 | IPC_CREAT);
	if (semid < 0) { perror("libthrd.sem_alloc.semget"); exit(EXIT_FAILURE); }
	return semid;
}

/* Frees the semaphore */
void sem_free(int semid) {
	#ifdef DEBUG
		fprintf(stderr, "Freeing the semaphore\n");
	#endif
	int status = semctl(semid, 0, IPC_RMID, NULL);
	if (status < 0) perror("libthrd.sem_free.semctl");
}

/* Initiates the semaphore at value val */
static void sem_init(int semid, int nb, unsigned short val) {
	int status = -1;
	union semun argument;
	unsigned short values[nb];

	/* Initializing semaphore values to val */
	memset(values, val, nb * sizeof(unsigned short));
	argument.array = values;

	status = semctl (semid, 0, SETALL, argument);
	if (status < 0) { perror("libthrd.sem_init.semctl"); exit(EXIT_FAILURE); }
}

/* Inits a semaphore of nb elements at the value val */
int initMutexes(int nb, unsigned short val) {
	int semid = sem_alloc(nb);
	sem_init(semid, nb, val);
	return semid;
}

/* Main function to request/free the resource */
static int PV(int semid, int index, int act) {
	struct sembuf op;
	op.sem_num = index;
	op.sem_op = act; /* P = -1; V = 1 */
	op.sem_flg = 0;
	return semop(semid, &op, 1);
}

/* Request resource to the semaphore and set the calling thread to sleep if
   it is not yet available. Thread resumes when resource is given. */
void P(int semid, int index) {
	if (PV(semid, index, -1) < 0) perror ("libthrd.P");
}

/* Free resource */
void V(int semid, int index) {
	if (PV(semid, index, 1) < 0) perror ("libthrd.V");
}


/* ----------- */
/*   Threads   */
/* ----------- */
void* lanceFunction(void *arg) {
	/* Copie de l'argument */
	Parameters *funcParameters = arg;
	/* Appel de la fonction avec l'argument dans la structure */
	funcParameters->fonction(funcParameters->argument);
	/* Liberation de la memoire */
	free(funcParameters->argument);
	free(funcParameters);

	livingThreads--;
	#ifdef DEBUG
		fprintf(stderr, "Thread terminated, %d remaining\n", livingThreads);
	#endif

	pthread_exit(NULL);
}


/* Returns 0 on success, negative integer if failed */
int lanceThread(void (*func)(void *), void *arg, int size) {
	#ifdef MAX_THREADS
		if (livingThreads == MAX_THREADS) {
			//#ifdef DEBUG
				fprintf(stderr, "lanceThread.thread_limit: error");
			//#endif
			return -5;
		}
	#endif

	Parameters* funcParameters = (Parameters*) malloc(sizeof(Parameters));
	if (funcParameters == NULL) {
		perror("lanceThread.funcParameters.malloc"); return -1;
	}

	funcParameters->fonction = func;
	if ((funcParameters->argument = malloc(size)) == NULL) {
		perror("lanceThread.funcParameters.argument.malloc"); return -2;
	}
	memcpy(funcParameters->argument, arg, (size_t)size);

	pthread_attr_t attr;
	pthread_t tid;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&tid, &attr, lanceFunction, funcParameters) != 0) {
		perror("lanceThread.pthread_create"); return -3;
	}

	livingThreads++;
	#ifdef DEBUG
		printf("Thread started, %d running\n", livingThreads);
	#endif
	return 0;
}
