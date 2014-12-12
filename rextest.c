#include <regex.h>
#include <errno.h>
#include <stdio.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <sysexits.h>
#include <curl/curl.h>

static int trace = 1;

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

static int
read_text(const char *fname, char **text, size_t *len)
{
	struct stat st;
	FILE* f = NULL;
	char* buff = NULL;
	int ret = -1;

	if (stat(fname, &st) != 0)
	{
		printf("Cannot stat file %s. Error: %d\n", fname, errno);
		goto out;
	}

	f = fopen(fname, "rt");
	if (!f)
	{
		printf("Cannot open file %s. Error: %d\n", fname, errno);
		goto out;
	}

	buff = malloc(st.st_size + 1);
	if (!buff)
	{
		printf("Cannot allocate %lld bytes. Error: %d\n", st.st_size + 1, errno);
		goto out;
	}

	fread(buff, 1, st.st_size, f);
	fclose(f);
	f = NULL;
	buff[st.st_size] = 0;
	*text = buff;
	*len = st.st_size;
	ret = 0;

out:
	if (f)
		fclose(f);

	return ret;
}

#define TRGROUPS 2

struct trscanner
{
	char		*text;          // text to scan
	int             len;	        // maximum length
	regex_t         preg;           // compiled <td>(.*)</td> regexp
	regmatch_t      matches[2];     // current matches
	char		*sbeg;          // group 1 start
	char		*send;		// group 1 end
	size_t          mlen;           // group 1 length
};

struct departure
{
	char		*time;		// departure time
	char		*destination;   // destination station name
	char		*route;         // train route name
	char		*train;         // train label or number
	char		*platform;      // departure platform
	char		*status;        // status
	SLIST_ENTRY(departure) entries;
};

#define CEND    "\x1b[0m "
#define RED     "\x1b[31m"
#define YELLOW  "\x1b[33m"
#define GREEN   "\x1b[32m"

#define COL_TRAIN	RED "[%s]" CEND
#define COL_DEST	YELLOW "[%s]" CEND
#define COL_PLATFORM	GREEN "[%s]" CEND

static void
departure_dump(struct departure *d)
{
	printf("[%s] " COL_TRAIN COL_DEST COL_PLATFORM "%s\n",
		d->time, d->train, d->destination,
		d->platform, d->status);
}

static void
trscanner_create(struct trscanner *s, char *text, int len)
{
	int rc;

	memset(s, 0, sizeof(struct trscanner));

	rc = regcomp(&s->preg, "<td[^>]*>[\n]*([^\n]*)[\n ]*</td>", REG_EXTENDED | REG_ENHANCED | REG_MINIMAL);
	if (rc != 0)
		print_rex_error(rc, &s->preg);

	s->text = text;
	s->len = len;
	s->matches[0].rm_so = 0;
	s->matches[0].rm_eo = len;
}

static void
trscanner_destroy(struct trscanner *s)
{
	regfree(&s->preg);
	memset(s, 0, sizeof(struct trscanner));
}

static int
trscanner_next(struct trscanner *s)
{
	int rc;

	rc = regexec(&s->preg, s->text, TRGROUPS, s->matches, REG_STARTEND);

	if (rc == REG_NOMATCH)
		return 0;

	if (rc != 0)
		print_rex_error(rc, &s->preg);

	if (s->matches[0].rm_so == -1)
		return 0;

	s->sbeg = &s->text[s->matches[1].rm_so];
	s->send = &s->text[s->matches[1].rm_eo];
	s->mlen = s->matches[1].rm_eo - s->matches[1].rm_so;
	*s->send = 0;

//	printf("    td:%02x,[%s]\n", s->sbeg[0], s->sbeg);

	s->matches[0].rm_so = s->matches[0].rm_eo;
	s->matches[0].rm_eo = s->len;

	return 1;
}

SLIST_HEAD(departures, departure);

static void
parse_tr(char *text, int start, int end, struct departures *list)
{
	struct trscanner scan;
	struct departure *dep;

	trscanner_create(&scan, &text[start], end - start);
	dep = calloc(1, sizeof(struct departure));

	if (!trscanner_next(&scan))
		err(EX_DATAERR, "first table cell doesn't contain a time");

	dep->time = scan.sbeg; // 1

	if (!trscanner_next(&scan))
		err(EX_DATAERR, "second table cell doesn't contain a destination station");

	dep->destination = scan.sbeg; // 2
	if (strstr(dep->destination, "&nbsp;-<i>SEC</i>") != NULL) {
		strcpy(&dep->destination[strlen(dep->destination)-18], " (SEC)");
	}

	if (!trscanner_next(&scan))
		err(EX_DATAERR, "second table cell doesn't contain a destination station");

	dep->platform = scan.sbeg; // 3

	if (!trscanner_next(&scan))
		err(EX_DATAERR, "second table cell doesn't contain a destination station");

	dep->route = scan.sbeg; // 4

	if (!trscanner_next(&scan))
		err(EX_DATAERR, "second table cell doesn't contain a destination station");

	dep->train = scan.sbeg; // 5

	if (!trscanner_next(&scan))
		err(EX_DATAERR, "second table cell doesn't contain a destination station");

	dep->status = scan.sbeg; // 6

	trscanner_destroy(&scan);

	if (trace)
		departure_dump(dep);

	SLIST_INSERT_HEAD(list, dep, entries);
}


static struct departures *
parse_njt_departures(const char *fname)
{
	int rc;
	regex_t preg;
	char *text;
	size_t len;

	struct departures *dlist = calloc(1, sizeof(struct departures));
	SLIST_INIT(dlist);

	rc = read_text("1~.html", &text, &len);
	if (rc != 0)
		err(rc, "cannot read file");

	rc = regcomp(&preg, "<tr (.*)</tr>", REG_EXTENDED | REG_ENHANCED | REG_MINIMAL);
	if (rc != 0)
		print_rex_error(rc, &preg);

	regmatch_t matches[TRGROUPS];

	matches[0].rm_so = 0;
	matches[0].rm_eo = len;

	while (matches[0].rm_so >= 0)
	{
		rc = regexec(&preg, text, TRGROUPS, matches, REG_STARTEND);

		if (rc == REG_NOMATCH)
			break;

		if (rc != 0)
			print_rex_error(rc, &preg);

		if (matches[0].rm_so == -1)
			break;

		parse_tr(text, matches[1].rm_so, matches[1].rm_eo, dlist);

		matches[0].rm_so = matches[0].rm_eo;
		matches[0].rm_eo = len;
	}

	regfree(&preg);

	return dlist;
}

static int
expired(const char *fname)
{
	struct stat st;

	int rc = stat(fname, &st);
	if (rc != 0)
		return 1;

	if (st.st_mtime + 60 < time())
		return 1;

	return 0;
}

static int
fetch_departure_vision(const char *fname)
{
	
}

static void
fetch_departures(const char *station_code)
{
	char fname[MAX_PATH];

	sprintf(fname, "/tmp/njtransit-%s.html", station_code);

	if (expired(fname)) {
		fetch_departure_vision(fname);
	}

	struct departures* deps = parse_njt_departures(fname);
}

int main()
{
	curl_global_init(CURL_GLOBAL_ALL);

	test2();

	curl_global_cleanup();

	return 0;
}

