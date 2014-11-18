#!/bin/sh
gcc -o test -x c - <<EOF

#include <stdio.h>

int main()
{
	printf("test -- ok\n");
}

EOF
./test
echo ====
make
edit server.c:30
