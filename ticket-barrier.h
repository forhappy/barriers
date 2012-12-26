/*
 * =============================================================================
 *
 *       Filename:  ticket-barrier.h
 *
 *    Description:  ticket barrier
 *
 *        Created:  12/26/2012 01:50:26 PM
 *
 *         Author:  Fu Haiping (forhappy), haipingf@gmail.com
 *        Company:  ICT ( Institute Of Computing Technology, CAS )
 *
 * =============================================================================
 */
#ifndef _TICKET_BARRIER_H_
#define _TICKET_BARRIER_H_

typedef struct ticket_barrier_s_ ticket_barrier_t;

struct ticket_barrier_s_
{
	unsigned count_next;
	unsigned total;
	union
	{
		struct
		{
			unsigned count_in;
			unsigned count_out;
		};
		unsigned long long reset;
	};
};

/* Range from count_next to count_next - (1U << 31) is allowed to continue */


extern void ticket_barrier_init(ticket_barrier_t *b, unsigned count);

extern void ticket_barrier_destroy(ticket_barrier_t *b);

extern int ticket_barrier_wait(ticket_barrier_t *b);

#endif /* _TICKET_BARRIER_H_ */
