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

#include "../common/net.h"
#include "stations.h"
#include "version.h"

static int debug = 0;                 /* debug parameter */
static int debug_server = 0;          /* make requests to debug local server */
static char *station_from = NULL;     /* departure station */
static char *station_to = NULL;       /* destination station */
static FILE *log = NULL;              /* verbose debug log */
static int email = 0;                 /* send email */
static int all = 0;                   /* show all trains for station */
static char *train = NULL;            /* train code */

static struct option longopts[] = {
	{ "list",         no_argument,       NULL, 'l' },
	{ "from",         required_argument, NULL, 'f' },
	{ "to",           required_argument, NULL, 't' },
	{ "mail",         no_argument,       NULL, 'm' },
	{ "all",          no_argument,       NULL, 'a' },
	{ "stops",        no_argument,       NULL, 'p' },
	{ "debug",        no_argument,       NULL, 'd' },
	{ "help",         no_argument,       NULL, 'h' },
	{ "debug-server", no_argument,       NULL, 's' },
	{ "version",      no_argument,       NULL, 'v' },
	{ NULL,           0,                 NULL,  0  }
};

static void
synopsis()
{
	printf("usage: departures [-ldmhvap] [-f station] [-t station] [-p train]\n");
}

static void
usage()
{
	synopsis();

	printf(
		"options:\n"
		"    -l, --list            list stations\n"
		"    -f, --from=station    get next departure and train status\n"
		"    -t, --to=station      set destination station\n"
		"    -a, --all             get all departures for station\n"
		"    -p, --stops=train     get stops for train\n"
		"    -m, --mail            send email with nearest departure\n"
		"    -d, --debug           output debug information\n"
		"    -s, --debug-server    use debug server\n"
		"    -v, --version         print version\n"
		);
}

/* ===== data structures ===================== */

struct departure
{
	char            *time;          /* departure time */
	char            *destination;   /* destination station name */
	char            *line;          /* rail line name */
	char            *train;         /* train label or number */
	char            *track;         /* departure track label or number */
	char            *status;        /* train status */
	const char      *code;          /* station code */
	SLIST_ENTRY(departure) entries; /* handler for slist */
};

SLIST_HEAD(departure_list, departure);

struct departures
{
	struct departure_list *list;   /* departures list */
	size_t size;                   /* departures list size */
};

struct station
{
	char *code;                     /* station code */
	char *name;                     /* station name */
	struct departures* deps;        /* list of departures for this station */
	SLIST_ENTRY(station) entries;   /* handler for slist */
};

SLIST_HEAD(station_list, station);

struct route
{
	const char *name;                  /* route name */
	struct station_list *stations;     /* station list for this route */
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
	printf(COL_TIME COL_TRAIN "%s " COL_DEST COL_TRACK "%s\n",
		d->time, d->train, d->code, d->destination,
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
parse_tr(char *text, int len, struct departures *deps, struct departure **last_added)
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

	if (strncmp("DEP", dep->time, 3) == 0)
		return;

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

	dep->code = station_code(dep->destination);

	if (*last_added == NULL)
		SLIST_INSERT_HEAD(deps->list, dep, entries);
	else
		SLIST_INSERT_AFTER(*last_added, dep, entries);

	deps->size++;
	*last_added = dep;
}

static void
station_load(struct station* st, const char *fname)
{
	int rc;
	regex_t p1, p2;
	regmatch_t m1, m2;
	char *text;
	size_t len;
	struct departure *last_added = NULL;

	st->deps = calloc(1, sizeof(struct departures));
	if (st->deps == NULL)
		err(1, "Cannot allocate deps");

	st->deps->list = calloc(1, sizeof(struct departure_list));
	SLIST_INIT(st->deps->list);

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

		parse_tr(&text[m1.rm_eo], m2.rm_so - m1.rm_eo, st->deps, &last_added);

		m1.rm_so = m2.rm_eo;
		m1.rm_eo = len;
	}

