#ifndef __TIMER_H__
#define __TIMER_H__

#include <time.h>
#include <stdint.h>
#include <sys/time.h>

struct timer
{
	struct timeval start;
	struct timeval end;
};

void timer_reset(struct timer* t);
uint64_t timer_elapsed(struct timer* t);
void timer_print_elapsed(const char* msg, int n, struct timer* t);
int timer_format_elapsed(char* buf, size_t size, const char* msg, int n, struct timer* t);

#endif
