// async write test

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <libaio.h>
#include <ucontext.h>
#include "timer.h"

const int N = 10000;
const int MAX_AIO = 10;

io_context_t ctx_id;
char buf[8*1024];
const size_t n_buf = sizeof(buf);
int fd = -1;
size_t written = 0;
size_t total_flushed = 0;
long n_events = 0;

static void
get_aio()
{
	int i;
	struct io_event events[100];
	struct timespec timeout = {1, 0};

	long nr = io_getevents(ctx_id, 1, MAX_AIO, events, &timeout);
	if (nr < 0)
	{
		printf("n_events: %ld\n", n_events);
		perror("io_getevents");
		exit(1);
	}
	n_events -= nr;
//	printf("get: %ld\n", nr);

	for (i = 0; i < nr; i++)
	{
		struct io_event* e = &events[i];

		if (e->res != 0 && e->res2 != 0)
			printf("failed\n");
	}
}

static void
start_aio(size_t number, size_t offset)
{
	struct iocb iocbs[1];
	struct iocb* piocbs[1];

	piocbs[0] = &iocbs[0];

	io_prep_pwrite(&iocbs[0], fd, buf, written, offset);
	long nr = io_submit(ctx_id, 1, piocbs);
	n_events += nr;

	if (n_events > 100)
		printf("submitted at: %lu\n", number);

	if (nr != 1)
	{
		printf("n_events: %ld\n", n_events);
		perror("io_submit");
		exit(1);
	}

	if (n_events < MAX_AIO)
		return;

	get_aio();
}

static void
write_plain()
{
	struct timer t;
	timer_reset(&t);

	FILE* f = fopen("awrite1~.txt", "wb");
	if (f == NULL)
	{
		perror("fopen");
		exit(1);
	}

	int i;
	for (i = 0; i < N; i++)
		fprintf(f, "line: %08d, jsahdkjhsad sa dakshdjsahd sajd sad dfsfsdfsdfsv sdvsd\n", i);
	fclose(f);
	timer_print_elapsed("write_plain", N, &t);
}

static void
write_async()
{
	struct timer t;
	timer_reset(&t);

	int rc = io_setup(MAX_AIO, &ctx_id);
	if (rc != 0)
	{
		perror("io_setup");
		exit(1);
	}

	fd = open("awrite2~.txt", O_CREAT | O_WRONLY, S_IRWXU);
	if (fd == -1)
	{
		perror("open");
		exit(1);
	}

	char* ptr = buf;
	int i, n;

	for (i = 0; i < N; i++)
	{
		n = sprintf(ptr, "line: %08d, jsahdkjhsad sa dakshdjsahd sajd sad dfsfsdfsdfsv sdvsd\n", i);
		ptr += n;
		written += n;
		if (written > n_buf - 100)
		{
			start_aio(i, total_flushed);
			total_flushed += written;
			ptr = buf;
			written = 0;
		}
	}

	// last chunk
	if (written > 0)
	{
		start_aio(i, total_flushed);
		total_flushed += written;
	}

	get_aio();
	close(fd);
	timer_print_elapsed("write_async", N, &t);
	io_destroy(ctx_id);
}

void on_write(int signum, siginfo_t* info, void* data)
{
	ucontext_t* ctx = (ucontext_t*)data;
	printf("on_write called: %d, %p, %p\n", signum, info, ctx);
}

static void
write_signaled()
{
	static int signum = 0;
	static pid_t pid = 0;

	if (signum == 0)
		signum = SIGRTMIN + 1;

	if (pid == 0)
		pid = getpid();

	fd = open("fifo.txt", O_WRONLY, S_IRWXU);

//	fd = mkfifo("fifo.txt", O_CREAT | S_IRWXU);
	if (fd == -1)
	{
		perror("open");
		exit(1);
	}

	int rc;

	rc = fcntl(fd, F_SETOWN, pid);
	printf("F_SETOWN: %d\n", rc);

	rc = fcntl(fd, F_SETSIG, signum);
	printf("F_SETSIG: %d, signum: %d\n", rc, signum);

	rc = fcntl(fd, F_SETFL, fcntl(fd,F_GETFL)|O_NONBLOCK|O_ASYNC);
	printf("F_SETFL: %d, is_set: %d\n", rc, (fcntl(fd,F_GETFL)&(O_NONBLOCK|O_ASYNC)) == (O_NONBLOCK|O_ASYNC));

	struct sigaction act;
	struct sigaction oldact;

	memset(&act, 0, sizeof(struct sigaction));

	act.sa_sigaction = on_write;
	act.sa_flags = SA_SIGINFO;

	rc = sigaction(signum, &act, &oldact);
	printf("sigaction: %d\n", rc);

	rc = write(fd, "test\n", 5);
	printf("write: %d\n", rc);

	sleep(5);
}

int main()
{
	if (0)
		write_signaled();
	if (0)
		write_plain();
	write_async();

	system("diff -q awrite1~.txt awrite2~.txt");

	return 0;
}
