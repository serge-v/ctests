#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include "sockets.h"

struct httpreq
{
	const char* method;
	const char* path;
};

int httpreq_parse(char* s, int len, struct httpreq* req)
{
	char* p = s;
	memset(req, 0, sizeof(struct httpreq));

	req->method = p;
	p = strchr(p, ' ');
	if (p < s || p >= s + len)
		return -1;
	*p = 0;
	p++;
	req->path = p;

	p = strchr(p, ' ');
	if (p < s || p >= s + len)
		return -1;

	*p = 0;

	return 0;
}

int main()
{
	int rc, asocket, lsocket = -1;
	struct sockaddr si_from;
	ssize_t size;
	char rbuf[20000];
	char sbuf[20000];
	struct httpreq req;
	socklen_t addrlen = sizeof(si_from);

	if ((lsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		perror("cannot create socket");
		return -1;
	}

	if (set_reuse_addr(lsocket) == -1)
	{
		perror("cannot reuse socket");
		return -1;
	}

	if (bind_address(lsocket, "0.0.0.0", 44126) == -1)
	{
		perror("cannot bind socket");
		goto err;
	}

	if (listen(lsocket, 100) == -1)
	{
		perror("cannot listen socket");
		goto err;
	}

	while (1)
	{
		asocket = accept(lsocket, &si_from, &addrlen);
		if (asocket == -1)
		{
			perror("cannot accept socket");
			goto err;
		}

		size = recv(asocket, rbuf, sizeof(rbuf), 0);
		rc = sprintf(sbuf, "HTTP/1.1 200 OK\r\n\r\nsize: %lu, buf:\r\n%s\r\n", size, rbuf);
		send(asocket, sbuf, rc, 0);
		close(asocket);

		if (size == -1)
		{
			perror("cannot recv");
			goto err;
		}

		if (size == 0)
			continue;

		rbuf[size] = 0;
		printf("len: %lu, buf:\n%s\n", size, rbuf);

		httpreq_parse(rbuf, size, &req);
		printf("method: %s\n", req.method);
		printf("path:   %s\n", req.path);
		if (strcmp("/q", req.path) == 0)
			break;
	}

err:
	close(lsocket);
	return 1;
}
