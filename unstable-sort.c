#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct item
{
	int key;
	char *value;
};

static int cmp_item(const void *a, const void *b)
{
	struct item *x = (struct item *)a;
	struct item *y = (struct item *)b;

	if (x->key > y->key)
		return 1;

	if (x->key < y->key)
		return -1;

	return 0;
}

#define N 30

int main()
{
	int i, j, bad = 0;
	char s[100];

	struct item *items1 = calloc(N, sizeof(struct item));
	struct item *items2 = calloc(N, sizeof(struct item));
	
	for (i = 0; i < N; i++) {
		items1[i].key = i / 4;
		asprintf(&items1[i].value, "item%d", i);

		items2[i].key = i / 4;
		asprintf(&items2[i].value, "item%d", i);
	}

	srand(time(NULL));

	for (i = 0; i < rand()%3; i++)
		qsort(items1, N, sizeof(struct item), cmp_item);

	for (i = 0; i < rand()%3; i++)
		qsort(items2, N, sizeof(struct item), cmp_item);

	for (i = 0; i < N; i++) {
		if (strcmp(items1[i].value, items2[i].value) != 0) {
			printf("%d %d=%s %d=%s\n", i, items1[i].key, items1[i].value, items1[i].key, items2[i].value);
			bad++;
		}
	}
	printf("bad: %d\n", bad);
}

