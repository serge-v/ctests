/*
use: -std=c11.
Anyway it doesn't compile in gcc 4.8.2.
*/

#include <stdio.h>
#include <threads.h>

void thread_func(void* p)
{
}

int main()
{
	thrd_t thr;
	int h = thrd_create(&thr, thread_func, nullptr);
}
