#ifndef __LIBTHRD_H__
#define __LIBTHRD_H__

extern int livingThreads;

typedef struct parameters {
	void (*fonction)(void *);
	void *argument;
} Parameters;

/* Threads */
void* lanceFunction(void *arg);
int lanceThread(void (*threadTCP)(void *), void *arg, int val);
int getLivingThreads();

/* Mutexes */
void P(int semid, int index);
void V(int semid, int index);
int initMutexes(int nb, unsigned short val);
void sem_free(int semid);

#endif
