#include "sockets.h"
#include <fcntl.h>
#include <memory.h>
#include <stdio.h>
#include <errno.h>

int create_tcp_socket(const char* address, int port, struct sockaddr_in* si, int* s)
{
	if ((*s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		return -1;

	memset(si, 0, sizeof(struct sockaddr_in));
	si->sin_family = AF_INET;
	si->sin_port = htons(port);

	if (inet_aton(address, &si->sin_addr) == 0)
		return -1;

	return 0;
}

int create_udp_socket(const char* address, int port, struct sockaddr_in* si, int* s)
{
	if ((*s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		return -1;

	memset(si, 0, sizeof(struct sockaddr_in));
	si->sin_family = AF_INET;
	si->sin_port = htons(port);

	if (inet_aton(address, &si->sin_addr) == 0)
		return -1;

	return 0;
}

int set_reuse_addr(int socket)
{
	int one = 1;

	if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)) == -1)
		return -1;
	return 0;
}

int set_multicast_if(int socket, const char* address, int port)
{
	struct sockaddr_in si;
	memset(&si, 0, sizeof(struct sockaddr_in));

	si.sin_family = AF_INET;
	si.sin_port = htons(port);
	si.sin_addr.s_addr = inet_addr(address);

	if (setsockopt(socket, IPPROTO_IP, IP_MULTICAST_IF, &si.sin_addr, sizeof (si.sin_addr)) < 0)
		return -1;
	return 0;
}

int set_rcv_timeout(int socket, int seconds, int microseconds)
{
	struct timeval tv;
	tv.tv_sec = seconds;
	tv.tv_usec = microseconds;
	if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval)) == -1)
		return -1;
	return 0;
}

int set_nonblock(int socket)
{
	int flags = fcntl(socket, F_GETFL, 0);
	if (flags == -1)
		return -1;

	if (fcntl(socket, F_SETFL, flags|O_NONBLOCK) < 0)
		return -1;

	return 0;
}

int bind_address(int socket, const char* address, int port)
{
	struct sockaddr_in si;
	memset(&si, 0, sizeof(si));

	si.sin_family = AF_INET;
	si.sin_port = htons(port);
	si.sin_addr.s_addr = inet_addr(address);

	if (bind(socket, (struct sockaddr *)&si, sizeof(si)) < 0)
		return -1;
	return 0;
}

int add_membership(int socket, const char* address)
{
	struct ip_mreq mreq;
	memset(&mreq, 0, sizeof(mreq));

	mreq.imr_multiaddr.s_addr = inet_addr(address);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);

	if (setsockopt(socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1)
		return -1;
	return 0;
}

int send_command(const char* address, int port, const char* command, int clients)
{
	struct sockaddr_in si, si_remote;
	int s, i, len, rc = 0;
	socklen_t slen = sizeof(si);
	const int BUFLEN = 4096;
	char buf[BUFLEN];

	create_udp_socket(address, port, &si, &s);
	set_rcv_timeout(s, 1, 0);

	sprintf(buf, "%s", command);

	if (sendto(s, buf, strlen(buf), 0, (struct sockaddr*)&si, slen) == -1)
		return -1;

	for (i = 0; i < clients; i++)
	{
		len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr*)&si_remote, &slen);
		if (len == -1)
		{
			printf("recvfrom: %d\n", errno);
			rc = -1;
		}
		else
		{
			buf[len] = 0;
			printf("S: %s\n", buf);
		}
	}

	if (close(s) == -1)
		rc = -1;

	return rc;
}
