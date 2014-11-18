#include "timer.h"
#include <stdio.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

int main()
{
	int rc, ffd;

	if (mkfifo("myfifo", 0666) == -1)
		perror("cannot create fifo");

	ffd = open("myfifo", O_RDWR | O_NONBLOCK);
	if (ffd == -1)
	{
		perror("cannot open fifo");
		return -1;
	}

	int efd = epoll_create(1);
	if (efd == -1)
	{
		perror("cannot create epoll descriptor");
		return -1;
	}

	struct epoll_event evt;

	evt.events = EPOLLIN;
	evt.data.fd = ffd;
	rc = epoll_ctl(efd, EPOLL_CTL_ADD, ffd, &evt);
	if (rc == -1)
	{
		printf("cannot add event for fifo %d. errno: %d\n", evt.data.fd, errno);
		return 1;
	}

	struct epoll_event events[1];

	printf("listening...\n");

	struct timer t;
	timer_reset(&t);

	int timeout = 60000;
	char buf[1024];
	int len = 0;
	int count = 0;
	int total_count = 0;
	int errors = 0;

	while (1)
	{
		int signaled = epoll_wait(efd, events, 1, timeout);

		if (signaled == -1)
		{
			printf("epoll_wait. errno: %d\n", errno);
			continue;
		}

		int i;
		for (i = 0; i < signaled; i++)
		{
			if (events[i].events == POLLIN)
			{
				rc = read(events[i].data.fd, buf, 50);
				if (rc == -1)
				{
					errors++;
					break;
				}
				len += rc;
				count++;
				total_count++;
				if (count % 100000 == 0)
				{
					timer_print_elapsed("fifo_read", count, &t);
					printf("len: %d, errors: %d\n", len, errors);
					timer_reset(&t);
					count = 0;
				}
			}
		}
	}

	timer_print_elapsed("fifo_read", total_count, &t);
	printf("len: %d, errors: %d\n", len, errors);

	return 0;
}
