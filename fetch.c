#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>

struct response
{
	int error;              /* curl error */
	int code;               /* http error code */
	int len;                /* http content length */
	const char *fname;      /* file name to store body */
	char *data;             /* if file name is null allocate http body in memory */
};

static void
response_destroy(struct response* resp)
{
	if (resp->data != NULL)
		free(resp->data);
	memset(&resp, 0, sizeof(struct response));
}

static size_t
header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
	struct response* resp = (struct response*)userdata;
	size_t len = size * nitems;

	if (strncmp("HTTP/1.", buffer, 7) == 0)
		resp->code = atoi(&buffer[9]);

	return len;
}

static size_t
write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct response* resp = (struct response*)userdata;
	int len = size * nmemb;
	resp->len = len;

	if (resp->fname != NULL) {
		FILE* f = fopen(resp->fname, "at");
		if (f == NULL)
			err(1, "Cannot open file %s. Error: %d", resp->fname, errno);

		int rc = fwrite(ptr, 1, len, f);
		if (rc == -1)
			err(1, "Cannot write file %s. Error: %d", resp->fname, errno);

		fclose(f);
	} else {
		resp->data = malloc(len);
		memcpy(resp->data, ptr, len);
	}
	return len;
}

static int
get_url(const char *url, struct response *resp)
{
	CURL *curl;
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
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

void
fetch(const char *url, const char *fname)
{
	struct response resp;

	memset(&resp, 0, sizeof(struct response));
	resp.fname = fname;

	int rc = get_url(url, &resp);

	if (rc != 0) {
		errx(1, "Cannot fetch url %s. Curl error %d : %s", url, resp.error, curl_easy_strerror(resp.error));
	}

	if (resp.code != 200) {
		errx(1, "Cannot fetch url %s. HTTP error %d", url, resp.code);
	}

	response_destroy(&resp);
}
