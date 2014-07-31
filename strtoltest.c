#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

int main(int argc, char**argv)
{
	uint64_t u1 = 0xFFFFFFFFFFFFFFFF;
	int64_t i1 = 0x7FFFFFFFFFFFFFFF;
	int64_t i2 = -0x7FFFFFFFFFFFFFFF;

	printf("uint64: 0x%016llx, %llu, size: %d\n", u1, u1, sizeof(u1));
	printf(" int64: 0x%016llx, %lld, size: %d\n", i1, i1, sizeof(i1));
	printf(" int64: 0x%016llx, %lld, size: %d\n", i2, i2, sizeof(i2));

	const char* s1 = "18446744073709551612";

	i1 = strtol(s1, NULL, 10);
	if (i1 == LONG_MAX || i1 == LONG_MIN)
		perror("i1 overflow");
	printf("i1: int64: 0x%016llx, %lld, size: %d\n", i1, i1, sizeof(i1));

	u1 = strtoul(s1, NULL, 10);
	printf("u1: uint64: 0x%016llx, %llu, size: %d, errno: %d\n", u1, u1, sizeof(u1), errno);

	return 0;
}
