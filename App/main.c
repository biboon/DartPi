#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include <libthrd.h>

unsigned long* hits = NULL;

void loop(void* params) {
	unsigned long* _params = (unsigned long*) params;
	struct drand48_data buffer;
	double x, y;
	srand48_r(time(NULL) * _params[1], &buffer);
	for( ; _params[0] > 0; _params[0]--) {
		drand48_r(&buffer, &x);
		drand48_r(&buffer, &y);
		if ((x*x + y*y) < 1) hits[_params[1]]++;
	}
}


/** Main function **/
int main(int argc, char** argv) {
	unsigned long shots = 0, threads = 0, tothits = 0, params[2], i = 0;
	clock_t start, end;
	if (argc < 3) {
		printf("Usage: %s shots threads\n", argv[0]);
		return -1;
	}

	shots = strtol(argv[1], NULL, 10);
	threads = strtol(argv[2], NULL, 10);

	printf("This sofware will give an approximation of Pi using\n");
	printf("Dartboard algorithm with %ld shots and %ld threads\n\n", shots, threads);
	
	hits = (unsigned long*) calloc(threads, sizeof(unsigned long));
	params[0] = shots/threads;

	start = clock();

	for (i = 1; i < threads; i++) {
		params[1] = i;
		lanceThread(loop, params, 2 * sizeof(unsigned long));
	}

	params[1] = 0;
	loop(params);	

	// Wait until each thread returns
	while (livingThreads > 0);

	for (i = 0; i < threads; i++) tothits += hits[i];
	long double pi = (long double)(tothits << 2)/(long double)shots;
	end = clock();
	printf("Pi is %.12Lf\n", pi);
	printf("Processing time: %f s\n", (double)(end - start)/CLOCKS_PER_SEC);
	free(hits);

	return 0;
}
