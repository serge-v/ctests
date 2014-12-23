#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <stdlib.h>

struct buf
{
	char *s;
	size_t len;
	size_t cap;
};

static void
buf_append(struct buf *b, const char *s, size_t n)
{
	if (b->s == NULL) {
		b->s = malloc(1024);
	} else 	if (b->len + n > b->cap) {
		b->cap = (b->len + n) * 2;
		b->s = realloc(b->s, b->cap);
	}
	strncat(b->s, s, n);
	b->len += n;
}

struct message
{
	const char *to;
	const char *from;
	const char *subject;
	const char *body;
	struct buf b;       /* composed message after compose() */
};

static int
message_compose(struct message *m)
{
	if (m == NULL) {
		fprintf(stdout, "Message is null\n");
		return 1;
	}

	if (m->to == NULL) {
		fprintf(stdout, "Recipient is not specified\n");
		return 1;
	}

	if (m->from == NULL) {
		fprintf(stdout, "Sender is not specified\n");
		return 1;
	}

	if (m->body == NULL) {
		fprintf(stdout, "Message body is empty\n");
		return 1;
	}

	char s[1024];
	const size_t sz = sizeof(s);

	/* "Date: Mon, 29 Nov 2010 21:54:29 +1100\r\n" */

	time_t now = time(NULL);

	int n = strftime(s, sz, "Date: %a, %d %b %Y %T %z\r\n", gmtime(&now));
	buf_append(&m->b, s, n);

	n = snprintf(s, sz, "To: %s\r\n", m->to);
	buf_append(&m->b, s, n);

	n = snprintf(s, sz, "From: %s\r\n", m->from);
	buf_append(&m->b, s, n);

	if (m->subject != NULL) {
		n = snprintf(s, sz, "Subject: %s\r\n", m->subject);
		buf_append(&m->b, s, n);
	}

	buf_append(&m->b, "\r\n", 2);
	buf_append(&m->b, m->body, strlen(m->body));
	buf_append(&m->b, "\r\n", 2);

	return 0;
}

struct upload_status
{
	const char *text; /* composed message */
	size_t pos;       /* current sending position */
	size_t len;       /* message length */
};

static size_t
payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct upload_status *status = (struct upload_status *)userp;
	const char *data;

	if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1))
		return 0;

	data = &status->text[status->pos];

	size_t write_len = size * nmemb;
	size_t remain_len = status->len - status->pos;

	if (remain_len < write_len)
		write_len = remain_len;

	memcpy(ptr, data, write_len);
	status->pos += write_len;

	return write_len;
}

/* gmail smtp sender */

struct gmail
{
	const char *server;            /* smtp server */
	const char *user;              /* smtp user */
	const char *pin_password;      /* pinentry path to smtp password item */
	int debug;                     /* using local server without credentials */
};

static int
gmail_send(const struct gmail *g, struct message *m)
{
	CURL *curl;
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	struct upload_status status;

	status.pos = 0;
	status.len = m->b.len;
	status.text = m->b.s;

	curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "Cannot init curl\n");
		return 1;
	}

	char pincmd[100];
	snprintf(pincmd, 100, "pass %s", g->pin_password);
	FILE *f = popen(pincmd, "r");
	if (f == NULL) {
		fprintf(stderr, "Cannot read password using %s", pincmd);
		return 1;
	}

	char password[100];
	const size_t pwdlen = 16;

	memset(password, 0, sizeof(password));
	size_t n = fread(password, 1, sizeof(password), f);
	fclose(f);

	if (n == pwdlen+1 && password[pwdlen] == '\n') {
		password[pwdlen] = 0;
		n = pwdlen;
	}

	if (n != 16) {
		fprintf(stderr, "Invalid gmail app password for pin %s\n", g->pin_password);
		return 1;
	}

	if (g->debug) {
		curl_easy_setopt(curl, CURLOPT_URL, "smtp://127.0.0.1:8025");
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	} else {
		curl_easy_setopt(curl, CURLOPT_URL, g->server);
		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
		curl_easy_setopt(curl, CURLOPT_USERNAME, g->user);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
	}

	curl_easy_setopt(curl, CURLOPT_MAIL_FROM, m->from);
	recipients = curl_slist_append(recipients, m->to);
	curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
	curl_easy_setopt(curl, CURLOPT_READDATA, &status);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

	res = curl_easy_perform(curl);

	if (res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));

	curl_slist_free_all(recipients);
	curl_easy_cleanup(curl);

	return (int)res;
}

int send_email_from_me(const char *to, const char *subject, const char *body)
{
	struct gmail g = {
		.server = "smtp://smtp.gmail.com:587",
		.user = "serge0x76@gmail.com",
		.pin_password = "curlsmtp",
		.debug = 0

	};

	struct message m = {
		.from = "serge0x76@gmail.com",
		.to = "voilokov@gmail.com",
		.subject = subject,
		.body = body
	};

	int rc = message_compose(&m);

	if (rc != 0)
		return 1;

	rc = gmail_send(&g, &m);

	return rc;
}

int main(int argc, char **argv)
{
	int rc = send_email_from_me("voilokov@gmail.com", "test", "body");
	printf("sent: %d\n", rc);
}

