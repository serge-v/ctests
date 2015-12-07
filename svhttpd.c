/*
 * Monitors event~.txt file and pushes graphical data to the websock clients. 
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <memory.h>
#include <openssl/sha.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include "common/http.h"
#include "common/crypt.h"
#include "common/socket.h"

bool debug = false;

/*
 * accept_key = base64(sha1(sec_websocket_key + guid))
 */
static char *
create_accept_key(const char *req_key)
{
	char s[100];
	const size_t sz = sizeof(s);
	uint8_t sha[20];
	size_t dlen = 50;
	char *resp_key = calloc(1, dlen);

	int rc = snprintf(s, sz, "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", req_key);
	SHA1((unsigned char*)&s, rc, sha);
	base64_encode(sha, sizeof(sha), &resp_key);

	return resp_key;
}

/*
 Example of ws request.
 
 Started on 0.0.0.0:44126
 len: 595, buf:
 GET / HTTP/1.1
 Host: localhost:44126
 Pragma: no-cache
 Cache-Control: no-cache
 Origin: file://
 Sec-WebSocket-Version: 13
 User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.80 Safari/537.36
 Accept-Encoding: gzip, deflate, sdch
 Accept-Language: en-US,en;q=0.8,ru;q=0.6
 Cookie: id=0
 Sec-WebSocket-Key: fpSsxBqJ70cY7FytBAZUjg==
 Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits
 X-Forwarded-For: ::1
 X-Forwarded-Host: localhost:8082
 X-Forwarded-Server: gifts
 Upgrade: WebSocket
 Connection: Upgrade

 */

#define MAX_WS_COUNT 100

struct wsclient
{
	int socket;
};

static char *events_fname = "%s/src/xtree/ctests/events~.txt";
static int kq = -1;
static struct wsclient wsclients[MAX_WS_COUNT];
static int ws_count = 0;
static int fd = -1;

static void
make_frame(struct buf *buf, const struct buf *payload)
{
	uint8_t header[4];
	size_t header_size = 0;
	buf_clean(buf);

	header[0] = 0x81; /* final bit set, text bit set */

	if (payload->len < 126) {
		header[1] = payload->len;
		header_size = 2;
	} else if (payload->len < 32768) {
		header[1] = 126;
		header[2] = (payload->len & 0xFF00) >> 8;
		header[3] = payload->len  & 0xFF;
		header_size = 4;
	} else {
		errx(1, "long data for ws frame");
	}

	buf_append(buf, (const char*)header, header_size);
	buf_append(buf, payload->s, payload->len);
}

static void
broadcast(const struct buf *frame)
{
	size_t i;
	ssize_t sent;

	for (i = 0; i < ws_count; i++) {
		if (wsclients[i].socket == 0)
			continue;

		sent = send(wsclients[i].socket, frame->s, frame->len, 0);
		if (sent == -1) {
			warn("send to %d\n", wsclients[i].socket);
			close(wsclients[i].socket);
			wsclients[i].socket = 0;
		} else if (debug) {
			warn("sent to %d: %zd\n", wsclients[i].socket, sent);
		}
	}
}


static void
send_frame(const char *text, int websocket)
{
	struct buf frame;
	struct buf payload;
	ssize_t sent;

	buf_init(&frame);
	buf_init(&payload);

	buf_appendf(&payload, text);
	make_frame(&frame, &payload);
	sent = send(websocket, frame.s, frame.len, 0);
	if (sent == -1)
		warn("send_frame");
	buf_clean(&frame);
	buf_clean(&payload);
}

static void
read_record(ssize_t len, uint64_t *value)
{
	char s[1024];

	ssize_t sz = read(fd, s, len);
	if (sz == -1)
		err(1, "cpars_read_realtime_record");

	int n = sscanf(s, "%llu\n", value);
	if (n != 1)
		errx(1, "wrong line: %s", s);
}

static int x = 0;
static int y = 0;

