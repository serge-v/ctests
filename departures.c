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
#include <getopt.h>
#include <inttypes.h>

#include "net.h"
#include "stations.h"

static int debug = 0;                 /* debug parameter */
static int debug_server = 0;          /* make requests to debug local server */
static char *station_from = NULL;     /* departure station */
static char *station_to = NULL;       /* destination station */
static FILE* log = NULL;              /* verbose debug log */
static int email = 0;                 /* send email */

static struct option longopts[] = {
	{ "list",         no_argument,       NULL, 'l' },
	{ "from",         required_argument, NULL, 'f' },
	{ "to",           required_argument, NULL, 't' },
	{ "mail",         no_argument,       NULL, 'm' },
	{ "debug",        no_argument,       NULL, 'd' },
	{ "help",         no_argument,       NULL, 'h' },
	{ "debug-server", no_argument,       NULL, 's' },
	{ NULL,           0,                 NULL,  0  }
};

static void
synopsis()
{
	printf("usage: departures [-ldmh] [-f station] [-t station]\n");
}

static void
usage()
{
	printf(
		"usage: departures [-ldmh] [-f station] [-t station]\n"
		"options:\n"
		"    -l, --list            list stations\n"
		"    -f, --from=STATION    get nearest departure and train status\n"
		"    -t, --to=STATION      set destination station\n"
		"    -m, --mail            send email with nearest departure\n"
		"    -d, --debug           output debug information\n"
		);
}

/* ===== data structures ===================== */

struct departure
{
	char		*time;          // departure time
	char		*destination;   // destination station name
	char		*line;          // rail line name
	char		*train;         // train label or number
	char		*track;         // departure track label or number
	char		*status;        // status
	SLIST_ENTRY(departure) entries; // handler for slist
};

SLIST_HEAD(departures, departure);

struct station
{
	const char *code;               // station code
	const char *name;               // station name
	struct departures* deps;        // list of departures for this station
	SLIST_ENTRY(station) entries;   // handler for slist
};

SLIST_HEAD(stations, station);

struct route
{
	const char *name;              // route name
	struct stations *stations;     // station list for this route
};

/* =========================================== */

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
	FILE *f = NULL;
	char *buff = NULL;
	int ret = -1;

	if (stat(fname, &st) != 0) {
		printf("Cannot stat file %s. Error: %d\n", fname, errno);
		goto out;
	}

	f = fopen(fname, "rt");
	if (f == NULL) {
		printf("Cannot open file %s. Error: %d\n", fname, errno);
		goto out;
	}

	buff = malloc(st.st_size + 1);
	if (buff == NULL) {
		printf("Cannot allocate %" PRId64 " bytes. Error: %d\n", st.st_size + 1, errno);
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
	if (f != NULL)
		fclose(f);

	return ret;
}

struct trscanner
{
	char            *text;          // text to scan
	int             len;	        // maximum length
	regex_t         p1;             // <td>
	regex_t         p2;             // </td>
	regmatch_t      m1;             // current matches
	regmatch_t      m2;             // current matches
	char            *sbeg;          // td inner text start
	char            *send;          // td inner text end
	size_t          mlen;           // td inner text length
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

	if (debug)
		fprintf(log, "extract tds from: %.*s\n", len, text);

	rc = regcomp(&s->p1, "<td[^>]*>", REG_EXTENDED);
	if (rc != 0)
		print_rex_error(rc, &s->p1);

	rc = regcomp(&s->p2, "</td>", REG_EXTENDED);
	if (rc != 0)
		print_rex_error(rc, &s->p2);

	s->text = text;
	s->len = len;
	s->m1.rm_so = 0;
	s->m1.rm_eo = len;
}

static void
trscanner_destroy(struct trscanner *s)
{
	regfree(&s->p1);
	regfree(&s->p2);
	memset(s, 0, sizeof(struct trscanner));
}

