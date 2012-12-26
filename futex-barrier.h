/*
 * =============================================================================
 *
 *       Filename:  futex-barrier.h
 *
 *    Description:  futex barrier implementation.
 *
 *        Created:  12/26/2012 11:52:52 AM
 *
 *         Author:  Fu Haiping (forhappy), haipingf@gmail.com
 *        Company:  ICT ( Institute Of Computing Technology, CAS )
 *
 * =============================================================================
 */
#ifndef _FUTEX_BARRIER_H_
#define _FUTEX_BARRIER_H_

typedef struct pool_barrier_s_ pool_barrier_t;

struct pool_barrier_s_
{
	union
	{
		struct
		{
			unsigned seq;
			unsigned count;
		};
		unsigned long long reset;
	};
	unsigned refcount;
	unsigned total;
};

extern void pool_barrier_init(pool_barrier_t *b, unsigned count);

extern void pool_barrier_destroy(pool_barrier_t *b);

extern int pool_barrier_wait(pool_barrier_t *b);

extern int pool_barrier_wait2(pool_barrier_t *b);

#endif /* _FUTEX_BARRIER_H_ */
