#include <stdio.h>

const char* list[] = {
	"obama",
	"obamb",
	"nixon",
	"bush",
	"kennedy",
	"lincoln",
	"reagan",
	"clinton",
	"Jobs",
	"Gates",
	"Ballmer",
};

size_t n_list = sizeof(list) / sizeof(list[0]);

int main()
{
	int i;
	
	for (i = 0; i < n_list; i++)
	{
		int a = *(int*)list[i];
		printf("%20s: %lx %d\n", list[i], a, *(int*)list[i] % 4796 % 275 % 4);
	}
	
	printf("%lu\n", sizeof(int));

	return 0;
}
