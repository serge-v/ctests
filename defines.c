#include <stdio.h>

int main()
{
#ifdef __linux__
	printf("__linux__ defined\n");
#endif

#ifdef __osx__
	printf("__osx__ defined\n");
#endif
	printf("int size: %lu\n", sizeof(int));
	printf("long int size: %lu\n", sizeof(long int));
	printf("long: %lu\n", sizeof(long));
	printf("long long: %lu\n", sizeof(long long));
}
