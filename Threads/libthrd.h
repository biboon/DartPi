#ifndef __LIBTHRD_H__
#define __LIBTHRD_H__

/* Threads */
int thrd_start(pthread_t *, void (*)(void *), void *, size_t);
int thrd_join(pthread_t);
int thrd_kill(pthread_t, int);

/* Mutexes */
int thrd_semaphore_create(int, unsigned short);
int thrd_semaphore_destroy(int);
int thrd_mutex_lock(int, unsigned short);
int thrd_mutex_lock_try(int, unsigned short);
int thrd_mutex_unlock(int, unsigned short);

#ifdef _GNU_SOURCE
int thrd_mutex_lock_timed(int, unsigned short, long);
#endif

#endif
