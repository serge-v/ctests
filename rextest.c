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
	size_t rc;

	regex_t p1, p2;
	regmatch_t m1, m2;

	rc = regcomp(&p1, "<td[^>]*>", REG_EXTENDED);
	if (rc != 0)
		print_rex_error(rc, &p1);

	rc = regcomp(&p2, "</td>", REG_EXTENDED);
	if (rc != 0)
		print_rex_error(rc, &p2);


	char *s = calloc(100000, 1);
        strcpy(s, "<td nowrap> \nfirst cell \n</td><td nowrap>second cell</td>");

	FILE *f = fopen("1~.html", "rt");
	fread(s, 1, 100000, f);
	fclose(f);

	size_t len = strlen(s);

	m1.rm_so = 0;
	m1.rm_eo = len;

	size_t count = 0;

	for (;;) {

		rc = regexec(&p1, s, 1, &m1, REG_STARTEND);

		if (rc == REG_NOMATCH)
			break;

		if (rc != 0)
			print_rex_error(rc, &p1);

		m2.rm_so = m1.rm_eo;
		m2.rm_eo = len;

		rc = regexec(&p2, s, 1, &m2, REG_STARTEND);

		if (rc == REG_NOMATCH)
			break;

		if (rc != 0)
			print_rex_error(rc, &p2);

		printf("td: [%.*s]\n", (int)(m2.rm_so - m1.rm_eo), &s[m1.rm_eo]);

		m1.rm_so = m2.rm_eo;
		m1.rm_eo = len;
	}

	regfree(&p1);
	regfree(&p2);
}

int main()
{
	test1();
	return 0;
}