static void
send_last(ssize_t len)
{
	uint64_t value = 0;
	struct buf payload, frame;
	buf_init(&frame);
	buf_init(&payload);

	if (x > 100) {
		x = 0;
		y += 50;
	}

	if (y > 1000) {
		y = 0;
		buf_appendf(&payload, "clean=1\n");
	}

	read_record(len, &value);

	if (value == 0) {
		buf_appendf(&payload, "f=red\n");
	} else if (value < 100) {
		buf_appendf(&payload, "f=magenta\n");
		value = 10;
	} else if (value < 200) {
		buf_appendf(&payload, "f=yellow\n");
		value = 20;
	} else {
		buf_appendf(&payload, "f=green\n");
		value = 30;
	}

	buf_appendf(&payload, "p=%d,%d", (x++)*8, y + value);
	make_frame(&frame, &payload);
	broadcast(&frame);
	buf_clean(&payload);
}

static int
upgrade_connection(struct httpreq *req, struct buf *resp)
{
	char *sec_websocket_accept = create_accept_key(req->sec_websocket_key);

	buf_appendf(resp,
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Connection: Upgrade\r\n"
		"Upgrade: websocket\r\n"
		"Sec-WebSocket-Accept: %s\r\n\r\n",
		sec_websocket_accept);

	free(sec_websocket_accept);
	int websocket = req->fd;
	req->ownership_taken = 1;

	int set = 1;
	setsockopt(websocket, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));

	send(websocket, resp->s, resp->len, 0);
	printf("ws connected: %d\n", websocket);
	wsclients[ws_count].socket = websocket;
	ws_count++;

	return websocket;
}

static void
draw_corners(int websocket)
{
	struct buf buf;
	struct buf payload;

	buf_init(&buf);
	buf_init(&payload);
	buf_appendf(&payload,
		    "l=0,-5,0,5\n"
		    "l=-5,0,5,0\n"
		    "l=1000,-5,1000,5\n"
		    "l=995,0,1005,0\n"
		    "l=0,1495,0,1505\n"
		    "l=-5,1500,5,1500\n"
		    );

	make_frame(&buf, &payload);
	send(websocket, buf.s, buf.len, 0);
	buf_clean(&buf);
	buf_clean(&payload);
}

static void
on_request(struct httpreq *req, struct buf *resp, void *data)
{
	if (req->connection_upgrade) {

		if (ws_count >= MAX_WS_COUNT) {
			buf_appendf(resp, "HTTP/1.1 500 limit\r\n\r\n");
			return;
		}

		int websocket = upgrade_connection(req, resp);
		draw_corners(websocket);
	}

	buf_appendf(resp,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: 5\r\n\r\n"
		"test\n");
}

int main(int argc, char **argv)
{
	int port = 44126;

	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-' && argv[i][1] == 'a')
			port = 44125;
	}

	const char *address = "0.0.0.0";
	int i, nev;
	off_t off;
	struct httpd server;
	struct kevent chlist[2];
	struct kevent evlist[2];
	struct timespec tout = { 30, 0 };

	asprintf(&events_fname, events_fname, getenv("HOME"));
	memset(&server, 0, sizeof(struct httpd));

	server.handler = on_request;

	if (httpd_start(&server, address, port) == -1)
		errx(1, "cannot start server");

	kq = kqueue();
	if (kq == -1)
		err(1, "kqueue");

	fd = open(events_fname, O_RDONLY);
	if (fd == -1)
		err(1, "events~.txt");

	off = lseek(fd, 0, SEEK_END);
 	if (off == -1)
		err(1, "lseek");

	EV_SET(&chlist[0], server.listen_socket, EVFILT_READ, EV_ADD, 0, 0, 0);
	EV_SET(&chlist[1], fd, EVFILT_READ, EV_ADD, 0, 0, 0);

	struct buf tick_frame;
	struct buf payload;

	buf_init(&tick_frame);
	buf_init(&payload);

	for (;;) {
		nev = kevent(kq, chlist, 2, evlist, 2, &tout);
		if (nev < 0)
			err(1, "kevent");

		if (nev == 0) {
			buf_clean(&payload);
			buf_appendf(&payload, "f=blue\np=%d,0", (x++)*8);
			make_frame(&tick_frame, &payload);
			broadcast(&tick_frame);
			continue;
		}

		if (evlist[0].flags & EV_EOF)
			errx(1, "socket closed");

		for (i = 0; i < nev; i++) {
			if (evlist[i].flags & EV_ERROR)
				errc(1, evlist[i].data, "EV_ERROR");

			if (evlist[i].ident == server.listen_socket) {
				httpd_accept(&server);
				continue;
			}

			if (evlist[i].ident == fd)
				send_last(evlist[i].data);
		}
	}

	close(kq);

	return 0;
}
