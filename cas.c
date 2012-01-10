#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define BLOCK_START(id,p,n) ((id)*(n)/(p))
#define BLOCK_SIZE(id,p,n) (BLOCK_START((id)+1,p,n)-BLOCK_START(id,p,n))

typedef long long counter_t;

struct data_t {
	pthread_t id;
	counter_t *counter;
	long long num_iterations;
};

void *thread_main(void *arg) {
	struct data_t *data = (struct data_t *)arg;
	long long num_iterations = data->num_iterations;
	counter_t old = 0, prev, new;
	
	while(num_iterations-->0) {
		new = old + 1;
		prev = __sync_val_compare_and_swap(data->counter, old, new);
		if(old == prev) {
			old = new;
		} else {
			old = prev;
		}
	}
	
	return NULL;
}

void run_test(int num_threads, long long num_iterations) {
	int i;
	counter_t counter = 0;
	struct data_t thread_data[num_threads], *data;
	struct timeval start, end;

	gettimeofday(&start, NULL);
	for(i = 0; i < num_threads; i++) {
		data = &thread_data[i];
		data->counter = &counter;
		data->num_iterations = BLOCK_SIZE(i, num_threads, num_iterations);
		pthread_create(&data->id, NULL, thread_main, data);
	}
	for(i = 0; i < num_threads;i++) {
		data = &thread_data[i];
		pthread_join(data->id, NULL);
	}
	gettimeofday(&end, NULL);
	
	time_t elapsed_time_us = end.tv_usec - start.tv_usec;
	elapsed_time_us += 1000000l * (end.tv_sec - start.tv_sec);
	printf("CAS/sec = %d\n", (int)(num_iterations / (elapsed_time_us / 1e6)));
}

int main(int argc, char **argv) {
	char *ptr;
	int num_threads;
	long long num_iterations;
	
	errno = 0;
	if(argc != 3) {
		printf("usage: %s #threads #iterations\n", argv[0]);
		return 1;
	}
	
	num_threads = strtol(argv[1], &ptr, 0);
	if(errno || num_threads <= 0 || *ptr != '\0') {
		printf("#threads must be a positive integer.\n");
		return 1;
	}
	
	num_iterations = strtoll(argv[2], &ptr, 0);
	if(errno || num_iterations <= 0 || *ptr != '\0') {
		printf("#iterations must be a positive integer.\n");
		return 1;
	}
	
	run_test(num_threads, num_iterations);
	
	return 0;
}

