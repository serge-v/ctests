#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

static uint64_t time_us()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	uint64_t us = t.tv_sec * 1000000 + t.tv_usec;
	return us;
}

#define N 10000
#define CS 32

char b[N][N];
char a[N][N];

void loop1()
{
	int i, j;
	uint64_t start_us = time_us();
	for (i = 0; i < N; i++)
	{
		for (j = 0; j < N; j++)
			a[i][j] = b[i][j];
	}
	printf("loop1: %lu\n", time_us() - start_us);
}

void loop2()
{
	int i, j;
	uint64_t start_us = time_us();
	for (j = 0; j < N; j++)
	{
		for (i = 0; i < N; i++)
			a[i][j] = b[i][j];
	}
	printf("loop2: %lu\n", time_us() - start_us);
}

void loop3()
{
	int i, j, k;
	uint64_t start_us = time_us();
	for (k = 0; k < CS; k++)
		for (i = N / CS * k; i < N / CS * (k + 1); i++)
		{
			for (j = 0; j < N; j++)
				a[i][j] = b[i][j];
		}
	printf("loop3: %lu\n", time_us() - start_us);
}

int main()
{
	int i;
	uint64_t a = 9223372036854775808UL;
	printf("%ld, 0x%08lX\n", a, a);
	printf("%lu\n", a);

	a = 9223372036854775808UL;
	printf("%jd 0x%08lX\n", (int64_t)a, (int64_t)a);

	printf("a: %p\n", a);
	printf("b: %p\n", b);

	for (i = 0; i < 2; i++)
	{
		loop1();
		loop2();
		loop3();
	}
}
