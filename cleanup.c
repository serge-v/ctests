#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>

static uint64_t total_slept = 0;
static uint64_t start_time = 0;

uint64_t
get_ticks()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return tv.tv_sec*1000000 + tv.tv_usec;
}

uint64_t
elapsed()
{
	return get_ticks() - start_time;
}

void
mysleep(uint64_t us)
{
	usleep(us);
	total_slept += us;
//	printf("%6llu slept: %llu\n", elapsed(), us);

}

struct info
{
	uint64_t start;
	const char *name;
	uint64_t acc_cost;
};

struct disposer
{
	struct info parent;
};

static struct info tls;

struct disposer
init(const char *name)
{
	struct disposer d;

	d.parent = tls;

	tls.start = get_ticks();
	tls.name = name;
	tls.acc_cost = 0;
	printf("%6llu %s.in\n", tls.start-start_time, tls.name);
	return d;
}


//#define DISPOSABLE __attribute__((__cleanup__(clean_up)))

void
clean_up(struct disposer *d)
{
	uint64_t cost = get_ticks() - tls.start;

	printf("%6llu %s.out %llu us, acc: %llu\n", elapsed(), tls.name, cost, tls.acc_cost);

	d->parent.acc_cost += tls.acc_cost;
	tls = d->parent;
	tls.acc_cost += cost;
	tls.start += tls.acc_cost;
}

void
test_xx()
{
	struct disposer d = init("x300");
	if (0) d.parent.name = NULL;
	mysleep(300000);
	clean_up(&d);
}

void
test_x()
{
	struct disposer d = init("x200");
	if (0) d.parent.name = NULL;
	test_xx();
	mysleep(200000);
	clean_up(&d);
}

void
test()
{
	struct disposer d = init("x100");
	if (0) d.parent.name = NULL;
	test_x();
	mysleep(100000);
	clean_up(&d);
}

int main(int argc, char **argv)
{
	start_time = get_ticks();

	struct disposer d = init("x400");
	if (0) d.parent.name = NULL;

	test();
	mysleep(400000);
	printf("total slept: %llu\n", total_slept);

	clean_up(&d);
	return 0;
}
