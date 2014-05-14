#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE (1024*1024)

uint64_t produced = 0;
uint64_t buffer[BUFFER_SIZE];

struct cdata
{
	int               id;
	pthread_t         tid;
	int      stop;
	uint64_t consumed;
};

static struct cdata consumers[] =
{
	{0, 0, 0, 0},
	{1, 0, 0, 0},
	{2, 0, 0, 0},
	{3, 0, 0, 0},
};

size_t n_consumers = sizeof(consumers) / sizeof(consumers[0]);

static void wait_consumed()
{
	size_t i;
	uint64_t min_consumed;

	while (1)
	{
		min_consumed = 0xFFFFFFFFFFFFFFFF;
		for (i = 0; i < n_consumers; i++)
		{
			if (consumers[i].consumed < min_consumed)
				min_consumed = consumers[i].consumed;
		}

		if (produced - min_consumed < BUFFER_SIZE)
			break;

		usleep(1);
	}
}

void* producer(void* data)
{
	uint64_t seq_num = 1;

	if (0)
		printf("%p\n", data);

	while (1)
	{
		wait_consumed();

		buffer[produced % BUFFER_SIZE] = seq_num++;

//		asm volatile ("" : : : "memory");
//		asm volatile("mfence":::"memory");
		produced += 1;
//		__sync_fetch_and_add(&produced, 1);

		if (produced > 10000000)
			return NULL;
	}

	return NULL;
}

static int wait_produced(uint64_t to_consume, struct cdata* d)
{
	while ((to_consume + 1) > produced)
	{
		usleep(1);
		if (d->stop == 1)
			return 1;
	}

	return 0;
}

void* consumer(void* data)
{
	struct cdata* d = (struct cdata*)data;
	uint64_t to_consume = d->id;

	while (1)
	{
		if (wait_produced(to_consume, d) == 1)
			return NULL;

		uint64_t token = buffer[to_consume % BUFFER_SIZE];
		if (token == 0)
		{
			printf("consumer%d: sequence broken at produced: %lu, to_consume: %lu, idx: %lu\n",
			       d->id, produced, to_consume, to_consume % BUFFER_SIZE);
			exit(1);
		}

		buffer[to_consume % BUFFER_SIZE] = 0;

		// memory_barrier should be here;

		d->consumed = to_consume;
		to_consume += n_consumers;
	}

	return 0;
}

int main()
{
	pthread_t t1;
	int rc;
	size_t i;

	for (i = 0; i < n_consumers; i++)
	{
		rc = pthread_create(&consumers[i].tid, NULL, consumer, (void*)&consumers[i].id);
		if (rc != 0)
		{
			fprintf(stderr, "pthread_create2 error: %d\n", rc);
			exit(1);
		}
	}

	rc = pthread_create(&t1, NULL, producer, (void*)"producer");
	if (rc != 0)
	{
		fprintf(stderr, "pthread_create1 error: %d\n", rc);
		exit(1);
	}

	pthread_join(t1, NULL);

	for (i = 0; i < n_consumers; i++)
		consumers[i].stop = 1;

	for (i = 0; i < n_consumers; i++)
		pthread_join(consumers[i].tid, NULL);

	return 0;
}


