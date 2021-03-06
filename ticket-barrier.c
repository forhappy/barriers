/*
 * =============================================================================
 *
 *       Filename:  ticket-barrier.c
 *
 *    Description:  ticket barrier implementation.
 *
 *        Created:  12/26/2012 01:50:41 PM
 *
 *         Author:  Fu Haiping (forhappy), haipingf@gmail.com
 *        Company:  ICT ( Institute Of Computing Technology, CAS )
 *
 * =============================================================================
 */
#include <stdio.h>
#include <pthread.h>
#include <linux/futex.h>
#include <stdint.h>
#include <limits.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include "ticket-barrier.h"

/* Compile Barrier */
#define barrier() asm volatile("": : :"memory")

/* Atomic add, returning the new value after the addition */
#define atomic_add(P, V) __sync_add_and_fetch((P), (V))

/* Atomic add, returning the value before the addition */
#define atomic_xadd(P, V) __sync_fetch_and_add((P), (V))

/* Atomic or */
#define atomic_or(P, V) __sync_or_and_fetch((P), (V))

/* Force a read of the variable */
#define atomic_read(V) (*(volatile typeof(V) *)&(V))

/* Atomic 32 bit exchange */
static inline unsigned xchg_32(void *ptr, unsigned x)
{
	__asm__ __volatile__("xchgl %0,%1"
				:"=r" ((unsigned) x)
				:"m" (*(volatile unsigned *)ptr), "0" (x)
				:"memory");

	return x;
}

/* Atomic 64 bit exchange */
static inline unsigned long long xchg_64(void *ptr, unsigned long long x)
{
	__asm__ __volatile__("xchgq %0,%1"
				:"=r" ((unsigned long long) x)
				:"m" (*(volatile unsigned long long *)ptr), "0" (x)
				:"memory");

	return x;
}

static long sys_futex(void *addr1, int op, int val1,
        struct timespec *timeout, void *addr2, int val3)
{
	return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3);
}

/* Range from count_next to count_next - (1U << 31) is allowed to continue */

void ticket_barrier_init(ticket_barrier_t *b, unsigned count)
{
	b->count_in = 0;
	b->count_out = 0;
	b->count_next = -1;
	b->total = count;
}

void ticket_barrier_destroy(ticket_barrier_t *b)
{
	/* Alter the refcount so we trigger futex wake */
	atomic_add(&b->count_in, -1);

	/* However, threads can be leaving... so wait for them */
	while (1)
	{
		unsigned count_out = atomic_read(b->count_out);

		/* Make sure everything is synchronized */
		barrier();

		if (count_out == atomic_read(b->count_in) + 1) return;

		sys_futex(&b->count_out, FUTEX_WAIT_PRIVATE, count_out, NULL, NULL, 0);
	}
}

int ticket_barrier_wait(ticket_barrier_t *b)
{
	unsigned wait = atomic_xadd(&b->count_in, 1);
	unsigned next;
	unsigned long long temp;

	int ret;

	while (1)
	{
		next = atomic_read(b->count_next);

		/* Have the required number of threads entered? */
		if (wait - next == b->total)
		{
			/* Move to next bunch */
			b->count_next += b->total;

			/* Wake up waiters */
			sys_futex(&b->count_next, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, 0);

			ret = PTHREAD_BARRIER_SERIAL_THREAD;

			break;
		}

		/* Are we allowed to go? */
		if (wait - next >= (1UL << 31))
		{
			ret = 0;
			break;
		}

		/* Go to sleep until our bunch comes up */
		sys_futex(&b->count_next, FUTEX_WAIT_PRIVATE, next, NULL, NULL, 0);
	}

	/* Add to count_out, simultaneously reading count_in */
	temp = atomic_xadd(&b->reset, 1ULL << 32);

	/* Does count_out == count_in? */
	if ((temp >> 32) == (unsigned) temp)
	{
		/* Notify destroyer */
		sys_futex(&b->count_out, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
	}

	return ret;
}
