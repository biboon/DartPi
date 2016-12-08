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

struct thrd_task_info {
	void (*function)(void*);
	void *argument;
};


/* ---------- */
/* Semaphores */
/* ---------- */
/* Requests a semaphore with R/W of nb elements */
static inline int thrd_semaphore_get(int nb)
{
	int semid = semget(IPC_PRIVATE, nb, 0600 | IPC_CREAT);
	if (semid != -1) return semid;
	perror("thrd_semaphore_get.semget");
	return -1;
}

/* Initializes the semaphore at value val */
static inline int thrd_semaphore_set(int semid, int nb, unsigned short val)
{
	unsigned i;
	unsigned short values[nb];
	/* Initializing semaphore values to val */
	for (i = 0; i < nb; ++i) values[i] = val;
	union semun argument = { .array = values };
	if (0 == semctl (semid, 0, SETALL, argument)) return 0;
	perror("thrd_semaphore_set.semctl");
	return -1;
}

/* Creates a semaphore of nb elements at the value val */
int thrd_semaphore_create(int nb, unsigned short val)
{
	int semid = thrd_semaphore_get(nb);
	if (semid != -1 && -1 != thrd_semaphore_set(semid, nb, val)) {
#ifdef DEBUG
		printf("Got semaphore #%d of %d mutexes\n", semid, nb);
#endif
		return semid;
	}
	return -1;
}

/* Removes the semaphore */
int thrd_semaphore_destroy(int semid)
{
	#ifdef DEBUG
		printf("Destroying semaphore %d\n", semid);
	#endif
	if (0 == semctl(semid, 0, IPC_RMID)) return 0;
	perror("thrd_semaphore_destroy.semctl");
	return -1;
}

/* Request resource to the semaphore and set the calling thread to sleep if
   it is not yet available. Thread resumes when resource is given. */
int thrd_mutex_lock(int semid, unsigned short index)
{
	struct sembuf ops = { .sem_num = index, .sem_op = -1, .sem_flg = 0 };
	if (0 == semop(semid, &ops, 1)) return 0;
	if (errno != EINTR) perror("thrd_mutex_lock");
	return -1;
}

#ifdef _GNU_SOURCE
#include <time.h>
/* Same as P but waits at most nanos nanoseconds.
   Returns 0 if the lock was performed or 1 if it did not.
	-1 on error. */
int thrd_mutex_lock_timed(int semid, unsigned short index, long nanos)
{
	struct sembuf ops = { .sem_num = index, .sem_op = -1, .sem_flg = 0 };
	struct timespec timeout = { .tv_sec = 0, .tv_nsec = nanos };
	if (0 == semtimedop(semid, &ops, 1, &timeout)) return 0;
	if (errno == EAGAIN) return 1;
	if (errno != EINTR) perror("thrd_mutex_lock_timed");
	return -1;
}
#endif

/* Same as P but doesn't block. Returns 0 if the lock was performed or
   1 if it did not. -1 on error. */
int thrd_mutex_lock_try(int semid, unsigned short index)
{
	struct sembuf ops = { .sem_num = index, .sem_op = -1, .sem_flg = IPC_NOWAIT };
	if (0 == semop(semid, &ops, 1)) return 0;
	if (errno == EAGAIN) return 1;
	perror("thrd_mutex_lock_try");
	return -1;
}

/* Free resource */
int thrd_mutex_unlock(int semid, unsigned short index)
{
	struct sembuf ops = { .sem_num = index, .sem_op = 1, .sem_flg = 0 };
	if (0 == semop(semid, &ops, 1)) return 0;
	perror("thrd_mutex_unlock");
	return -1;
}


/* ----------- */
/*   Threads   */
/* ----------- */
static void *thrd_start_task(void *_task)
{
	struct thrd_task_info *task = _task;
	/* Call the function */
	task->function(task->argument);
	/* Task is over, free memory */
	free(task->argument);
	free(task);
	pthread_exit(NULL);
}


/* Returns 0 on success, negative integer if failed */
int thrd_start(pthread_t *thread, void (*func)(void *), void *arg, size_t size)
{
	pthread_t tid;
	struct thrd_task_info *task;
	/* Save the task info for the thread */
	task = malloc(sizeof(struct thrd_task_info));
	if (task == NULL) {
		perror("thrd_start.task.malloc");
		return -1;
	}
	task->function = func;
	task->argument = malloc(size);
	if (task->argument == NULL) {
		perror("thrd_start.task.argument.malloc");
		return -1;
	}
	memcpy(task->argument, arg, size);
	/* Start the thread */
	if (0 == pthread_create(&tid, NULL, thrd_start_task, task)) {
		if (thread != NULL) *thread = tid;
		return 0;
	}
	perror("thrd_start.pthread_create");
	return -1;
}

int thrd_join(pthread_t thread)
{
#ifdef DEBUG
	printf("Waiting thread\n");
#endif
	int status = pthread_join(thread, NULL);
	if (status == 0) return 0;
	fprintf(stderr, "thrd_join: %s\n", strerror(status));
	return -1;
}

int thrd_kill(pthread_t thread, int _signal)
{
#ifdef DEBUG
	printf("Killing thread with signal %d\n", _signal);
#endif
	int status = pthread_kill(thread, _signal);
	if (status == 0) return 0;
	fprintf(stderr, "thrd_kill: %s\n", strerror(status));
	return -1;
}
