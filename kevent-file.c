#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <err.h>
#include <sys/uio.h>
#include <unistd.h>
     
int main()
{
	struct timespec tout = { 10, 0 };
	struct kevent chlist[1];
	struct kevent evlist[1];
	int kq, nev, i, fd, count = 0;
	ssize_t sz;

	kq = kqueue();
	if (kq == -1)
		err(1, "kqueue");
	
	fd = open("events~.txt", O_RDONLY);
	if (fd == -1)
		err(1, "events~.txt");

	printf("fd: %d\n", fd);

	EV_SET(&chlist[0], fd, EVFILT_READ, EV_ADD, 0, 0, 0);
	
	char buf[4096];

	for (;;) {
		nev = kevent(kq, chlist, 1, evlist, 1, &tout);
		if (nev < 0)
			err(1, "kevent");

		if (nev == 0) {
			printf("tout\n");
			continue;
		}

		printf("nev: %d\n", nev);

		if (evlist[0].flags & EV_EOF)
			errx(1, "socket closed");

		for (i = 0; i < nev; i++) {
			if (evlist[i].flags & EV_ERROR)
				errc(1, evlist[i].data, "EV_ERROR");

			printf("-------------------\n");
			printf("ident: %lu\n",  evlist[i].ident);
			printf("filter: %d\n",  evlist[i].filter);
			printf("flags: %02X\n",  evlist[i].flags);
			printf("fflags: %04X\n",  evlist[i].fflags);
			printf("data: %ld\n",  evlist[i].data);
			printf("udata: %p\n",  evlist[i].udata);
			
			sz = read(evlist[i].ident, buf, evlist[i].data);
			printf("read: %zd\n",  sz);
		}
	}
}
