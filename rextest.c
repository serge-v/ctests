#include <regex.h>
#include <stdio.h>
#include <err.h>

/*
int regcomp(regex_t *restrict preg, const char *restrict pattern, int cflags);
size_t regerror(int errcode, const regex_t *preg, char *errbuf, errbuf_size);
int regexec(const regex_t *restrict preg, const char *string, size_t nmatch, regmatch_t pmatch, int eflags);
void regfree(regex_t *preg);
*/

static void
print_rex_error(int errcode, const regex_t *preg) {
	char buf[1000];
	size_t rc = regerror(errcode, preg, buf, sizeof(buf));
	if (rc <= 0) {
		err(1, "cannot get error string for code: %d", errcode);
	}
	err(1, "%s", buf);
}

static void
test1()
{
	int rc;
	
	regex_t preg;
	
	rc = regcomp(&preg, "\\(http://\\)\\([^/]*\\)/\\(.*\\)/", 0);
	if (rc != 0) {
		print_rex_error(rc, &preg);
	}

	regmatch_t matches[10];

	const char *s = "http://cnn.com/articles/";
	
	rc = regexec(&preg, s, 10, matches, 0);
	if (rc != 0) {
		print_rex_error(rc, &preg);
	}

	printf("rc: %d\n", rc);

	for (int i = 0; i < 10; i++) {
		if (matches[i].rm_so == -1)
			break;

		printf("%d: %lld, %lld: %.*s\n", i, matches[i].rm_so, matches[i].rm_eo,
			(int)(matches[i].rm_eo - matches[i].rm_so), &s[matches[i].rm_so]);
	}

	regfree(&preg);
}

int main()
{
	test1();
	return 0;
}

