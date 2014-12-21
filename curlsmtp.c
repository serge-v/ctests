#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

static const char *message_template =

	"Date: Mon, 29 Nov 2010 21:54:29 +1100\r\n"
	"To: %s\r\n"
	"From: %s\r\n"
	"Cc: " CC "(Another example User)\r\n"
	"Subject: SMTP example message\r\n"
	"\r\n"
	"The body of the message starts here.\r\n"
	"\r\n"


struct upload_status
{
	int lines_read;
};

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct upload_status *upload_ctx = (struct upload_status *)userp;
	const char *data;

	if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1))
		return 0;

	data = payload_text[upload_ctx->lines_read];

	if (data)
	{
		size_t len = strlen(data);
		memcpy(ptr, data, len);
		upload_ctx->lines_read++;

		return len;
	}

	return 0;
}

/* gmail smtp sender */

struct gmail
{
	const char *from;              /* sender */
	const char **to;               /* recipients */
	const char *user;              /* smtp user */
	const char *pin_password;      /* pinentry path to smtp password item */
	const char *subject;           /* email subject */
	const char *body;              /* body of the mail in plain text format */
};


int gmail_send(struct mail* m)
{
	CURL *curl;
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	struct upload_status upload_ctx;

	upload_ctx.lines_read = 0;

	curl = curl_easy_init();
	if (!curl)
		return 1;

	curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.gmail.com:587");
	curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
//	curl_easy_setopt(curl, CURLOPT_CAINFO, "./certificate.pem");

	curl_easy_setopt(curl, CURLOPT_MAIL_FROM, m->from);
	curl_easy_setopt(curl, CURLOPT_USERNAME, m->user);

	char pincmd[100];
	snprontf(pincmd, 100, "pass %s", m->pin_password);
	FILE *f = popen(pincmd, "r");
	if (f == NULL) {
		fprintf(stderr, "Cannot read password using %s", pincmd);
		return 1;
	}

	char password[100];
	fread(password, 1, 100, f);
	curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
	fclose(f);

	recipients = curl_slist_append(recipients, TO);
//	recipients = curl_slist_append(recipients, CC);
	curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

	curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
	curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
//	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	res = curl_easy_perform(curl);

	if (res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));

	curl_slist_free_all(recipients);
	curl_easy_cleanup(curl);

	return (int)res;
}

int main()
{
	struct gmail m = {
		.from = "serge0x76@gmail.com",
		.to = "voilokov@gmail.com",
		.subject = "test",
		.body = "test body",

	};
}

