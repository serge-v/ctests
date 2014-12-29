struct buf
{
	char *s;
	size_t len;
	size_t cap;
};

void buf_append(struct buf *b, const char *s, size_t n);

struct message
{
	const char *to;        /* recipient */
	const char *from;      /* sender */
	const char *subject;   /* message subject */
	const char *body;      /* message body */
};

int send_email(const struct message *m);
int fetch_url(const char *url, const char *fname);
int post_url(const char *url, const char *post_data, const char *fname);

