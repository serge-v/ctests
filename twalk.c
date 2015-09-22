#include <search.h>
#include <stdio.h>
#include <stdlib.h>

struct record
{
	int id;
	int data;
};

static int
compare(const void *pa, const void *pb)
{
	struct record *r1 = (struct record *)pa;
	struct record *r2 = (struct record *)pb;

	if (r1->id < r2->id) {
		return -1;
	}

	if (r1->id > r2->id) {
		return 1;
	}

	return 0;
}

static void
print_node(const void *node, VISIT order, int level)
{
	struct record *r = *(struct record **)node;

	if (order == preorder || order == leaf) {
		printf("%*sid: %d, data: %d\n", level*4, "", r->id, r->data);
	}
}

static void *tree;

static void
add(uint64_t id)
{
	struct record key = { id, 0 };
	void *found = NULL;
	struct record* rec = NULL;

	found = tfind(&key, &tree, compare);
	printf("tfind: %p\n", found);
	
	if (found == NULL) {
		rec = calloc(1, sizeof(struct record));
		rec->id = id;
		rec->data = 0;
		printf("new rec: %p\n", rec);
		found = tsearch(rec, &tree, compare);
		rec = *(struct record**)found;
		printf("tsearch: %p, id: %d, data: %d\n", found, rec->id, rec->data);
	} else {
		rec = *(struct record**)found;
		printf("exists: %p, id: %d, data: %d\n", found, rec->id, rec->data);
		rec->data++;
	}
}

int
main()
{
	for (int i = 0; i < 1000; i++)
		add(rand()%13);
	
	twalk(tree, print_node);
}
