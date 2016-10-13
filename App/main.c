#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

#include <libthrd.h>

#define PI 3.141592653589793

typedef struct {
	pthread_t tid;
	int id;
	unsigned long shot, hit, step, *remaining;
	int semid;
} WorkerInfo_t;


int hash(void *ptr, size_t size) {
	int i = size - 1, res = 0;
	for ( ; i != 0; --i) res ^= *(unsigned char*)(ptr + i);
	return res;
}


void loop(void* params) {
	WorkerInfo_t *worker = *(WorkerInfo_t **)params;
	unsigned long todo;
	struct drand48_data buffer;
	double x, y;
	srand48_r(time(NULL) * hash(worker, sizeof(WorkerInfo_t)), &buffer);

	P(worker->semid, 0);
	while (*(worker->remaining)) {
		todo = (*(worker->remaining) >= worker->step) ? worker->step : *(worker->remaining);
		*(worker->remaining) -= todo;
		V(worker->semid, 0);
		worker->shot += todo;
		for ( ; todo > 0; --todo) {
			drand48_r(&buffer, &x);
			drand48_r(&buffer, &y);
			if ((x*x + y*y) <= 1.0) (worker->hit)++;
		}
		P(worker->semid, 0);
	}
	V(worker->semid, 0);
}


/** Main function **/
int main(int argc, char** argv) {
	unsigned long remaining, threads, step, i, tothits = 0, totshots = 0;
	int semid;
	clock_t start, end;
	struct timeval tstart, tend;

	/* Printing some info */
	printf("This sofware will give an approximation of Pi using Dartboard algorithm\n");
	if (argc < 3) {
		printf("Usage: %s shots threads\n", argv[0]);
		return -1;
	}

	/* Initialization */
	remaining = strtol(argv[1], NULL, 10);
	threads = strtol(argv[2], NULL, 10);
	step = remaining / threads / 20;
	WorkerInfo_t workers[threads];
	memset(workers, 0, threads * sizeof(WorkerInfo_t));
	semid = initMutexes(1, 1);
	printf("Starting with %ld shots and %ld threads\n", remaining, threads);

	start = clock();
	gettimeofday(&tstart, NULL);

	/* Start all the workers */
	WorkerInfo_t *_worker;
	for (i = 1; i < threads; i++) {
		_worker = workers + i;
		_worker->id = i;
		_worker->remaining = &remaining;
		_worker->step = step;
		_worker->semid = semid;
		startThread(&(_worker->tid), loop, &_worker, sizeof(WorkerInfo_t *));
	}

	/* Start the work for the main thread */
	_worker = workers;
	_worker->remaining = &remaining;
	_worker->step = step;
	_worker->semid = semid;
	loop(&_worker);

	/* Wait until each thread returns */
	for (i = 1; i < threads; i++) waitThread(workers[i].tid);
	sem_free(semid);

	/* See results */
	for (i = 0; i < threads; i++) {
		tothits += workers[i].hit;
		totshots += workers[i].shot;
	}
	long double pi = (long double)(tothits * 4)/(long double)totshots;
	end = clock();
	gettimeofday(&tend, NULL);

	#ifdef VERBOSE
		for (i = 0; i < threads; i++)
			printf("%d: %ld shots for %ld hits\n", workers[i].id, workers[i].shot, workers[i].hit);
	#endif

	/* Display results */
	printf("Pi is %.15Lf, err is %.15Lf\n\n", pi, pi - PI);
	printf("CPU time: %f s\n", (double)(end - start)/CLOCKS_PER_SEC);
	printf("Real time: %f s\n", (double)(tend.tv_usec - tstart.tv_usec)/1e6 + (double)(tend.tv_sec - tstart.tv_sec));

	return 0;
}
