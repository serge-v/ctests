#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>

struct fifo_row
{
	int a;
	STAILQ_ENTRY(fifo_row) list_entry;
};


STAILQ_HEAD(queue, fifo_row) fifo_queue;

int main()
{
	STAILQ_INIT(&fifo_queue);
	printf("init: %p %p\n", fifo_queue.stqh_first, fifo_queue.stqh_last);

	struct fifo_row *row = calloc(1, sizeof(struct fifo_row));
	printf("row: %p\n", row);
	row->a = 100;

//	STAILQ_INSERT_TAIL(&fifo_queue, row, list_entry);

	STAILQ_NEXT((row), list_entry) = NULL;

	printf("1: %p %p\n", fifo_queue.stqh_first, fifo_queue.stqh_last);

	*(&fifo_queue)->stqh_last = (row);

	printf("2: %p %p\n", fifo_queue.stqh_first, fifo_queue.stqh_last);

	(&fifo_queue)->stqh_last = &STAILQ_NEXT((row), list_entry);
	
	printf("3: %p %p\n", fifo_queue.stqh_first, fifo_queue.stqh_last);
}
