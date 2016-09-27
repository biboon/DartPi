#ifndef __LIBTHRD_H__
#define __LIBTHRD_H__

extern int livingThreads;

/* Threads */
int startThread(void (*func)(void *), void *arg, int val);
int getLivingThreads();

/* Mutexes */
int P(int semid, int index);
int P_try(int semid, int index);
int V(int semid, int index);
int initMutexes(int nb, unsigned short val);
void sem_free(int semid);

#endif
