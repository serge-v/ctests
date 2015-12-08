/*
 * Simple example of shared drawing using web browsed and websocket server.
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
#include "drawpad.html.h"

bool debug = true;

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

#define MAX_WS_COUNT 100

struct wsclient
{
	int socket;
};

static int kq = -1;
static struct wsclient wsclients[MAX_WS_COUNT];
static int ws_count = 0;

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
		}
	}
}

static void
close_client(int fd)
{
	size_t i;

	for (i = 0; i < ws_count; i++) {
		if (wsclients[i].socket != fd)
			continue;

		close(wsclients[i].socket);
		/* compact client list by moving last element into current element */
		wsclients[i] = wsclients[ws_count-1];
		wsclients[ws_count-1].socket = -1;
		ws_count--;
		break;
	}
}

static void
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
}

static void
on_request(struct httpreq *req, struct buf *resp, void *data)
{
	if (req->connection_upgrade) {

		if (ws_count >= MAX_WS_COUNT) {
			buf_appendf(resp, "HTTP/1.1 500 limit\r\n\r\n");
			return;
		}

		upgrade_connection(req, resp);
		return;
	}

	buf_appendf(resp,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: %zu\r\n\r\n"
		"%s\n",
		drawpad_html_size,
		drawpad_html);
}

static struct buf history_ring[1000];
static size_t history_idx = 0;
static size_t history_size = 1000;

static void
history_init()
{
	for (int i = 0; i < history_size; i++) {
		buf_init(&history_ring[i]);
	}
}

static void
history_add(const struct buf *frame)
{
	history_ring[history_idx].len = 0;
	buf_append(&history_ring[history_idx], frame->s, frame->len);
	history_idx++;
	if (history_idx >= history_size)
		history_idx = 0;
}

static void
history_replay(int fd)
{
	for (int i = 0; i < history_size; i++) {
		struct buf *frame = &history_ring[i];
		if (frame->len > 0) {
			send(fd, frame->s, frame->len, 0);
		}
	}
}

static struct buf frame;
static struct buf payload;

static const char *
decode_frame_inplace(char *s, int *len)
{
	char *mask, *data, *p;
	int datalen, i;

	datalen = s[1] & 0x7F;
	mask = &s[2];
	data = &s[6];

	for (i = 0, p = data; i < datalen; i++, p++) {
		*p ^= mask[i % 4];
	}

	*len = datalen;
	return data;
}

static void
process_client_mmove(int fd)
{
	char buf[100];
	const char *data;
	int len;

	ssize_t was_read = read(fd, buf, 100);

	if (was_read == -1) {
		err(1, "cannot read from webclient %d", fd);
	}

	buf[was_read] = 0;

	data = decode_frame_inplace(buf, &len);
	buf_append(&payload, data, len);
	make_frame(&frame, &payload);
	broadcast(&frame);
	history_add(&frame);

	frame.len = 0;
	payload.len = 0;
}

static const char *address = "0.0.0.0";
static struct kevent chlist[10];
static struct kevent evlist[10];
static struct timespec tout = { 30, 0 };

static struct httpd server = {
	.handler = on_request
};

int main(int argc, char **argv)
{
	int port = 44127;

	int i, fd, events_count = 0, changes_count = 1;

	buf_init(&frame);
	buf_init(&payload);
	history_init();

	if (httpd_start(&server, address, port) == -1)
		errx(1, "cannot start server");

	kq = kqueue();
	if (kq == -1)
		err(1, "kqueue");

	EV_SET(&chlist[0], server.listen_socket, EVFILT_READ, EV_ADD, 0, 0, 0);

	for (;;) {
		events_count = kevent(kq, chlist, changes_count, evlist, 10, &tout);

		if (events_count < 0)
			err(1, "kevent");

		if (events_count == 0)
			continue;

		changes_count = 0;

		for (i = 0; i < events_count; i++) {

			if (evlist[i].flags & EV_ERROR)
				errc(1, evlist[i].data, "EV_ERROR");

			if (evlist[i].flags & EV_EOF) {
				if (evlist[i].ident == server.listen_socket)
					err(1, "listening socket closed");

				close_client(evlist[i].ident);
				continue;
			}

			if (evlist[i].ident == server.listen_socket) {
				fd = httpd_accept(&server);
				if (fd > 0) {
					history_replay(fd);
					EV_SET(&chlist[changes_count], fd, EVFILT_READ, EV_ADD, 0, 0, 0);
					changes_count++;
				}
				continue;
			}
			
			process_client_mmove(evlist[i].ident);
		}
	}

	close(kq);

	return 0;
}
