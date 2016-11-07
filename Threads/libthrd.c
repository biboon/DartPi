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

typedef struct {
	void (*function)(void*);
	void *argument;
} TaskInfo_t;


/* ---------- */
/* Semaphores */
/* ---------- */
/* Requests a semaphore with R/W of nb elements */
static int sem_alloc(int nb) {
	int semid = semget(IPC_PRIVATE, nb, 0600 | IPC_CREAT);
	if (semid < 0) perror("libthrd.sem_alloc.semget");
	#ifdef VERBOSE
		else
			printf("Created semaphore #%d of %d mutexes\n", semid, nb);
	#endif
	return semid;
}

/* Frees the semaphore */
int sem_free(int semid) {
	#ifdef VERBOSE
		printf("Freeing the semaphore\n");
	#endif
	int status = semctl(semid, 0, IPC_RMID);
	if (status < 0) perror("libthrd.sem_free.semctl");
	return status;
}

/* Initializes the semaphore at value val */
static int sem_init(int semid, int nb, unsigned short val) {
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
	int status = semop(semid, &ops, 1);
	if (status < 0 && errno != EINTR) perror("libthrd.P");
	return status;
}

#ifdef _GNU_SOURCE
#include <time.h>

/* Same as P but waits at most nanos nanoseconds.
   Returns 0 if the lock was performed or -1 if it did not. */
int P_timed(int semid, unsigned short index, long nanos) {
	struct sembuf ops = { .sem_num = index, .sem_op = -1, .sem_flg = 0 };
	struct timespec timeout = { .tv_sec = 0, .tv_nsec = nanos };
	int status = semtimedop(semid, &ops, 1, &timeout);
	if (status < 0 && errno != EINTR) perror("libthrd.P_timed");
	return status;
}

#endif

/* Same as P but doesn't block. Returns 0 if the lock was performed or
   -1 if it did not. */
int P_nowait(int semid, unsigned short index) {
	struct sembuf ops = { .sem_num = index, .sem_op = -1, .sem_flg = IPC_NOWAIT };
	int status = semop(semid, &ops, 1);
	if (status < 0) {
		if (errno == EAGAIN) return 1;
		else perror("libthrd.P_try");
	}
	return status;
}

/* Free resource */
int V(int semid, unsigned short index) {
	struct sembuf ops = { .sem_num = index, .sem_op = 1, .sem_flg = 0 };
	int status = semop(semid, &ops, 1);
	if (status < 0) perror("libthrd.V");
	return status;
}


/* ----------- */
/*   Threads   */
/* ----------- */
static void *startTask(void *task) {
	/* Make a local save of the argument */
	TaskInfo_t *_task = (TaskInfo_t *)task;
	/* Call the function */
	_task->function(_task->argument);
	/* Task is over, free memory */
	free(_task->argument);
	free(_task);
	pthread_exit(NULL);
}


/* Returns 0 on success, negative integer if failed */
int startThread(pthread_t *thread, void (*func)(void *), void *arg, size_t size) {
	pthread_t tid;
	TaskInfo_t* task;

	#ifdef MAX_THREADS
		if (livingThreads >= MAX_THREADS) {
			fprintf(stderr, "startThread.thread_limit: error");
			return -5;
		}
	#endif

	/* Save the task info for the thread */
	task = (TaskInfo_t*) malloc(sizeof(TaskInfo_t));
	if (task == NULL) { perror("startThread.task.malloc"); return -1; }
	task->function = func;
	task->argument = malloc(size);
	if (task->argument == NULL) { perror("startThread.task.argument.malloc"); return -2; }
	memcpy(task->argument, arg, (size_t)size);

	/* Start the thread */
	if (pthread_create(&tid, NULL, startTask, task) != 0) {
		perror("startThread.pthread_create"); return -3;
	}

	if (thread != NULL) *thread = tid;
	return 0;
}

int killThread(pthread_t thread, int _signal) {
	#ifdef VERBOSE
		printf("Killing thread with signal %d\n", _signal);
	#endif
	int status = pthread_kill(thread, _signal);
	if (status != 0) perror("libthrd.killThread");
	return status;
}

int waitThread(pthread_t thread) {
	#ifdef VERBOSE
		printf("Waiting thread\n");
	#endif
	int status = pthread_join(thread, NULL);
	if (status != 0) perror("libthrd.waitThread");
	return status;
}
