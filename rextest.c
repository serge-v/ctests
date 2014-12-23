#include <ctype.h>
#include <curl/curl.h>
#include <err.h>
#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>

static int debug = 1;

static void
print_rex_error(int errcode, const regex_t *preg)
{
	char buf[1000];
	size_t rc = regerror(errcode, preg, buf, sizeof(buf));
	if (rc <= 0)
		err(1, "cannot get error string for code: %d", errcode);
	err(1, "%s", buf);
}

static void
test1()
{
	int rc;

	regex_t preg;

//	rc = regcomp(&preg, "(http://)([^/]*)/(.*)/", REG_EXTENDED | REG_MINIMAL);
	rc = regcomp(&preg, "<td[^>]*>[\n]*([^\n]*)[\n ]*</td>", REG_EXTENDED | REG_ENHANCED | REG_MINIMAL);
	if (rc != 0)
		print_rex_error(rc, &preg);

	regmatch_t matches[10];

//	const char *s = "http://cnn.com/articles/";
	const char *s = "<td nowrap>\ncr test\n</td>";

	rc = regexec(&preg, s, 10, matches, 0);
	if (rc != 0)
		print_rex_error(rc, &preg);

	printf("rc: %d\n", rc);

	for (int i = 0; i < 10; i++)
	{
		if (matches[i].rm_so == -1)
			break;

		printf("%d: %lld, %lld: [%.*s]\n", i, matches[i].rm_so, matches[i].rm_eo,
		       (int)(matches[i].rm_eo - matches[i].rm_so), &s[matches[i].rm_so]);
	}

	regfree(&preg);
}

int main()
{
	test1();
	return 0;
}
