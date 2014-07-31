#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <zlib.h>
#include <bzlib.h>
#include <lzo/lzo1x.h>
#include "timer.h"

int default_read(int mem_align)
{
	struct timer t;
	int rc;
	int count = 0;

	void* buff = NULL;

	if (mem_align)
		rc = posix_memalign(&buff, 4096, 4096);
	else
		buff = malloc(4096);

	FILE* f = fopen("1G.txt", "rt");

	printf("=== default, mem_align: %d ===\n", mem_align);

	reset(&t);

	while (!feof(f))
	{
		fgets(buff, 4096, f);
		count++;
	}

	print_elapsed("lines", count, &t);
	return count;
}

int default_gz_read(int mem_align)
{
	struct timer t;
	int rc;
	int count = 0;

	void* buff = NULL;

	if (mem_align)
		rc = posix_memalign(&buff, 4096, 4096);
	else
		buff = malloc(4096);

	gzFile f = gzopen("1G.gz", "rb");
	if (f == NULL)
	{
		fprintf(stderr, "gzopen error\n");
		exit(1);
	}

	printf("=== default gz, mem_align: %d ===\n", mem_align);

	reset(&t);

	while (!gzeof(f))
	{
		gzgets(f, buff, 4096);
		count++;
	}

	gzclose(f);

	print_elapsed("lines", count, &t);
	return count;
}

int default_lzo_read()
{
	size_t was_read;
	int rc;
	unsigned char buff[4096];
	unsigned char deflate_buff[1024 * 100];
	int count;
	unsigned const char* p;
	lzo_uint dst_len;

	rc = lzo_init();
	if (LZO_E_OK != rc)
	{
		fprintf(stderr, "Cannot init LZO library. Error: %d\n", rc);
		exit(1);
	}

	FILE* f = fopen("1G.txt.lzo", "rb");
	if (!f)
	{
		perror("Cannot open LZO file");
		exit(1);
	}

	while (!feof(f))
	{
		was_read = fread(buff, 1, 4096, f);
		if (was_read <= 0)
			break;

		dst_len = 1024 * 100;
		rc = lzo1x_decompress(buff, was_read, deflate_buff, &dst_len, NULL);
		if (rc != 0)
		{
			fprintf(stderr, "lzo decompress error: %d\n", rc);
			exit(1);
		}

		if (dst_len > 4096 * 3)
		{
			fprintf(stderr, "lzo decompress overflow: %lu\n", dst_len);
			exit(1);
		}

		deflate_buff[dst_len] = 0;
	}

	fclose(f);

	return 0;
}

#define BSIZE 1024*1024*2

int increased_buffer_read(int mem_align)
{
	struct timer t;
	int rc;
	char line[4096];
	int count = 0;
	void* buff = NULL;

	if (mem_align)
		rc = posix_memalign(&buff, 4096, BSIZE);
	else
		buff = malloc(BSIZE);

	FILE* f = fopen("1G.txt", "rt");
	setvbuf(f, buff, _IOFBF, BSIZE);

	printf("=== with 2MB buffer, mem_align: %d ===\n", mem_align);

	reset(&t);

	while (!feof(f))
	{
		fgets(line, 4096, f);
		count++;
	}

	free(buff);

	print_elapsed("lines", count, &t);
	return count;
}

#define TNUM 20
pthread_t threads[TNUM];

enum tests
{
	TEST1,
	TEST2
};

struct targs
{
	enum tests test_num;
	int mem_align;
	int result;
};

void* thread_proc(void* data)
{
	struct targs* args = (struct targs*)data;

	switch (args->test_num)
	{
	case TEST1:
		args->result = default_read(args->mem_align);
		break;

	case TEST2:
		args->result = increased_buffer_read(args->mem_align);
		break;
	}

	return NULL;
}

int main()
{
	int i, rc;
	struct targs args[TNUM];
	struct timer t;
	int count = 0;

//	default_read(0);
//	default_read(1);
	default_lzo_read(0);
	default_lzo_read(1);
//	default_gz_read(0);
//	default_gz_read(1);
//	increased_buffer_read(0);
//	increased_buffer_read(1);

	return 0;

	printf("=== default 20 threads ===\n");
	reset(&t);

	for (i = 0; i < TNUM; i++)
	{
		args[i].test_num = TEST1;
		rc = pthread_create(&threads[i], NULL, thread_proc, &args[i]);
		if (rc < 0)
		{
			perror("pthread_create");
			return 1;
		}
	}

	for (i = 0; i < TNUM; i++)
	{
		rc = pthread_join(threads[i], NULL);
		if (rc < 0)
		{
			perror("pthread_join");
			return 1;
		}
		count += args[i].result;
	}

	print_elapsed("lines total", count, &t);

	count = 0;
	reset(&t);

	for (i = 0; i < TNUM; i++)
	{
		args[i].test_num = TEST2;
		rc = pthread_create(&threads[i], NULL, thread_proc, &args[i]);
		if (rc < 0)
		{
			perror("pthread_create");
			return 1;
		}
	}

	for (i = 0; i < TNUM; i++)
	{
		rc = pthread_join(threads[i], NULL);
		if (rc < 0)
		{
			perror("pthread_join");
			return 1;
		}
		count += args[i].result;
	}
	print_elapsed("lines total", count, &t);

	return 0;
}
