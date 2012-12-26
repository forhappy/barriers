/*
 * =============================================================================
 *
 *       Filename:  fast-barrier.c
 *
 *    Description:  fast barrier implementation.
 *
 *        Created:  12/26/2012 02:15:11 PM
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

#include "fast-barrier.h"

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
				:"=r" ((unsigned long  long) x), "=m"(*(unsigned long long *)ptr)
				:"m" (*(volatile unsigned long long *)ptr), "0" (x)
				:"memory");

	return x;
}

static long sys_futex(void *addr1, int op, int val1,
        struct timespec *timeout, void *addr2, int val3)
{
	return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3);
}

void fast_barrier_init(fast_barrier_t *b, pthread_barrierattr_t *a, unsigned count)
{
	b->seq = 0;
	b->count = 0;
	b->refcount = 1;
	/* Total of waiting threads */
	b->total = count - 1;

	if (count < sysconf(_SC_NPROCESSORS_ONLN))
	{
		b->spins = 1000;
	}
	else
	{
		b->spins = 1;
	}

	/* Default to process private */
	b->flags = FUTEX_PRIVATE_FLAG;
	if (a)
	{
		/* Check for a shared barrier */
		int shared = 0;
		pthread_barrierattr_getpshared(a, &shared);
		if (shared) b->flags = 0;
	}
}

void fast_barrier_destroy(fast_barrier_t *b)
{
	/* Trigger futex wake */
	atomic_add(&b->refcount, -1);

	/* Wait until all references to the barrier are dropped */
	while (1)
	{
		unsigned ref = atomic_read(b->refcount);

		if (!ref) return;

		sys_futex(&b->refcount, FUTEX_WAIT | b->flags, ref, NULL, NULL, 0);
	}
}

int fast_barrier_wait(fast_barrier_t *b)
{
	int ret;

	atomic_add(&b->refcount, 1);

	while (1)
	{
		unsigned seq = atomic_read(b->seq);
		unsigned count = atomic_xadd(&b->count, 1);

		if (count < b->total)
		{
			int i;
			seq |= 1;

			for (i = 0; i < b->spins; i++)
			{
				if ((atomic_read(b->seq) | 1) != seq) break;
			}

			/* Can we proceed? */
			while ((atomic_read(b->seq) | 1) == seq)
			{
				/* Hack - set a flag that says we are sleeping */
				*(volatile char *) &b->seq = 1;

				/* Sleep on it instead */
				sys_futex(&b->seq, FUTEX_WAIT | b->flags, seq, NULL, NULL, 0);
			}

			ret = 0;
			break;
		}

		if (count == b->total)
		{
			/* Simultaneously clear count, increment sequence number, and clear wait flag */
			seq = atomic_read(b->seq);

			if (xchg_64(&b->reset, (seq | 1) + 255) & 1)
			{
				/* Wake up sleeping threads */
				sys_futex(&b->seq, FUTEX_WAKE | b->flags, INT_MAX, NULL, NULL, 0);
			}

			ret = PTHREAD_BARRIER_SERIAL_THREAD;
			break;
		}

		seq |= 1;
		/* Hack - set a flag that says we are sleeping */
		*(volatile char *) &b->seq = 1;

		/* We were too slow... wait for the barrier to be released */
		sys_futex(&b->seq, FUTEX_WAIT | b->flags, seq, NULL, NULL, 0);
	}

	/* Are we the last to wake up? */
	if (atomic_xadd(&b->refcount, -1) == 1)
	{
		/* Wake destroying thread */
		sys_futex(&b->refcount, FUTEX_WAKE | b->flags, 1, NULL, NULL, 0);
	}
	return ret;
}

