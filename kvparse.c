#include <stdio.h>

int main() {
	char k[100], v[100];
	
	char *s = "key1 = value1\n";
	
	int n = sscanf(s, "%[^ =] = %[^\n]", k, v);
	
	printf("n: %d, k: [%s], v: [%s]\n", n, k, v);
}


