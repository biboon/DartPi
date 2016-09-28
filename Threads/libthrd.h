#ifndef __LIBTHRD_H__
#define __LIBTHRD_H__

extern int livingThreads;

/* Threads */
int startThread(void (*func)(void *), void *arg, size_t size);
int getLivingThreads();

/* Mutexes */
int sem_free(int semid);
int initMutexes(int nb, unsigned short val);
int set_mutex(int semid, int index, unsigned short val);
int get_mutex(int semid, int index);
int PV(int semid, unsigned short* index, short* act, short* flg, size_t nops);
int PV_one(int semid, unsigned short index, short act, short flg);
int P(int semid, unsigned short index);
int P_try(int semid, unsigned short index);
int V(int semid, unsigned short index);

#endif
