/* Ce fichier contient des fonctions de thread */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

#include "libthrd.h"

//#define MAX_THREADS 12

/* Global variables */
int livingThreads = 0;

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};

typedef struct {
	void (*function)(void*);
	void *argument;
} TaskInfo;


/* ---------- */
/* Semaphores */
/* ---------- */
/* Requests a semaphore with R/W of nb elements */
static int sem_alloc(int nb) {
	int semid = semget(IPC_PRIVATE, nb, 0600 | IPC_CREAT);
	if (semid < 0) { perror("libthrd.sem_alloc.semget"); exit(EXIT_FAILURE); }
	#ifdef DEBUG
		printf("Created semaphore #%d of %d mutexes\n", semid, nb);
	#endif
	return semid;
}

/* Frees the semaphore */
int sem_free(int semid) {
	#ifdef DEBUG
		printf("Freeing the semaphore\n");
	#endif
	int status = semctl(semid, 0, IPC_RMID);
	if (status < 0) perror("libthrd.sem_free.semctl");
	return status;
}

/* Initiates the semaphore at value val */
static int sem_init(int semid, int nb, unsigned short val) {
	int status;
	union semun argument;
	unsigned short values[nb];

	/* Initializing semaphore values to val */
	memset(values, val, nb * sizeof(unsigned short));
	argument.array = values;
	status = semctl (semid, 0, SETALL, argument);
	if (status < 0) { perror("libthrd.sem_init.semctl"); exit(EXIT_FAILURE); }
	return status;
}

/* Inits a semaphore of nb elements at the value val */
int initMutexes(int nb, unsigned short val) {
	int semid = sem_alloc(nb);
	sem_init(semid, nb, val);
	return semid;
}

/* Sets the index-th mutex of semaphore semid at the value val */
int set_mutex(int semid, int index, unsigned short val) {
	int status;
	union semun argument;

	argument.val = val;
	status = semctl(semid, index, SETVAL, argument);
	if (status < 0) { perror("libthrd.set_mutex.semctl"); exit(EXIT_FAILURE); }
	return status;
}

/* Main function to request/free the resource */
static int PV(int semid, unsigned short index, short act, short flg) {
	struct sembuf op;
	op.sem_num = index;
	op.sem_op = act; /* P = -1; V = 1 */
	op.sem_flg = flg;
	return semop(semid, &op, 1);
}

/* Request resource to the semaphore and set the calling thread to sleep if
   it is not yet available. Thread resumes when resource is given. */
int P(int semid, int index) {
	int status = PV(semid, index, -1, 0);
	if (status < 0) perror("libthrd.P");
	return status;
}

/* Tries a request to the semaphore and returns immediately. 0 if the lock
   succeeded, 1 if the mutex could not be locked because it already was, or -1. */
int P_try(int semid, int index) {
	int status = PV(semid, index, -1, IPC_NOWAIT);
	if (status < 0) {
		if (errno == EAGAIN) return 1;
		else perror("libthrd.P_try");
	}
	return status;
}

/* Free resource */
int V(int semid, int index) {
	int status = PV(semid, index, 1, 0);
	if (status < 0) perror("libthrd.V");
	return status;
}


/* ----------- */
/*   Threads   */
/* ----------- */
static void* startTask(void *arg) {
	/* Make a local save of the argument */
	TaskInfo* task = (TaskInfo*)arg;
	/* Call the function */
	task->function(task->argument);
	/* Task is over, free memory */
	free(task->argument);
	free(task);
	livingThreads--;
	#ifdef DEBUG
		fprintf(stderr, "Thread terminated, %d remaining\n", livingThreads);
	#endif
	pthread_exit(NULL);
}

/* Returns 0 on success, negative integer if failed */
int startThread(void (*func)(void *), void *arg, int size) {
	pthread_attr_t attr;
	pthread_t tid;
	TaskInfo* task;

	#ifdef MAX_THREADS
		if (livingThreads >= MAX_THREADS) {
			fprintf(stderr, "startThread.thread_limit: error");
			return -5;
		}
	#endif

	/* Save the task info for the thread */
	task = (TaskInfo*) malloc(sizeof(TaskInfo));
	if (task == NULL) { perror("startThread.task.malloc"); return -1; }
	task->function = func;
	if ((task->argument = malloc(size)) == NULL) {
		perror("startThread.task.argument.malloc"); return -2;
	}
	memcpy(task->argument, arg, (size_t)size);

	/* Start the thread */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&tid, &attr, startTask, task) != 0) {
		perror("startThread.pthread_create"); return -3;
	}

	livingThreads++;
	#ifdef DEBUG
		printf("Thread started, %d running\n", livingThreads);
	#endif
	return 0;
}
