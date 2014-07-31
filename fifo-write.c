#include "timer.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

int main()
{
	int i, rc, len, ffd;

	ffd = open("myfifo", O_RDWR | O_NONBLOCK);
	if (ffd == -1)
	{
		perror("cannot open fifo");
		return -1;
	}

	printf("writing fifo...\n");

	struct timer t;
	timer_reset(&t);

	const int N = 1000000;
	int errors = 0;
	char buf[1024];
	len = sprintf(buf, "01234567890123456789012345678901234567890123456789");

	for (i = 0; i < N; i++)
	{
		do
		{
			rc = write(ffd, buf, len);
			if (rc == -1)
			{
				errors++;
				usleep(2000);
			}
		}
		while (rc == -1);

		if (rc != len)
		{
			perror("cannot write to fifo");
			break;
		}
	}

	timer_print_elapsed("fifo_write", i, &t);
	printf("errors: %d\n", errors);

	return 0;
}
