#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main()
{
	int child_pid = fork();

	if (child_pid == 0) {
		int pid = getpid();
		int ppid = getppid();

		printf("child: %d, ppid: %d\n", pid, ppid);
		int rc = execl("/bin/ps", "-ef", NULL);
		printf("parent: execl.rc: %d. %s\n", rc, strerror(errno));
	} else {
		int pid = getpid();
		printf("parent: %d, child_id: %d\n", pid, child_pid);
		int rc = waitpid(child_pid, NULL, 0);
		printf("parent: child stopped: %d. %s\n", rc, strerror(errno));
	}
}
