#ifndef __LIBTHRD_H__
#define __LIBTHRD_H__

/* Threads */
int startThread(pthread_t *thread, void (*func)(void *), void *arg, size_t size);
int waitThread(pthread_t thread);
int killThread(pthread_t thread, int _signal);

/* Mutexes */
int sem_free(int semid);
int initMutexes(int nb, unsigned short val);
int P(int semid, unsigned short index);
int P_try(int semid, unsigned short index);
int V(int semid, unsigned short index);

#endif
