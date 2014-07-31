#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <libaio.h>

static uint64_t time_us()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	uint64_t us = t.tv_sec * 1000000 + t.tv_usec;
	return us;
}

#define BSIZE 1024*1024*4

char line[1024];
char buff[BSIZE];

void sync_fgets()
{
	uint64_t start, elapsed, lines, id;

	FILE* f = fopen("1G.txt", "rt");
	setvbuf(f, buff, _IOFBF, BSIZE);

	lines = 0;
	start = time_us();

	while (!feof(f))
	{
		if (!fgets(line, 1024, f))
			break;

		for (id = 0; id < 1000; id++) {}
		lines++;
	}

	elapsed = (time_us() - start);
	printf("fgets: lines: %lu(%luM), time: %lu ms, %.2fM lps\n", lines, lines / 1000000, elapsed / 1000, (float)lines / (float)elapsed);

out:
	fclose(f);
}

char s[2][BSIZE];

void async_read()
{
	uint64_t start, elapsed, lines, offs, id, was_read;
	int rc, idx, nidx, fd;
	char* p, *end;

	io_context_t ctx;
	struct iocb io;
	struct iocb* pio = &io;
	struct io_event event;

	fd = open("1G.txt", O_RDONLY | O_DIRECT);
	if (fd < 0)
	{
		perror("open");
		goto out;
	}

	lines = 0;
	memset(&ctx, 0, sizeof(io_context_t));
	rc = io_setup(1, &ctx);
	if (rc < 0)
	{
		perror("io_setup");
		goto out;
	}

	start = time_us();

	// read first block synchronously
	idx = 0;
	offs = 0;
	was_read = read(fd, s[idx], BSIZE);
	offs += was_read;

	while (1)
	{
		// start next block
		nidx = (idx + 1) % 2;
		io_prep_pread(&io, fd, s[nidx], BSIZE, offs);

		rc = io_submit(ctx, 1, &pio);
		if (rc < 0)
		{
			perror("io_submit");
			goto out;
		}

		// process read data
		p = s[idx];
		p += 5;

		while (1)
		{
			p = strchr(p, '\n');
			if (!p || (int)p - (int)s[idx] >= was_read)
				break;
			lines++;
			p++;
			for (id = 0; id < 1000; id++) {}
		}

		// wait next block

		struct timespec timeout = {5, 0};
		rc = io_getevents(ctx, 1, 1, &event, &timeout);
		if (rc < 0 || event.res2 != 0)
		{
			perror("io_getevents");
			goto out;
		}

		was_read = event.res;
		if (was_read == 0)
			break;

		idx = nidx;
		offs += was_read;
	}

	elapsed = (time_us() - start);
	printf("aread: lines: %lu(%luM), time: %lu ms, %.2fM lps\n", lines, lines / 1000000, elapsed / 1000, (float)lines / (float)elapsed);

out:
	if (fd > 0)
		close(fd);
}

int main()
{
	sync_fgets();
	async_read();
	return 0;
}
