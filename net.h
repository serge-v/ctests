struct message
{
	const char *to;        /* recipient */
	const char *from;      /* sender */
	const char *subject;   /* message subject */
	const char *body;      /* message body */
};

int send_email(const struct message *m);
int fetch_url(const char *url, const char *fname);

