#include <stdio.h>

int main()
{
#ifdef __linux__
	printf("__linux__ defined\n");
#endif

#ifdef __osx__
	printf("__osx__ defined\n");
#endif

}