static int
trscanner_next(struct trscanner *s)
{
	int rc;

	rc = regexec(&s->p1, s->text, 1, &s->m1, REG_STARTEND);

	if (rc == REG_NOMATCH)
		return 0;

	if (rc != 0)
		print_rex_error(rc, &s->p1);

	s->m2.rm_so = s->m1.rm_eo;
	s->m2.rm_eo = s->len;

	rc = regexec(&s->p2, s->text, 1, &s->m2, REG_STARTEND);

	if (rc == REG_NOMATCH)
		return 0;

	if (rc != 0)
		print_rex_error(rc, &s->p2);

	s->sbeg = &s->text[s->m1.rm_eo];
	s->send = &s->text[s->m2.rm_so];
	s->mlen = s->m2.rm_so - s->m1.rm_eo;
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

	if (debug)
		fprintf(log, "    td: [%s]\n", s->sbeg);

	s->m1.rm_so = s->m2.rm_eo;
	s->m1.rm_eo = s->len;

	return 1;
}

static void
parse_tr(char *text, int len, struct departures *list, struct departure **last_added)
{
	if (strstr(text, "<td colspan=") != NULL)
		return;

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
		errx(1, "Cannot parse track");

	dep->track = scan.sbeg; // 3
	if (strcmp("Single", dep->track) == 0)
		strcpy(dep->track, "1");

	if (!trscanner_next(&scan))
		errx(1, "Cannot parse line");

	dep->line = scan.sbeg; // 4

	if (!trscanner_next(&scan))
		errx(1, "Cannot parse train");

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
	regex_t p1, p2;
	regmatch_t m1, m2;
	char *text;
	size_t len;
	struct departure *last_added = NULL;

	struct departures *dlist = calloc(1, sizeof(struct departures));
	SLIST_INIT(dlist);

	rc = read_text(fname, &text, &len);
	if (rc != 0)
		err(rc, "cannot read file");

	rc = regcomp(&p1, "<tr[^>]*>", REG_EXTENDED);
	if (rc != 0)
		print_rex_error(rc, &p1);

	rc = regcomp(&p2, "</tr>", REG_EXTENDED);
	if (rc != 0)
		print_rex_error(rc, &p2);

	m1.rm_so = 0;
	m1.rm_eo = len;

	for (;;)
	{
		rc = regexec(&p1, text, 1, &m1, REG_STARTEND);

		if (rc == REG_NOMATCH)
			break;

		if (rc != 0)
			print_rex_error(rc, &p1);

		m2.rm_so = m1.rm_eo;
		m2.rm_eo = len;

		rc = regexec(&p2, text, 1, &m2, REG_STARTEND);

		if (rc == REG_NOMATCH)
			break;

		if (rc != 0)
			print_rex_error(rc, &p2);

		parse_tr(&text[m1.rm_eo], m2.rm_so - m1.rm_eo, dlist, &last_added);

		m1.rm_so = m2.rm_eo;
		m1.rm_eo = len;
	}

	regfree(&p1);
	regfree(&p2);

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
	if (debug_server)
		api_url = "http://127.0.0.1:8000/njtransit-%s.html";

	char fname[PATH_MAX];
	char url[100];

	snprintf(fname, PATH_MAX, "/tmp/njtransit-%s.html", station_code);
	snprintf(url, 100, api_url, station_code);

	if (expired(fname)) {
		if (fetch_url(url, fname) != 0)
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
		if (strstr(dep->destination, dest) != NULL)
			return dep;
	}

	return NULL;
}

static void
departures_latest_status(struct departures **dlist, const char **route, size_t n,
			 const char *train, struct buf *b)
{
	struct departure *dep;
	int i, rc;
	char s[1024];
	size_t sz = sizeof(s);
	const char *status = NULL;
	const char *station_code = NULL;

	for (i = 0; i < n; i++) {

		if (dlist[i] == NULL)
			break;

		SLIST_FOREACH(dep, dlist[i], entries) {
			if (dep == NULL)
				break;

			if (strcmp(train, dep->train) != 0)
				continue;

			status = dep->status;
			station_code = route[i];
			rc = snprintf(s, sz, "%-30s(%s): %s\n", station_name(station_code), station_code, status);
			buf_append(b, s, rc);
		}
	}
}

