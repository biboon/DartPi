#ifndef __LIBTHRD_H__
#define __LIBTHRD_H__

extern int livingThreads;

/* Threads */
int startThread(void (*func)(void *), void *arg, int val);
int getLivingThreads();

/* Mutexes */
int sem_free(int semid);
int initMutexes(int nb, unsigned short val);
int set_mutex(int semid, int index, unsigned short val);
int P(int semid, int index);
int P_try(int semid, int index);
int V(int semid, int index);

#endif
