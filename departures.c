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

#include "fetch.h"

static int debug = 0;

static void
print_rex_error(int errcode, const regex_t *preg)
{
	char buf[1000];
	size_t rc = regerror(errcode, preg, buf, sizeof(buf));
	if (rc <= 0)
		err(1, "cannot get error string for code: %d", errcode);
	err(1, "%s", buf);
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
	char		*line;          // rail line name
	char		*train;         // train label or number
	char		*track;         // departure track label or number
	char		*status;        // status
	SLIST_ENTRY(departure) entries;
};

#define CEND    "\x1b[0m "
#define BLACK   "\x1b[30m"
#define RED     "\x1b[31m"
#define YELLOW  "\x1b[33m"
#define GREEN   "\x1b[32m"

#define COL_TIME        BLACK  "%7s"   CEND
#define COL_TRAIN       RED    "%5s"   CEND
#define COL_DEST        YELLOW "%-20s" CEND
#define COL_TRACK       GREEN  "%3s"   CEND

static void
departure_dump(struct departure *d)
{
	printf(COL_TIME COL_TRAIN COL_DEST COL_TRACK "%s\n",
		d->time, d->train, d->destination,
		d->track, d->status);
}

static void
trscanner_create(struct trscanner *s, char *text, int len)
{
	int rc;

	memset(s, 0, sizeof(struct trscanner));

	rc = regcomp(&s->preg, "<td[^>]*>(.*)</td>", REG_EXTENDED | REG_ENHANCED | REG_MINIMAL);
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

	/* skip space at start */

	while (isspace(*s->sbeg)) {
		s->sbeg++;
		s->mlen--;
	}

	/* trim the end */

	char *p = strchr(s->sbeg, '<');
	if (p == NULL)
		p = s->send;

	while (isspace(*p) || *p == '<') {
		*p = 0;
		s->mlen--;
		p--;
	}

//	printf("    td:%02x,[%s]\n", s->sbeg[0], s->sbeg);

	s->matches[0].rm_so = s->matches[0].rm_eo;
	s->matches[0].rm_eo = s->len;

	return 1;
}

SLIST_HEAD(departures, departure);

static void
parse_tr(char *text, int len, struct departures *list, struct departure **last_added)
{
	struct trscanner scan;
	struct departure *dep;

	trscanner_create(&scan, text, len);
	dep = calloc(1, sizeof(struct departure));

	if (!trscanner_next(&scan))
		err(1, "first table cell doesn't contain a time");

	dep->time = scan.sbeg; // 1

	if (!trscanner_next(&scan))
		errx(1, "second table cell doesn't contain a destination station");

	dep->destination = scan.sbeg; // 2
	if (strstr(dep->destination, "&nbsp;-") != NULL) {
		strcpy(&dep->destination[strlen(dep->destination)-7], " (SEC)");
	}

	if (!trscanner_next(&scan))
		errx(1, "no track found");

	dep->track = scan.sbeg; // 3
	if (strcmp("Single", dep->track) == 0)
		strcpy(dep->track, "1");

	if (!trscanner_next(&scan))
		errx(1, "no line found");

	dep->line = scan.sbeg; // 4

	if (!trscanner_next(&scan))
		errx(1, "no train found");

	dep->train = scan.sbeg; // 5

	if (trscanner_next(&scan))
		dep->status = scan.sbeg; // 6

	trscanner_destroy(&scan);

	if (*last_added == NULL)
		SLIST_INSERT_HEAD(list, dep, entries);
	else
		SLIST_INSERT_AFTER(*last_added, dep, entries);

	*last_added = dep;
}