	regfree(&p1);
	regfree(&p2);
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

static struct station*
station_create(const char *station_code)
{
	const char *api_url = "http://dv.njtransit.com/mobile/tid-mobile.aspx?SID=%s&SORT=A";
	if (debug_server)
		api_url = "http://127.0.0.1:8000/njtransit-%s.html";

	char fname[PATH_MAX];
	char url[100];

	struct station* st = calloc(1, sizeof(struct station));
	if (st == NULL)
		err(1, "Cannot allocate station");

	st->code = strdup(station_code);
	st->name = strdup(station_name(station_code));
	snprintf(fname, PATH_MAX, "/tmp/njtransit-%s.html", st->code);
	snprintf(url, 100, api_url, st->code);

	if (expired(fname)) {
		if (fetch_url(url, fname) != 0)
			err(1, "Cannot fetch departures for station");

		if (debug)
			printf("%s fetched\n", fname);
	}

	station_load(st, fname);

	return st;
}

static void
station_dump(struct station *s)
{
	printf("=== %s(%s) === [%zu] =====================\n",
		s->name, s->code, s->deps->size - 1);

	struct departure header = {
		.time = "DEP",
		.train = "TRAIN",
		.code = "SC",
		.destination = "TO",
		.track = "TRK",
		.status = "STATUS",
	};

	departure_dump(&header);

	struct departure* dep;

	SLIST_FOREACH(dep, s->deps->list, entries) {
		departure_dump(dep);
	}
}

static void
station_destroy(struct station *s)
{
	if (s == NULL)
		return;

	struct departure *dep;

	while (!SLIST_EMPTY(s->deps->list)) {
		dep = SLIST_FIRST(s->deps->list);
		SLIST_REMOVE_HEAD(s->deps->list, entries);
		free(dep);
	}

	free(s->deps);
	free(s);
}

static struct departure *
departures_nearest(struct departures *deps, const char *dest_code)
{
	struct departure *dep;

	SLIST_FOREACH(dep, deps->list, entries) {
		if (strcmp(dep->code, dest_code) == 0)
			return dep;
	}

	return NULL;
}

static void
stations_latest_status(struct station **stlist, const char **route, size_t n,
			 size_t curr_idx, const char *train, struct buf *b)
{
	struct departure *dep;
	int i, rc;
	char s[1024];
	size_t sz = sizeof(s);
	const char *status = NULL;
	const char *station_code = NULL;

	for (i = 0; i < n; i++) {

		if (stlist[i] == NULL)
			break;

		SLIST_FOREACH(dep, stlist[i]->deps->list, entries) {
			if (dep == NULL)
				break;

			if (strcmp(train, dep->train) != 0)
				continue;

			status = dep->status;
			station_code = route[curr_idx-i];
			rc = snprintf(s, sz, "    %s(%s): %s\n", station_name(station_code), station_code, status);
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

static int
compare(const void *v1, const void *v2)
{
	const char *s1 = v1;
	const char *s2 = v2;
	return strcmp(s1, s2);
}

static const char *
propose_destinations(struct station *st, const char *to)
{
	to = station_verify_code(to);
	if (to != NULL)
		return to;

	const size_t sz = st->deps->size;
	const char **codes = calloc(sz, sizeof(char*));

	size_t i = 0;
	struct departure* dep;
	SLIST_FOREACH(dep, st->deps->list, entries) {
		codes[i++] = dep->code;
	}

	mergesort(codes, sz, sizeof(char*), compare);

	size_t uniq_count = 0;
	const char *prev = NULL;
	for (i = 0; i < sz; i++) {
		if (prev == NULL || strcmp(prev, codes[i]) != 0) {
			uniq_count++;
			prev = codes[i];
		}
	}

	if (uniq_count == 1)
		return codes[0];

	printf("Multiple destinantions found.\nUse -t parameter and station code from the list:\n");
	prev = NULL;
	for (i = 0; i < sz; i++) {
		if (prev == NULL || strcmp(prev, codes[i]) != 0) {
			printf("%-20s %s\n", station_name(codes[i]), codes[i]);
			prev = codes[i];
		}
	}
	return NULL;
}

static void
parse_par(char *text, size_t len, char **stop_name, char **stop_status)
{
	regex_t p1, p2;
	regmatch_t m1, m2;

	int rc = regcomp(&p1, "<p[^>]*>", REG_EXTENDED);
	if (rc != 0)
		print_rex_error(rc, &p1);

	rc = regcomp(&p2, "</p>", REG_EXTENDED);
	if (rc != 0)
		print_rex_error(rc, &p2);

	m1.rm_so = 0;
	m1.rm_eo = len;

	rc = regexec(&p1, text, 1, &m1, REG_STARTEND);

	if (rc == REG_NOMATCH)
		return;

	if (rc != 0)
		print_rex_error(rc, &p1);

	m2.rm_so = m1.rm_eo;
	m2.rm_eo = len;

	rc = regexec(&p2, text, 1, &m2, REG_STARTEND);

	if (rc == REG_NOMATCH)
		return;

	if (rc != 0)
		print_rex_error(rc, &p2);


	size_t plen = m2.rm_so - m1.rm_eo;
	char* ptext = &text[m1.rm_eo];
	if (debug)
		fprintf(log, "  p raw: %.*s\n", (int)plen, ptext);
	char *p = strstr(ptext, "&nbsp;&nbsp;");
	if (p != NULL)
		memset(p, 0, 12);

	*stop_name = ptext;
	*stop_status = p + 12;
	text[m2.rm_so] = 0;

	if (debug)
		fprintf(log, "stop_name: %s, stop_status: %s\n", *stop_name, *stop_status);
}

struct stop
{
	char *name;
	const char *code;
	char *status;
	SLIST_ENTRY(stop) entries;
};

SLIST_HEAD(stop_list, stop);

static void
parse_train_stops(const char *fname, struct stop_list* list)
{
	int rc;
	regex_t p1, p2;
	regmatch_t m1, m2;
	char *text;
	size_t len;

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

	struct stop *last = NULL;

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


		size_t tdlen = m2.rm_so - m1.rm_eo;
		if (debug)
			fprintf(log, "tr: %.*s\n", (int)tdlen, &text[m1.rm_eo]);

		char *name = NULL;
		char *status = NULL;

		parse_par(&text[m1.rm_eo], tdlen, &name, &status);

		if (name != NULL) {
			struct stop *stop = calloc(1, sizeof(struct stop));

			stop->name = name;
			stop->code = station_code(stop->name);
			stop->status = status;

			if (SLIST_EMPTY(list))
				SLIST_INSERT_HEAD(list, stop, entries);
			else
				SLIST_INSERT_AFTER(last, stop, entries);

			last = stop;
		}

		m1.rm_so = m2.rm_eo;
		m1.rm_eo = len;
	}

	regfree(&p1);
	regfree(&p2);
}

static size_t
get_prev_stations(const char *from_code, const char *train, struct stop_list *list)
{
	const char *prefix = "";
	const char *api_url = "http://dv.njtransit.com/mobile/train_stops.aspx?sid=%s&train=%s%s";
	if (debug_server)
		api_url = "http://127.0.0.1:8000/njtransit-train-%s-%s%s.html";

	char fname[PATH_MAX];
	char url[100];

	snprintf(fname, PATH_MAX, "/tmp/njtransit-train-%s-%s.html", from_code, train);

	if (!debug_server && strlen(train) == 2)
		prefix = "00";

	snprintf(url, 100, api_url, from_code, prefix, train);

	if (expired(fname)) {
		if (fetch_url(url, fname) != 0)
			return -1;

		if (debug)
			printf("%s fetched\n", fname);
	}

	parse_train_stops(fname, list);

	if (debug) {
		struct stop *stop;
		SLIST_FOREACH(stop, list, entries) {
			printf("stop: %s(%s), %s\n", stop->name, stop->code, stop->status);
		}
	}

	return 0;
}

static struct stop *
stop_find(struct stop_list *list, const char *station_code)
{
	struct stop *stop;

	SLIST_FOREACH(stop, list, entries) {
		if (strcmp(stop->code, station_code) == 0)
			return stop;
	}

	return NULL;
}

static struct stop_list *
reversed(struct stop_list *list)
{
	struct stop *stop, *newstop;
	struct stop_list *nl = calloc(1, sizeof(struct stop_list));

	SLIST_FOREACH(stop, list, entries) {
		newstop = calloc(1, sizeof(struct stop));
		memcpy(newstop, stop, sizeof(struct stop));
		SLIST_INSERT_HEAD(nl, newstop, entries);
	}

	return nl;
}

static int
departures_get_upcoming(const char* from_code, const char *dest_code, struct buf *b)
{
	struct station *st = station_create(from_code);

	dest_code = propose_destinations(st, dest_code);

	if (dest_code == NULL)
		return 1;

	struct departure *next = departures_nearest(st->deps, dest_code);
	if (next == NULL)
		errx(1, "No next departure to %s(%s) found", station_name(dest_code), dest_code);
	if (debug)
		printf("next: %s %s\n", next->time, next->train);

	struct stop_list *route = calloc(1, sizeof(struct stop_list));

	get_prev_stations(from_code, next->train, route);

	if (SLIST_EMPTY(route))
		errx(1, "No route found for train %s from %s to %s", next->train, from_code, dest_code);

	struct stop_list *rev_route = reversed(route);

	struct stop *origin_stop = stop_find(rev_route, from_code);
	if (origin_stop == NULL)
		errx(1, "Strange that cannot get origin stop code from reversed route");

	const size_t MAXPREV = 10;
	struct station_list *slist = calloc(1, sizeof(struct station_list));

	size_t i = 0;

	struct stop *stop = SLIST_NEXT(origin_stop, entries);

	while (stop != NULL) {

		struct station *st = station_create(stop->code);

		if (st == NULL) {
			fprintf(stderr, "Cannot get departures for station code %s\n", stop->code);
			return 1;
		}

		if (debug) {
			printf("== Departures for %s(%s) ==\n", station_name(stop->code), stop->code);
			station_dump(st);
			printf("--\n");
		}

		stop = SLIST_NEXT(stop, entries);
	}
/*
	const char *final_name = station_name();
	const char *dest_name = station_name(route[dest_idx]);
	const char *from_name = station_name(route[idx]);

	int n;
	char s[1024];
	size_t sz = sizeof(s);

	if (nearest != NULL) {
		n = snprintf(s, sz, "Next train from %s to %s\n\n", from_name, dest_name);
		buf_append(b, s, n);

		n = snprintf(s, sz, "%s #%s, Track %s %s. Train status:\n\n",
			nearest->time, nearest->train, nearest->track, nearest->status);
		buf_append(b, s, n);

		stations_latest_status(sts, route, MAXPREV, idx, nearest->train, b);

		if (all) {
			station_dump(sts[0]);
			printf("\n");
		}
	} else {
		n = snprintf(s, sz, "No trains from %s to %s\n", from_name, dest_name);
		buf_append(b, s, n);
	}
*/
	buf_append(b, credits, sz_credits);

//	for (i = 0; i < MAXPREV; i++)
//		station_destroy(sts[i]);

	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		synopsis();
		return 1;
	}

	int ch;

	while ((ch = getopt_long(argc, argv, "lhdsmvaf:t:p:", longopts, NULL)) != -1) {
		switch (ch) {
			case 'd':
				debug = 1;
				log = fopen("/tmp/departures-debug.log", "wt");
				fprintf(log, "==================\n\n\n\n\n\n\n");
				break;
			case 'm':
				email = 1;
				break;
			case 'a':
				all = 1;
				break;
			case 'l':
				stations_list();
				return 0;
			case 'f':
				station_from = optarg;
				break;
			case 'p':
				train = optarg;
				break;
			case 't':
				station_to = optarg;
				break;
			case 's':
				debug_server = 1;
				break;
			case 'h':
				usage();
				return 1;
			case 'v':
				printf("departures\n");
				printf("version %s\n", app_version);
				printf("date %s\n", app_date);
				if (strlen(app_diff) > 0)
					printf("uncommited changes:\n%s\n", app_diff);
				return 1;
			default:
				synopsis();
				return 1;
		}
	}

	if (station_from == NULL)
		errx(1, "Origin station is not specified");

	curl_global_init(CURL_GLOBAL_ALL);

	struct buf b;
	memset(&b, 0, sizeof(struct buf));

	if (departures_get_upcoming(station_from, station_to, &b) == 0) {
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
	}

	curl_global_cleanup();
	return 0;
}

