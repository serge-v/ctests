#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

struct array
{
	int    n;     // number of elements
	int    lo;    // min allowed element value
	int    hi;    // max allowed element value
	int    *v;    // elements
};

static void
array_destroy(struct array *a)
{
	memset(a, 0, sizeof(struct array));
}

static void
array_init(struct array *a, int lo, int hi, int size, int *v)
{
	a->n = size;
	a->lo = lo;
	a->hi = hi;
	a->v = v;
}


static void
array_print(struct array *a)
{
	for (int i = 0; i < a->n; i++) {
		printf(" %d ", i);
	}
	printf("\n");

	for (int i = 0; i < a->n; i++) {
		printf("%2d ", a->v[i]);
	}
	printf("\n");
}


static void
array_dedup(struct array *a)
{
	int x = 0;
	int lo = a->lo;
	int hi = a->hi;
	int n = a->n;
	int *v = a->v;

	for (int i = 0; i < n; i++) {

		x = v[i];

		while (x >= lo && x <= hi) {
			if (v[x] > hi) {
				v[x]++;
				x = 0;
			} else {
				int prev = v[x];
				v[x] = hi + 1;
				x = prev;
			}

			if (v[i] > 0 && v[i] <= hi) {
				v[i] = 0;
			}

			if (x == i) {
				break;
			}
		}
	}
}

static void
test(int *v, int size, int lo, int hi)
{
	struct array a;

	array_init(&a, lo, hi, size, v);

	printf("\n===\n");

	if (size < 20) {
		array_print(&a);
	}

	array_dedup(&a);

	if (size < 20) {
		array_print(&a);
	}

	// print counts

	for (int i = 0; i < a.n; i++) {
		if (a.v[i] > hi) {
			printf("%d:%d ", i, a.v[i] - hi);

		}
	}

	printf("\n");
}

int
main(int argc, const char * argv[])
{
	int lo = 1;
	int hi = 10;
	int size = 10;

	int a1[] = { 1, 2, 3, 4, 1, 2, 3, 5, 6, 7 };
	test(a1, size, lo, hi);

	int a2[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	test(a2, size, lo, hi);

	int a3[] = { 0, 1, 1, 0, 1, 0, 1, 0, 9, 9 };
	test(a3, size, lo, hi);

	int a4[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
	test(a4, size, lo, hi);

	int a5[] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
	test(a5, size, lo, hi);

	int a6[] = { 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 };
	test(a6, size, lo, hi);

	size = 15*1000*1000;
	lo = 5*1000*1000;
	hi = 10*1000*1000;

	int *a7 = calloc(size, sizeof(int));

	for (int i = 0; i < size; i++) {
		a7[i] = 5000001 + i%10;
	}

	test(a7, size, lo, hi);

	free(a7);

	return 0;
}