static struct departures *
departures_parse(const char *fname)
{
	int rc;
	regex_t preg;
	char *text;
	size_t len;
	struct departure *last_added = NULL;

	struct departures *dlist = calloc(1, sizeof(struct departures));
	SLIST_INIT(dlist);

	rc = read_text(fname, &text, &len);
	if (rc != 0)
		err(rc, "cannot read file");

	rc = regcomp(&preg, "<tr[ *](.*)</tr>", REG_EXTENDED | REG_ENHANCED | REG_MINIMAL);
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

		parse_tr(&text[matches[1].rm_so], matches[1].rm_eo - matches[1].rm_so, dlist, &last_added);

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
	if (rc != 0) {
		if (debug)
			printf("%s doesn't exist\n", fname);
		return 1;
	}

	if (st.st_mtime + 60 < time(NULL)) {
		if (debug)
			printf("%s expired\n", fname);
		return 1;
	}

	return 0;
}

static struct departures*
departures_fetch(const char *station_code)
{
	const char *api_url = "http://dv.njtransit.com/mobile/tid-mobile.aspx?SID=%s&SORT=A";
	if (debug)
		api_url = "http://127.0.0.1:8000/njtransit-%s.html";

	char fname[PATH_MAX];
	char url[100];

	snprintf(fname, PATH_MAX, "/tmp/njtransit-%s.html", station_code);
	snprintf(url, 100, api_url, station_code);

	if (expired(fname)) {
		if (fetch(url, fname) != 0)
			return NULL;

		if (debug)
			printf("%s fetched\n", fname);
	}

	struct departures* deps = departures_parse(fname);

	return deps;
}

static void
departures_dump(struct departures *deps)
{
	struct departure* dep;

	SLIST_FOREACH(dep, deps, entries) {
		departure_dump(dep);
	}
}

static void
departures_destroy(struct departures *deps)
{
	if (deps == NULL)
		return;

	struct departure *dep;

	while (!SLIST_EMPTY(deps)) {
		dep = SLIST_FIRST(deps);
		SLIST_REMOVE_HEAD(deps, entries);
		free(dep);
	}
}

static struct departure *
departures_nearest(struct departures *deps, const char *dest)
{
	struct departure *dep;

	SLIST_FOREACH(dep, deps, entries) {
		if (strcmp(dest, dep->destination) == 0)
			return dep;
	}

	return NULL;
}

static void
departures_latest_status(struct departures **dlist, const char **stations, size_t n, const char *train)
{
	struct departure *dep;
	int i;
	const char *status = NULL;
	const char *station = NULL;

	for (i = 0; i < n; i++) {
		SLIST_FOREACH(dep, dlist[i], entries) {
			if (strcmp(train, dep->train) != 0)
				continue;

			if (strncmp("in ", dep->status, 3) == 0) {
				status = dep->status;
				station = stations[i];
				printf("%s at %s\n", status, station);
			}
		}
	}
}

static void
departures_print_upcoming()
{
	const char *stations[] = {"Sloatsburg", "Tuxedo", "Harriman" };
	const char *route[] = { "XG", "TC", "RM" };
	size_t i;

	struct departures* dlist[3];
	memset(dlist, 0, 3);

	for (i = 0; i < 3; i++) {

		dlist[i] = departures_fetch(route[i]);

		if (dlist[i] == NULL) {
			fprintf(stderr, "Cannot get departures for station code %s\n", route[i]);
			continue;
		}

		if (debug)
			departures_dump(dlist[i]);
	}

	const char *dest = "Hoboken (SEC)";
	struct departure *nearest = departures_nearest(dlist[0], dest);

	if (nearest != NULL) {
		printf("Nearest train to %s from %s\n", dest, route[0]);
		printf("%s #%s, Track %s %s\n",
		       nearest->time, nearest->train, nearest->track, nearest->status);
		departures_latest_status(dlist, stations, 3, nearest->train);
	} else {
		printf("No trains to %s from %s\n", dest, route[0]);
	}

	for (i = 0; i < 3; i++)
		departures_destroy(dlist[i]);
}

int main()
{
	curl_global_init(CURL_GLOBAL_ALL);
	departures_print_upcoming();
	curl_global_cleanup();
	return 0;
}
