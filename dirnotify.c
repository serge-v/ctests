#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

#define BUFF_SIZE ((sizeof(struct inotify_event) + FILENAME_MAX) * 1024)

void get_event(int fd, const char* target)
{
	ssize_t len, i = 0;
	char buff[BUFF_SIZE] = {0};

	len = read(fd, buff, BUFF_SIZE);

	while (i < len)
	{
		struct inotify_event *pevent = (struct inotify_event *)&buff[i];
		char action[81 + FILENAME_MAX] = {0};

		if (pevent->len)
			strcpy(action, pevent->name);
		else
			strcpy(action, target);
		strcat(action, ": ");

		if (pevent->mask & IN_ACCESS)
			strcat(action, "IN_ACCESS ");
		else if (pevent->mask & IN_ATTRIB)
			strcat(action, "IN_ATTRIB ");
		else if (pevent->mask & IN_CLOSE_WRITE)
			strcat(action, "IN_CLOSE_WRITE ");
		else if (pevent->mask & IN_CLOSE_NOWRITE)
			strcat(action, "IN_CLOSE_NOWRITE ");
		else if (pevent->mask & IN_CREATE)
			strcat(action, "IN_CREATE ");
		else if (pevent->mask & IN_DELETE)
			strcat(action, "IN_DELETE ");
		else if (pevent->mask & IN_DELETE_SELF)
			strcat(action, "IN_DELETE_SELF ");
		else if (pevent->mask & IN_MODIFY)
			strcat(action, "IN_MODIFY ");
		else if (pevent->mask & IN_MOVE_SELF)
			strcat(action, "IN_MOVE_SELF ");
		else if (pevent->mask & IN_MOVED_FROM)
			strcat(action, "IN_MOVED_FROM ");
		else if (pevent->mask & IN_MOVED_TO)
			strcat(action, "IN_MOVED_TO ");
		else if (pevent->mask & IN_OPEN)
			strcat(action, "IN_OPEN ");

		printf("%s\n", action);
		fflush(stdout);
		i += sizeof(struct inotify_event) + pevent->len;
	}
}

uint32_t get_mask(const char* s)
{
	uint32_t mask = 0;

	if (0 == strcmp(s, "IN_ACCESS"))
		mask = IN_ACCESS;
	else if (0 == strcmp(s, "IN_ATTRIB"))
		mask = IN_ATTRIB;
	else if (0 == strcmp(s, "IN_CLOSE_WRITE"))
		mask = IN_CLOSE_WRITE;
	else if (0 == strcmp(s, "IN_CLOSE_NOWRITE"))
		mask = IN_CLOSE_NOWRITE;
	else if (0 == strcmp(s, "IN_CREATE"))
		mask = IN_CREATE;
	else if (0 == strcmp(s, "IN_DELETE"))
		mask = IN_DELETE;
	else if (0 == strcmp(s, "IN_DELETE_SELF"))
		mask = IN_DELETE_SELF;
	else if (0 == strcmp(s, "IN_MODIFY"))
		mask = IN_MODIFY;
	else if (0 == strcmp(s, "IN_MOVE_SELF"))
		mask = IN_MOVE_SELF;
	else if (0 == strcmp(s, "IN_MOVED_FROM"))
		mask = IN_MOVED_FROM;
	else if (0 == strcmp(s, "IN_MOVED_TO"))
		mask = IN_MOVED_TO;
	else if (0 == strcmp(s, "IN_OPEN"))
		mask = IN_OPEN;

	return mask;
}

int main(int argc, char** argv)
{
	int queue = 0;
	int watchd = 0;

	if (argc < 2)
	{
		printf("Usage:\ndirnotify DIR MASK...\n");
		return 1;
	}

	const char* dir = argv[1];

	uint32_t mask = IN_ALL_EVENTS;
	int i;
	for (i = 2; i < argc; i++)
		mask |= get_mask(argv[i]);

	queue = inotify_init();
	if (queue < 0)
	{
		fprintf(stderr, "Cannot init inotify. Error: %d\n", errno);
		return 1;
	}

	watchd = inotify_add_watch(queue, dir, mask);
	if (watchd < 0)
	{
		fprintf(stderr, "Cannot add watch. Error: %d\n", errno);
		return 1;
	}

	while (1)
		get_event(queue, dir);

	return 0;
}
