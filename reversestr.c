#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <ctype.h>

static char str1[] = "abcdefghijklmnopqrstu";
static char str2[] = "abcdefghijklmnopqrst";

static void
strrev(char *s)
{
	int i;
	int len = strlen(s);
	
	for (i = 0; i < len / 2; i++) {
		char tmp = s[i];
		s[i] = s[len-i-1];
		s[len-i-1] = tmp;
	}
}


static uint64_t
atoi(const char *s)
{
	uint64_t res = 0;
	uint64_t mul = 1;

	int i;
	int len = strlen(s);
	
	for (i = len - 1; i >= 0; i--) {
		res += (s[i] - '0') * mul;
		mul *= 10;
	}

	return res;
}

struct list
{
	struct list *next;
	int data;
};

struct list el5 = { NULL, 5 };
struct list el4 = { &el5, 4 };
struct list el3 = { &el4, 3 };
struct list el2 = { &el3, 2 };
struct list el1 = { &el2, 1 };
struct list el0 = { &el1, 0 };


static void
print(struct list *list) {
	while (list) {
		printf("%d ", list->data);
		list = list->next;
	}
	printf("\n");
}

static struct list *
listrev(struct list *p)
{
	struct list *prev = NULL;

	while (p != NULL) {
		struct list *next = p->next;
		p->next = prev;
		prev = p;
		p = next;
	}
	
	return prev;
}

int strcasecmp(const char *s1, const char *s2)
{
	while (1) {
		if (*s1 == 0 && *s2 == 0)
			return 0;

		if (*s1 == 0 && *s1 != 0)
			return -1;

		if (*s2 != 0 && *s1 == 0)
			return -1;

		if (tolower(*s1) > tolower(*s2))
			return 1;
	
		if (tolower(*s1) < tolower(*s2))
			return -1;

		s1++;
		s2++;
	}
	
	return 0;
}

int main()
{
	strrev(str1);
	printf("%s\n", str1);
	strrev(str2);
	printf("%s\n", str2);
	
	const char s1[] = "123456789";
	printf("%s = %" PRIu64 "\n", s1, atoi(s1));

	print(&el0);	
	struct list *rl = listrev(&el0);
	print(rl);
	
	printf("test=test: %d\n", strcasecmp("test", "test"));
	printf("Test=tesT: %d\n", strcasecmp("Test", "tesT"));
	printf("test=tes: %d\n", strcasecmp("test", "tes"));
	printf("TES=test: %d\n", strcasecmp("TES", "test"));
	printf("test=TEST: %d\n", strcasecmp("test", "TEST"));
	printf("TEST=test: %d\n", strcasecmp("TEST", "test"));
}
