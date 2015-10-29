#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <memory.h>
#include <openssl/sha.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include "../common/http.h"
#include "../common/crypt.h"
#include "../common/socket.h"

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

static int kq = -1;
static struct wsclient wsclients[MAX_WS_COUNT];
static int ws_count = 0;

static void
on_request(struct httpreq *req, struct buf *resp, void *data)
{
	if (req->connection_upgrade) {

		if (ws_count >= MAX_WS_COUNT) {
			buf_appendf(resp, "HTTP/1.1 500 limit\r\n\r\n");
			return;
		}

		char *sec_websocket_accept = create_accept_key(req->sec_websocket_key);

		buf_appendf(resp, "HTTP/1.1 101 Switching Protocols\r\n");
		buf_appendf(resp, "Connection: Upgrade\r\n");
		buf_appendf(resp, "Upgrade: websocket\r\n");
		buf_appendf(resp, "Sec-WebSocket-Accept: %s\r\n\r\n", sec_websocket_accept);

		free(sec_websocket_accept);
		int websocket = req->fd;
		req->ownership_taken = 1;
		send(websocket, resp->s, resp->len, 0);
		printf("ws connected: %d\n", websocket);
		wsclients[ws_count].socket = websocket;
		ws_count++;
		return;
	}

	buf_appendf(resp, "HTTP/1.1 200 OK\r\n");
	buf_appendf(resp, "Content-Type: text/plain\r\n");
	buf_appendf(resp, "Content-Length: 5\r\n\r\n");
	buf_appendf(resp, "test\n");
}

static void
make_frame(struct buf *buf, const struct buf *payload)
{
	uint8_t header[4];
	size_t header_size = 0;

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

int main()
{
	const int port = 44126;
	const char *address = "0.0.0.0";
	int i, nev;
	struct httpd server;

	memset(&server, 0, sizeof(struct httpd));

	server.handler = on_request;

	if (httpd_start(&server, address, port) == -1)
		errx(1, "cannot start server");

	kq = kqueue();
	if (kq == -1)
		err(1, "kqueue");

	struct timespec tout = { 5, 0 };
	struct kevent chlist[1];
	struct kevent evlist[1];

	EV_SET(&chlist[0], server.listen_socket, EVFILT_READ, EV_ADD, 0, 0, 0);

	struct buf tick_frame;
	struct buf payload;

	buf_init(&tick_frame);
	buf_init(&payload);
	buf_appendf(&payload, "tick");

	make_frame(&tick_frame, &payload);

	for (;;) {
		nev = kevent(kq, chlist, 1, evlist, 1, &tout);
		if (nev < 0)
			err(1, "kevent");

		if (nev == 0) {
			for (i = 0; i < ws_count; i++) {
				ssize_t sent = send(wsclients[i].socket, tick_frame.s, tick_frame.len, 0);
				printf("tick sent: %d:%d %zd\n", i, wsclients[i].socket, sent);
			}
			continue;
		}

		if (evlist[0].flags & EV_EOF)
			errx(1, "socket closed");

		for (i = 0; i < nev; i++) {
			if (evlist[i].flags & EV_ERROR)
				errc(1, evlist[i].data, "EV_ERROR");

			if (evlist[i].ident == server.listen_socket)
				httpd_accept(&server);
		}
	}

	close(kq);

	return 0;
}
