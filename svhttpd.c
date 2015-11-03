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
		sent = send(wsclients[i].socket, frame->s, frame->len, 0);
		if (sent == -1)
			warn("send to %d\n", wsclients[i].socket);
		else
			printf("sent to %d: %zd\n", wsclients[i].socket, sent);
	}
}

struct calcpars
{
	int row, prev_row, prev_x, prev_y, dt;
	uint64_t timestamp, count, prev_count, prev_time, start_time;
	struct buf frame, payload;
	char s[1024];
};

static void
cpars_init(struct calcpars *p)
{
	memset(p, 0, sizeof(struct calcpars));
	p->prev_row = -1;
	buf_init(&p->frame);
	buf_init(&p->payload);
}

static void
cpars_read_history_record(struct calcpars *p, FILE *f)
{
	fgets(p->s, 1024, f);
	int n = sscanf(p->s, "%llu\t%llu\n", &p->timestamp, &p->count);
	if (n != 2)
		errx(1, "wrong line: %s", p->s);
}

static void
cpars_process_record(struct calcpars *p, int websocket)
{
	int x, y, dt;
	uint64_t diff_x, diff_y;

	if (p->prev_count == 0) {
		p->prev_count = p->count;
		p->start_time = p->timestamp;
		p->prev_time = p->timestamp;
		return;
	}

	dt = p->timestamp - p->prev_time;
	if (dt <= 0)
		return;

	diff_x = (p->timestamp - p->start_time)*20;
	diff_y = (p->count - p->prev_count);

	x = diff_x % 1000;
	p->row = diff_x / 1000;
	y = p->row * 50 + diff_y / dt;

	if (p->row != p->prev_row) {
		buf_appendf(&p->payload, "c=gray\nl=0,%d,1000,%d\n", p->row*50, p->row*50);
		p->prev_x = 0;
		p->prev_y += 50;
	}

	buf_appendf(&p->payload, "c=darkgreen\nl=%d,%d,%d,%d", p->prev_x, p->prev_y, x, y);
	make_frame(&p->frame, &p->payload);

	if (websocket == -1)
		broadcast(&p->frame);
	else
		send(websocket, p->frame.s, p->frame.len, 0);

	p->frame.len = 0;
	p->payload.len = 0;
	p->prev_count = p->count;
	p->prev_time = p->timestamp;
	p->prev_row = p->row;
	p->prev_x = x;
	p->prev_y = y;
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
send_history(int websocket)
{
	struct calcpars cpars;

	cpars_init(&cpars);

	FILE *f = fopen(events_fname, "rt");
	if (f == NULL)
		err(1, "events~.txt");

	while (!feof(f)) {
		cpars_read_history_record(&cpars, f);
		cpars_process_record(&cpars, websocket);
	}

	send_frame("c=red", websocket);
}

static struct calcpars realtime_cpars;

static void
cpars_read_realtime_record(struct calcpars *p, ssize_t len)
{
	ssize_t sz = read(fd, p->s, len);
	if (sz == -1)
		err(1, "cpars_read_realtime_record");

	int n = sscanf(p->s, "%llu\t%llu\n", &p->timestamp, &p->count);
	if (n != 2)
		errx(1, "wrong line: %s", p->s);
}

static void
send_last(ssize_t len)
{
	cpars_read_realtime_record(&realtime_cpars, len);
	cpars_process_record(&realtime_cpars, -1);
}

static int
upgrade_connection(struct httpreq *req, struct buf *resp)
{
	char *sec_websocket_accept = create_accept_key(req->sec_websocket_key);

	buf_appendf(resp, "HTTP/1.1 101 Switching Protocols\r\n");
	buf_appendf(resp, "Connection: Upgrade\r\n");
	buf_appendf(resp, "Upgrade: websocket\r\n");
	buf_appendf(resp, "Sec-WebSocket-Accept: %s\r\n\r\n", sec_websocket_accept);

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
		    "l=0,495,0,505\n"
		    "l=-5,500,5,500\n"
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
		send_history(websocket);
	}

	buf_appendf(resp, "HTTP/1.1 200 OK\r\n");
	buf_appendf(resp, "Content-Type: text/plain\r\n");
	buf_appendf(resp, "Content-Length: 5\r\n\r\n");
	buf_appendf(resp, "test\n");
}

int main()
{
	const int port = 44126;
	const char *address = "0.0.0.0";
	int i, nev;
	off_t off;
	struct httpd server;
	struct timespec tout = { 2, 0 };
	struct kevent chlist[2];
	struct kevent evlist[2];

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

	int count = 0;

	for (;;) {
		nev = kevent(kq, chlist, 2, evlist, 2, &tout);
		if (nev < 0)
			err(1, "kevent");

		if (nev == 0) {
			buf_clean(&payload);
			buf_appendf(&payload, "p=%d,20", (count++)*4);
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
