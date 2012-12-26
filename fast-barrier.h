/*
 * =============================================================================
 *
 *       Filename:  fast-barrier.h
 *
 *    Description:  fast barrier implementation.
 *
 *        Created:  12/26/2012 02:15:03 PM
 *
 *         Author:  Fu Haiping (forhappy), haipingf@gmail.com
 *        Company:  ICT ( Institute Of Computing Technology, CAS )
 *
 * =============================================================================
 */
#ifndef _FAST_BARRIER_H_
#define _FAST_BARRIER_H_
#include <inttypes.h>
#include <pthread.h>

typedef struct fast_barrier_s_ fast_barrier_t;

struct fast_barrier_s_
{
	union
	{
		struct
		{
			unsigned seq;
			unsigned count;
		};
		uint64_t reset;
	};
	unsigned refcount;
	unsigned total;
	int spins;
	unsigned flags;
};

extern void fast_barrier_init(fast_barrier_t *b, pthread_barrierattr_t *a, unsigned count);

extern int fast_barrier_wait(fast_barrier_t *b);

extern void fast_barrier_destroy(fast_barrier_t *b);

#endif /* _FAST_BARRIER_H_ */
