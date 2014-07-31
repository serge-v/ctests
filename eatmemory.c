#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>

int main(int argc, char** argv)
{
	uint64_t size_mb = 10;
	if (argc > 1)
		size_mb = strtoul(argv[1], NULL, 10);

	uint64_t size = size_mb * 1024 * 1024;
	void* mem = malloc(size);
	memset(mem, 0, size);
	printf("Allocated %luM\n", size_mb);
	printf("Press ENTER to stop.\n");
	getchar();
}
