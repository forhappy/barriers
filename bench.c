/*
 * =============================================================================
 *
 *       Filename:  pthread-barrier-main.c
 *
 *    Description:  pthread barrier main testcase.
 *
 *        Created:  12/26/2012 11:11:12 AM
 *
 *         Author:  Fu Haiping (forhappy), haipingf@gmail.com
 *        Company:  ICT ( Institute Of Computing Technology, CAS )
 *
 * =============================================================================
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "pthread-barrier.h"
#include "futex-barrier.h"
#include "ticket-barrier.h"
#include "fast-barrier.h"

// #define USE_MUTEX_CV_BARRIER
#define USE_FUTEX_BARRIER
// #define USE_TICKET_BARRIER
// #define USE_FAST_BARRIER

#if defined(USE_MUTEX_CV_BARRIER)
  #define pthread_barrier_t barrier_t
  #define pthread_barrier_init(B, A, N) (barrier_init((B), (N)), 0)
  #define pthread_barrier_destroy(B) (barrier_destroy(B), 0)
  #define pthread_barrier_wait barrier_wait

#elif defined(USE_FUTEX_BARRIER)
  #define pthread_barrier_t pool_barrier_t
  #define pthread_barrier_init(B, A, N) (pool_barrier_init((B), (N)), 0)
  #define pthread_barrier_destroy(B) (pool_barrier_destroy(B), 0)
  #define pthread_barrier_wait pool_barrier_wait

#elif defined(USE_TICKET_BARRIER)
  #define pthread_barrier_t ticket_barrier_t
  #define pthread_barrier_init(B, A, N) (ticket_barrier_init((B), (N)), 0)
  #define pthread_barrier_destroy(B) (ticket_barrier_destroy(B), 0)
  #define pthread_barrier_wait ticket_barrier_wait

#elif defined(USE_FAST_BARRIER)
  #define pthread_barrier_t fast_barrier_t
  #define pthread_barrier_init(B, A, N) (fast_barrier_init((B), (A), (N)))
  #define pthread_barrier_destroy(B) (fast_barrier_destroy(B), 0)
  #define pthread_barrier_wait fast_barrier_wait

#endif

/* Number of threads to use */
#define N 2

/* Number of times to call barrier primitive */
#define COUNT 1000000

static pthread_barrier_t barrier;

static void *do_barrier_bench(void *arg)
{
	int i;

	(void) arg;

	for (i = 0; i < COUNT; i++)
	{
		pthread_barrier_wait(&barrier);
	}

	return NULL;
}

int main(void)
{
	pthread_t th[N - 1];
    clock_t start, end;
	int i;

	pthread_barrier_init(&barrier, NULL, N);

	for (i = 0; i < N - 1; i++) {
		if (pthread_create(&th[i], NULL, do_barrier_bench, NULL)) {
			fprintf(stderr, "pthread_create failed");
            exit(EXIT_FAILURE);
		}
	}
    start = clock();

	do_barrier_bench(NULL);

    end = clock();

	pthread_barrier_destroy(&barrier);

	for (i = 0; i < N - 1; i++) {
		if (pthread_join(th[i], NULL)) {
            fprintf(stderr, "pthread_join failed");
            exit(EXIT_FAILURE);
        }
	}

	puts("bench OK");

    printf("time elasped %lds.\n", (end - start) / CLOCKS_PER_SEC);
    return 0;
}
