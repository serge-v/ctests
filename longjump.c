#include <stdio.h>
#include <setjmp.h>
#include <signal.h>

static jmp_buf jbuf;

static void catch_segv()
{
	longjmp(jbuf, 1);
}

int main()
{
	int *p = 0x12345;

	signal(SIGSEGV, catch_segv);

	if (setjmp(jbuf) == 0) {
		printf("%d\n", *p);
	} else {
		printf("Ouch! I crashed!. p = %p\n", p);
	}

	printf("exiting.\n");

	return 0;
}
