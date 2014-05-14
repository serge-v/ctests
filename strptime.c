#include <string.h>
#include <stdio.h>
#include <time.h>

int main()
{
	char date[] = "2013-11-01T17:51:05Z";
	struct tm t;
	char tz[3];

	int rc = sscanf(date, "%04d-%02d-%02dT%02d:%02d:%02d%3s", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec, tz);

	t.tm_year -= 1900;
	t.tm_mon -= 1;
	
	printf("parsed %d: %04d-%02d-%02dT%02d:%02d:%02d%s\n", rc, t.tm_year+1900, t.tm_mon-1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, tz);
	tzset();
	time_t tt = mktime(&t) - timezone + daylight*3600;
	printf("time_t: %lu, equal: %d\n", tt, tt == 1383328265);
	printf("ctime: %s\n", ctime(&tt));
}
