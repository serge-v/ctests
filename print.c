/*
test segfault on extra parameters
*/

#include <stdio.h>

int main()
{
	const char *text = "some text\n";
	const char *s = "%8x %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \n";
	printf(s);
}


