#include <stdio.h>
#include <stdint.h>

void
fib(int n)
{
	int i = 0;

	printf("0 1 ");
	uint64_t num = 0, prev = 1, pprev = 0;

	for (i = 0; i < n; i++) {
		num = prev + pprev;

		printf("%llu ", num);

		pprev = prev;
		prev = num;
	}
	printf("\n");
}

int
main()
{
	fib(10);
	fib(20);
	fib(30);
}

