#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

struct data_t {
	pthread_t id;
	long long *counter;
	long long target_iterations;
	long long cas_succ;
	long long cas_fail;
};


void *thread_main(void *arg) {
	struct data_t *data = (struct data_t *)arg;
	long long *counter = data->counter, target_iterations = data->target_iterations;
	long long old = 0, prev, new, cas_succ = 0, cas_fail = 0;

	while (old < target_iterations) {
		new = old + 1;
		prev = __sync_val_compare_and_swap(counter, old, new);
		if (prev == old) {
			old = new;
			cas_succ++;
		} else {
			old = prev;
			cas_fail++;
		}
	}

	data->cas_succ = cas_succ;
	data->cas_fail = cas_fail;
	return NULL;
}


void run_test(int num_threads, long long num_iterations) {
	int i;
	long long counter = 0, cas_succ = 0, cas_fail = 0;
	struct data_t thread_data[num_threads], *data;
	struct timeval start, end;

	gettimeofday(&start, NULL);
	for (i = 0; i < num_threads; i++) {
		data = &thread_data[i];
		data->counter = &counter;
		data->target_iterations = num_iterations;
		data->cas_succ = 0;
		data->cas_fail = 0;
		pthread_create(&data->id, NULL, thread_main, data);
	}
	for (i = 0; i < num_threads;i++) {
		data = &thread_data[i];
		pthread_join(data->id, NULL);
		cas_succ += data->cas_succ;
		cas_fail += data->cas_fail;
	}
	gettimeofday(&end, NULL);

	time_t elapsed_time_us = end.tv_usec - start.tv_usec;
	elapsed_time_us += 1000000l * (end.tv_sec - start.tv_sec);
	printf("time = %.3fms\n", elapsed_time_us / 1e3);
	printf("cas_succ = %lld\n", cas_succ);
	printf("cas_fail = %lld\n", cas_fail);
}


int main(int argc, char **argv) {
	char *ptr;
	int num_threads;
	long long num_iterations;

	errno = 0;
	if (argc != 3) {
		printf("usage: %s #threads #iterations\n", argv[0]);
		return 1;
	}

	num_threads = strtol(argv[1], &ptr, 0);
	if (errno || num_threads <= 0 || *ptr != '\0') {
		printf("#threads must be a positive integer.\n");
		return 1;
	}

	num_iterations = strtoll(argv[2], &ptr, 0);
	if (errno || num_iterations <= 0 || *ptr != '\0') {
		printf("#iterations must be a positive integer.\n");
		return 1;
	}

	run_test(num_threads, num_iterations);

	return 0;
}
