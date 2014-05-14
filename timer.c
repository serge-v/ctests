#include "timer.h"
#include <stdio.h>
#include <memory.h>

void timer_reset(struct timer* t)
{
	memset(t, 0, sizeof(struct timer));
	gettimeofday( &t->start, NULL );
}

uint64_t timer_elapsed(struct timer* t)
{
	uint64_t us;
	gettimeofday( &t->end, NULL );
	us = ( ( t->end.tv_sec - t->start.tv_sec ) * 1000000 + ( t->end.tv_usec - t->start.tv_usec ) );
	return us;
}

void timer_print_elapsed( const char* msg, int n, struct timer* t )
{
	uint64_t us = timer_elapsed( t );
	printf( "%s. count: %d, time: %0.2f ms, speed: %0.2f kps, one: %0.2f us\n",
	        msg, n, ( float )us / 1000.0, ( float )n / ( float )us * 1000.0, ( float )us / ( float )n );
}

int timer_format_elapsed(char* buf, size_t size, const char* msg, int n, struct timer* t)
{
	uint64_t us = timer_elapsed( t );
	return snprintf(buf, size, "%s. count: %d, time: %0.2f ms, speed: %0.2f kps, one: %0.2f us\n",
	                msg, n, ( float )us / 1000.0, ( float )n / ( float )us * 1000.0, ( float )us / ( float )n );
}

