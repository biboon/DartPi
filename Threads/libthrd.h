#ifndef __LIBTHRD_H__
#define __LIBTHRD_H__

/* Threads */
int startThread(pthread_t *, void (*)(void *), void *, size_t);
int waitThread(pthread_t);
int killThread(pthread_t, int);

/* Mutexes */
int sem_free(int);
int initMutexes(int, unsigned short);
int P(int, unsigned short);
int P_nowait(int, unsigned short);
int V(int, unsigned short);

#ifdef _GNU_SOURCE
int P_timed(int, unsigned short, long);
#endif

#endif
