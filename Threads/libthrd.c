/* Ce fichier contient des fonctions de thread */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

#include "libthrd.h"


union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};

struct taskinfo {
	void (*function)(void*);
	void *argument;
};


/* ---------- */
/* Semaphores */
/* ---------- */
/* Requests a semaphore with R/W of nb elements */
static inline int sem_alloc(int nb) {
	int semid = semget(IPC_PRIVATE, nb, 0600 | IPC_CREAT);
	if (semid < 0) perror("libthrd.sem_alloc.semget");
	#ifdef DEBUG
	else
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

/* Initializes the semaphore at value val */
static inline int sem_init(int semid, int nb, unsigned short val) {
	int status;
	unsigned i;
	union semun argument;
	unsigned short values[nb];

	/* Initializing semaphore values to val */
	for (i = 0; i < nb; ++i) values[i] = val;
	argument.array = values;
	status = semctl (semid, 0, SETALL, argument);
	if (status < 0) perror("libthrd.sem_init.semctl");
	return status;
}

/* Inits a semaphore of nb elements at the value val */
int initMutexes(int nb, unsigned short val) {
	int semid = sem_alloc(nb);
	sem_init(semid, nb, val);
	return semid;
}

/* Request resource to the semaphore and set the calling thread to sleep if
   it is not yet available. Thread resumes when resource is given. */
int P(int semid, unsigned short index) {
	struct sembuf ops = { .sem_num = index, .sem_op = -1, .sem_flg = 0 };
	if (0 == semop(semid, &ops, 1)) return 0;
	if (errno != EINTR) perror("libthrd.P");
	return -1;
}

#ifdef _GNU_SOURCE
#include <time.h>

/* Same as P but waits at most nanos nanoseconds.
   Returns 0 if the lock was performed or 1 if it did not.
	-1 on error. */
int P_timed(int semid, unsigned short index, long nanos) {
	struct sembuf ops = { .sem_num = index, .sem_op = -1, .sem_flg = 0 };
	struct timespec timeout = { .tv_sec = 0, .tv_nsec = nanos };
	if (0 == semtimedop(semid, &ops, 1, &timeout)) return 0;
	if (errno == EAGAIN) return 1;
	if (errno != EINTR) perror("libthrd.P_timed");
	return -1;
}

#endif

/* Same as P but doesn't block. Returns 0 if the lock was performed or
   1 if it did not. -1 on error. */
int P_nowait(int semid, unsigned short index) {
	struct sembuf ops = { .sem_num = index, .sem_op = -1, .sem_flg = IPC_NOWAIT };
	if (0 == semop(semid, &ops, 1)) return 0;
	if (errno == EAGAIN) return 1;
	perror("libthrd.P_try");
	return -1;
}

/* Free resource */
int V(int semid, unsigned short index) {
	struct sembuf ops = { .sem_num = index, .sem_op = 1, .sem_flg = 0 };
	if (0 == semop(semid, &ops, 1)) return 0;
	perror("libthrd.V");
	return -1;
}


/* ----------- */
/*   Threads   */
/* ----------- */
static void *startTask(void *_task) {
	/* Make a local save of the argument */
	struct taskinfo *task = _task;
	/* Call the function */
	task->function(task->argument);
	/* Task is over, free memory */
	free(task->argument);
	free(task);
	pthread_exit(NULL);
}


/* Returns 0 on success, negative integer if failed */
int startThread(pthread_t *thread, void (*func)(void *), void *arg, size_t size) {
	pthread_t tid;
	struct taskinfo *task;

	/* Save the task info for the thread */
	task = malloc(sizeof(struct taskinfo));
	if (task == NULL) { perror("startThread.task.malloc"); return -1; }
	task->function = func;
	task->argument = malloc(size);
	if (task->argument == NULL) { perror("startThread.task.argument.malloc"); return -2; }
	memcpy(task->argument, arg, size);

	/* Start the thread */
	if (pthread_create(&tid, NULL, startTask, task) != 0) {
		perror("startThread.pthread_create"); return -3;
	}
	if (thread != NULL) *thread = tid;

	return 0;
}

int killThread(pthread_t thread, int _signal) {
	#ifdef DEBUG
		printf("Killing thread with signal %d\n", _signal);
	#endif
	int status = pthread_kill(thread, _signal);
	if (status != 0) perror("libthrd.killThread");
	return status;
}

int waitThread(pthread_t thread) {
	#ifdef DEBUG
		printf("Waiting thread\n");
	#endif
	int status = pthread_join(thread, NULL);
	if (status != 0) perror("libthrd.waitThread");
	return status;
}
