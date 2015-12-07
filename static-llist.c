#include <stdio.h>


struct data
{
	int a;
};

static struct data d1 = { 101 };
static struct data d2 = { 102 };
static struct data d3 = { 103 };
static struct data d4 = { 104 };

struct llist
{
	struct data *d;
	struct llist* next;
};

#define cons(x, y) (struct llist[]){{ x, y }}

//struct llist *list = cons(&d1, cons(&d2, NULL));

struct llist n3 = {&d3, &d4};

struct llist n2 = {&d2, &n3};

struct llist n1 = {&d1, &n2};

int main() {

//	struct llist *p = list;
	struct llist *p = &n1;

	while(p != 0) {
		printf("%d\n", p->d->a);
		p = p->next;
	}
}
