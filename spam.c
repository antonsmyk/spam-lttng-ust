/*
 * The spam-lttng-ust utility
 * Copyright 2017 Itiviti AB, Anton Smyk <Anton.Smyk@itiviti.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include "spam-trace.h"

#define MILLIARD 1000000000
#define MAX_THREADS 64

static unsigned long events_per_sec;
static unsigned long limit_run_sec;
static unsigned num_threads = 1;
pthread_t threads[MAX_THREADS];

static void * spammer_thread(void *arg __attribute__((unused)) ) {
	const long sleep_nsec = MILLIARD / events_per_sec;
	struct timespec start;
	clock_gettime(CLOCK_REALTIME, &start);
	struct timespec stop = { start.tv_sec + limit_run_sec, 0 };
	for (;;) {
		struct timespec current;
		clock_gettime(CLOCK_REALTIME, &current);

		uint64_t timestamp = ((uint64_t)current.tv_sec) * MILLIARD + current.tv_nsec;
		tracepoint(spam, dummy_event, timestamp);

		if (current.tv_sec  > stop.tv_sec)
			break;

		const struct timespec sleep_ts = { 0, sleep_nsec };
		nanosleep(&sleep_ts, NULL);
	}
	return NULL;
}

int main(int argc, char **argv) {
	if (argc <= 3) {
		fprintf(stderr, "Usage: spam <rate per second> <period in seconds> [number of threads]\n");
		return 1;
	}
	events_per_sec = atol(argv[1]);
	limit_run_sec = atol(argv[2]);
	if (argc >= 4)
		num_threads = atoi(argv[3]);
	if (num_threads > MAX_THREADS) {
		fprintf(stderr, "Can't run more than %u threads!\n", MAX_THREADS);
		return 2;
	}
	printf("Started spamming with %lu trace events per second for %lu seconds using %u concurrent threads...\n",
			events_per_sec, limit_run_sec, num_threads);
	for (unsigned i = 0; i < num_threads; i++) {
		int r = pthread_create(&threads[i], NULL, spammer_thread, NULL);
		if (r != 0) {
			fprintf(stderr, "Couldn't start thread: %d, %s\n", r, strerror(r));
			return 3;
		}
	}
	for (unsigned i = 0; i < num_threads; i++) {
		void *retval = NULL;
		int r = pthread_join(threads[i], &retval);
		if (r != 0) {
			fprintf(stderr, "Couldn't join thread: %d, %s\n", r, strerror(r));
			/* ignore error */
		}
	}
	printf("Stopped spam!\n");
}
