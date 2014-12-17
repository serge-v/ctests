#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>

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

	if (resp->f != NULL) {

		int rc = fwrite(ptr, 1, len, resp->f);

		if (rc == -1 || rc != len)
			err(1, "Cannot write file %s. Error: %d", resp->fname, errno);

		return len;
	}

	memcpy(resp->p, ptr, len);
	resp->p += len;

	return len;
}

static int
get_url(const char *url, struct response *resp)
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
fetch(const char *url, const char *fname)
{
	int rc;
	struct response resp;

	memset(&resp, 0, sizeof(struct response));
	resp.fname = fname;

	rc = get_url(url, &resp);

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
