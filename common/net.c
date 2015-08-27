#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>

#include "net.h"
#include "struct.h"

/* ========= send_email using curl ========= */

static int
message_compose(const struct message *m, struct buf *b)
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
	buf_append(b, s, n);

	n = snprintf(s, sz, "To: %s\r\n", m->to);
	buf_append(b, s, n);

	n = snprintf(s, sz, "From: %s\r\n", m->from);
	buf_append(b, s, n);

	buf_appendf(b, "Content-Type: multipart/mixed; boundary=frontier\r\n");
	buf_appendf(b, "MIME-Version: 1.0\r\n");

	if (m->subject != NULL) {
		n = snprintf(s, sz, "Subject: %s\r\n", m->subject);
		buf_append(b, s, n);
	}

	buf_append(b, "\r\n", 2);
	buf_appendf(b, "--frontier\r\n");
	buf_appendf(b, "Content-Type: text/html; charset=\"us-ascii\"\r\n");
	buf_appendf(b, "MIME-Version: 1.0\r\n");
	buf_appendf(b, "Content-Transfer-Encoding: 7bit\r\n");
	buf_append(b, "\r\n", 2);
	buf_append(b, m->body, strlen(m->body));
	buf_appendf(b, "--frontier--\r\n");
	buf_append(b, "\r\n", 2);

	return 0;
}

struct upload_status
{
	const char *text; /* composed message */
	size_t pos;       /* current sending position */
	size_t len;       /* message length */
};

static size_t
read_data(void *ptr, size_t size, size_t nmemb, void *userp)
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

struct smtp_info
{
	const char *server;            /* smtp server */
	const char *user;              /* smtp user */
	const char *password_file;     /* file with password */
	int debug;                     /* using local server without credentials */
};

static int
curl_smtp_send(const struct smtp_info *s, const struct message *m)
{
	CURL *curl;
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	struct upload_status status;
	struct buf b;

	memset(&b, 0, sizeof(struct buf));

	int rc = message_compose(m, &b);
	if (rc != 0) {
		fprintf(stderr, "Cannot compose message\n");
		return 1;
	}

	status.pos = 0;
	status.len = b.len;
	status.text = b.s;

	curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "Cannot init curl\n");
		return 1;
	}

	FILE *f = fopen(s->password_file, "rt");
	if (f == NULL) {
		fprintf(stderr, "Cannot read password from %s\n", s->password_file);
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
		fprintf(stderr, "Invalid gmail app password for pin %s\n", s->password_file);
		return 1;
	}

	if (s->debug) {
		curl_easy_setopt(curl, CURLOPT_URL, "smtp://127.0.0.1:8025");
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	} else {
		curl_easy_setopt(curl, CURLOPT_URL, s->server);
		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
		curl_easy_setopt(curl, CURLOPT_USERNAME, s->user);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
	}

	curl_easy_setopt(curl, CURLOPT_MAIL_FROM, m->from);
	recipients = curl_slist_append(recipients, m->to);
	curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_data);
	curl_easy_setopt(curl, CURLOPT_READDATA, &status);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

	res = curl_easy_perform(curl);

	if (res != CURLE_OK)
		fprintf(stderr, "Cannot send email. Error: %s\n",
			curl_easy_strerror(res));

	curl_slist_free_all(recipients);
	curl_easy_cleanup(curl);

	return (int)res;
}

int
send_email(const struct message *m, const char *password_file)
{
	struct smtp_info gmail = {
		.server = "smtp://smtp.gmail.com:587",
		.user = "serge0x76@gmail.com",
		.password_file = password_file,
		.debug = 0
	};

	return curl_smtp_send(&gmail, m);
}

/* ========= fetch_url using curl ========= */

struct response
{
	int error;              /* curl error */
	int code;               /* http error code */
	size_t len;             /* http content length */
	size_t recv;            /* currently received length */
	const char *fname;      /* file name to store body */
	FILE *f;                /* file handler to store body */
	char *data;             /* if file name is null allocate http body in memory */
	char *p;                /* pointer after current chunk */
};

