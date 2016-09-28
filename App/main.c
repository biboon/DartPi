#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#include <libthrd.h>


typedef struct {
	pthread_t tid;
	int id;
	unsigned long shot, hit, step, *remaining;
	int *semid;
} WorkerInfo;



void loop(void* params) {
	WorkerInfo *worker = (WorkerInfo *) params;
	unsigned long todo;
	struct drand48_data buffer;
	double x, y;
	srand48_r(time(NULL) * (1 + worker->id), &buffer);

	P(*(worker->semid), 0);
	while (*(worker->remaining)) {
		todo = (*(worker->remaining) >= worker->step) ? worker->step : *(worker->remaining);
		*(worker->remaining) -= todo;
		V(*(worker->semid), 0);
		worker->shot += todo;
		for ( ; todo > 0; --todo) {
			drand48_r(&buffer, &x);
			drand48_r(&buffer, &y);
			if ((x*x + y*y) < 1) (worker->hit)++;
		}
		P(*(worker->semid), 0);
	}
	V(*(worker->semid), 0);

	#ifdef DEBUG
		printf("%d: %ld shots for %ld hits\n", worker->id, worker->shot, worker->hit);
	#endif
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
	WorkerInfo workers[threads];
	memset(workers, 0, threads * sizeof(WorkerInfo));
	semid = initMutexes(1, 1);
	printf("Starting with %ld shots and %ld threads\n", remaining, threads);

	start = clock();
	gettimeofday(&tstart, NULL);

	/* Start all the workers */
	for (i = 1; i < threads; i++) {
		workers[i].id = i;
		workers[i].remaining = &remaining;
		workers[i].step = step;
		workers[i].semid = &semid;
		startThread(&workers[i].tid, loop, workers + i, sizeof(WorkerInfo));
	}

	/* Start the work for the main thread */
	//workers[0].id = 0; // Should not be necessary since we used memset
	workers[0].remaining = &remaining;
	workers[0].step = step;
	workers[0].semid = &semid;
	loop(workers);

	/* Wait until each thread returns */
	for (i = 1; i < threads; i++) waitThread(workers[i].tid);

	/* See results */
	for (i = 0; i < threads; i++) {
		tothits += workers[i].hit;
		totshots += workers[i].shot;
	}
	long double pi = (long double)(tothits * 4)/(long double)totshots;
	end = clock();
	gettimeofday(&tend, NULL);

	/* Display results */
	printf("Pi is %.12Lf\n\n", pi);
	printf("CPU time: %f s\n", (double)(end - start)/CLOCKS_PER_SEC);
	printf("Real time: %f s\n", (double)(tend.tv_usec - tstart.tv_usec)/1e6 + (double)(tend.tv_sec - tstart.tv_sec));

	return 0;
}
