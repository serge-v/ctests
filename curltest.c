/*

for testing start server:
python -m SimpleHTTPServer

*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <curl/curl.h>

static int verbose = 0;

struct response
{
	int error;              // curl error
	int code;               // http error code
	int len;                // http content length
	char *data;             // http body
};

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
	resp->data = malloc(len);
	memcpy(resp->data, ptr, len);
	return len;
}

static int
get_url(const char* url, struct response* resp)
{
	CURL* curl;
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

static void
test_fetch(const char *url)
{
	struct response resp;
	memset(&resp, 0, sizeof(struct response));

	if (get_url(url, &resp) != 0) {
		printf("error %d : %s\n----\n", resp.error, curl_easy_strerror(resp.error));
	} else {
		printf("%s\ncode: %d, length: %d\n----------\n", resp.data, resp.code, resp.len);
	}
}

static void
fetch_url(const char *url, const char *fname)
{
	FILE *f = fopen(fname, "wb");
	if (f == NULL)
		err(1, "cannot open out file %s", fname);

	CURL *curl = curl_easy_init();
	if (curl == NULL)
		errx(1, "cannot init curl");

	char errbuf[CURL_ERROR_SIZE];
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(curl, CURLOPT_URL, url);

 	CURLcode err = curl_easy_perform(curl);
	if (err != CURLE_OK)
		errx(1, "curl error: %s", errbuf);
}


int
main()
{
	const char* vstr = getenv("V");
	if (vstr != NULL) {
		verbose = (strcmp("1", vstr) == 0);
	}

	curl_global_init(CURL_GLOBAL_ALL);
	
	fetch_url("http://weather.noaa.gov/pub/data/raw/fz/fznt01.kwbc.hsf.at1.txt", "/tmp/forecast.txt");

	if (0) {
		test_fetch("http://localhost:8000/curltest.c");
		test_fetch("http://localhost:8000/notfound.txt");
		test_fetch("http://localhost:8001/badport.txt");
		test_fetch("http://badaddress:8001/badaddress.txt");
		test_fetch("badprot://badaddress/badaddress.txt");
	}

	curl_global_cleanup();

	return 0;
}

