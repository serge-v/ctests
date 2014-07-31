#include <stdio.h>

int main(int argc, char** argv)
{
	FILE* f = popen("ls", "r");
	
	char buff[4097];
	
	do
	{
		size_t read = fread(buff, 1, 4096, f);
		buff[read] = 0;
		printf("%s", buff);
	}
	while (!feof(f));
	
	fclose(f);
}
