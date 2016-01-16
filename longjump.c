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
	int *p = NULL;

	signal(SIGSEGV, catch_segv);

	if (setjmp(jbuf) == 0) {
		printf("%d\n", *p);
	} else {
		printf("Ouch! I crashed!\n");
	}

	printf("exiting.\n");

	return 0;
}