static size_t
route_idx(const char *code, const char *route[], size_t n_route)
{
	size_t i;
	for (i = 0; i < n_route; i++) {
		if (strcmp(code, route[i]) == 0)
			return i;
	}
	return -1;
}

static void
departures_get_upcoming(const char* from, const char *dest,
			  const char *route[], size_t n_route, struct buf *b)
{
	size_t i;
	size_t idx = route_idx(from, route, n_route);
	size_t dest_idx = n_route - 1;

	if (idx == -1) {
		printf("Invalid station code\n");
		return;
	}

	const int MAXPREV = 10; // max previous stations

	struct departures **dlist = calloc(MAXPREV, sizeof(struct departures));

	for (i = 0; i < MAXPREV; i++) {

		if (idx - i + 1 == 0)
			break;

		dlist[i] = departures_fetch(route[idx-i]);

		if (dlist[i] == NULL) {
			fprintf(stderr, "Cannot get departures for station code %s\n", route[idx-i]);
			return;
		}

		if (debug) {
			printf("== Departures for %s(%s) ==\n", station_name(route[idx-i]), route[idx-i]);
			departures_dump(dlist[i]);
			printf("--\n");
		}
	}

	const char *dest_name = station_name(route[dest_idx]);
	const char *from_name = station_name(route[idx]);

	struct departure *nearest = departures_nearest(dlist[0], dest_name);

	int n;
	char s[1024];
	size_t sz = sizeof(s);

	if (nearest != NULL) {
		n = snprintf(s, sz, "Nearest train to %s from %s\n\n", dest_name, from_name);
		buf_append(b, s, n);

		n = snprintf(s, sz, "%s #%s, Track %s %s\n\n",
			nearest->time, nearest->train, nearest->track, nearest->status);
		buf_append(b, s, n);

		departures_latest_status(dlist, route, MAXPREV, nearest->train, b);
	} else {
		n = snprintf(s, sz, "No trains to %s from %s\n", dest_name, from_name);
		buf_append(b, s, n);
	}

	buf_append(b, credits, sz_credits);

	for (i = 0; i < MAXPREV; i++)
		departures_destroy(dlist[i]);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		synopsis();
		return 1;
	}

	int ch;

	while ((ch = getopt_long(argc, argv, "lhds:f:t:m", longopts, NULL)) != -1) {
		switch (ch) {
			case 'd':
				debug = 1;
				log = fopen("/tmp/departures-debug.log", "wt");
				fprintf(log, "==================\n\n\n\n\n\n\n");
				break;
			case 'm':
				email = 1;
				break;
			case 'l':
				stations_list();
				return 0;
			case 'f':
				station_from = optarg;
				break;
			case 't':
				station_to = optarg;
				break;
			case 'h':
				usage();
				return 1;
			default:
				synopsis();
				return 1;
		}
	}

	if (station_from == NULL)
		errx(1, "Origin station is not specified");

	curl_global_init(CURL_GLOBAL_ALL);

	static const char *route[] = { "PO", "OS", "MD", "CB", "CW", "RM", "TC", "XG", "SF", "TS", "HB" };
	const size_t N = sizeof(route) / sizeof(route[0]);

	struct buf b;
	memset(&b, 0, sizeof(struct buf));

	departures_get_upcoming(station_from, station_to, route, N, &b);

	printf("%s", b.s);

	if (email) {
		struct message m = {
			.from = "serge0x76@gmail.com",
			.to = "voilokov@gmail.com",
			.subject = "train to Hoboken",
			.body = b.s,
		};

		int rc = send_email(&m);

		if (rc != 0)
			return 1;
	}

	curl_global_cleanup();
	return 0;
}