static void
response_destroy(struct response* resp)
{
	if (resp->data != NULL)
		free(resp->data);
	memset(resp, 0, sizeof(struct response));
}

static size_t
header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
	struct response* resp = (struct response*)userdata;
	size_t len = size * nitems;

	if (strncmp("HTTP/1.", buffer, 7) == 0) {
		resp->code = atoi(&buffer[9]);
		return len;
	}

	if (strncmp("Content-Length: ", buffer, 16) == 0) {
		resp->len = atoi(&buffer[16]);

		if (resp->len == 0)
			return len;

		if (resp->fname != NULL) {
			resp->f = fopen(resp->fname, "wt");
			if (resp->f == NULL)
					err(1, "Cannot open file %s. Error: %d", resp->fname, errno);
			return len;
		}

		resp->data = malloc(resp->len);
		resp->p = resp->data;
		return len;
	}

	return len;
}

static size_t
write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct response* resp = (struct response*)userdata;
	size_t len = size * nmemb;

	resp->recv += len;

	if (resp->recv > resp->len)
		err(1, "Received %lu bytes that is more than expected %lu",
		    resp->recv, resp->len);

	if (resp->fname != NULL) {

		int rc = fwrite(ptr, 1, len, resp->f);

		if (rc == -1 || rc != len)
			err(1, "Cannot write file %s. Error: %d", resp->fname, errno);

		if (resp->recv == resp->len) {
			fclose(resp->f);
			resp->f = NULL;
		}

		return len;
	}

	memcpy(resp->p, ptr, len);
	resp->p += len;

	return len;
}

static int
curl_get(const char *url, struct response *resp)
{
	CURL *curl;
	struct curl_slist *chunk = NULL;

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);

	chunk = curl_slist_append(chunk,
	    "Accept: "
	    "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");

	chunk = curl_slist_append(chunk,
	    "User-Agent: "
	    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9_5) "
	    "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.95");

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, resp);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp);
	CURLcode err = curl_easy_perform(curl);
	resp->error = err;

	curl_slist_free_all(chunk);
	curl_easy_cleanup(curl);

	return err;
}

int
fetch_url(const char *url, const char *fname)
{
	int rc;
	struct response resp;

	memset(&resp, 0, sizeof(struct response));
	resp.fname = fname;

	rc = curl_get(url, &resp);

	if (rc != 0) {
		fprintf(stderr, "Cannot fetch url %s. Curl error %d : %s\n",
			url, resp.error, curl_easy_strerror(resp.error));
		goto out;
	}

	if (resp.code != 200) {
		fprintf(stderr, "Cannot fetch url %s. HTTP error %d\n", url, resp.code);
		rc = resp.code;
		goto out;
	}
out:
	response_destroy(&resp);
	return rc;
}

static int
curl_post(const char *url, const char *post_data, struct response *resp)
{
	CURL *curl;
	struct curl_slist *chunk = NULL;

	curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, url);

	chunk = curl_slist_append(chunk,
	    "Content-Type: text/xml,charset=utf-8");

	chunk = curl_slist_append(chunk,
	    "SOAPAction: \"http://www.altoromutual.com/bank/ws/GetUserAccounts\"");

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, resp);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp);
	CURLcode err = curl_easy_perform(curl);
	resp->error = err;

	curl_easy_cleanup(curl);

	return err;
}

int
post_url(const char *url, const char *post_data, const char *fname)
{
	int rc;
	struct response resp;

	memset(&resp, 0, sizeof(struct response));
	resp.fname = fname;

	rc = curl_post(url, post_data, &resp);

	if (rc != 0) {
		fprintf(stderr, "Cannot post to url %s. Curl error %d : %s\n",
			url, resp.error, curl_easy_strerror(resp.error));
		goto out;
	}

	if (resp.code != 200) {
		fprintf(stderr, "Cannot post to url %s. HTTP error %d\n", url, resp.code);
		rc = resp.code;
		goto out;
	}
out:
	response_destroy(&resp);
	return rc;
}


