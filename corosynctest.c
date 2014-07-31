//#include <config.h>

#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <corosync/corotypes.h>
#include <corosync/cpg.h>

static void deliver(cpg_handle_t handle, const struct cpg_name *group_name, uint32_t nodeid,
                    uint32_t pid, void *msg, size_t msg_len)
{
	printf("deliver group: %s, nodeid %x, handle: %lx, pid: %u, msg: %s, len: %lu\n", group_name->value, nodeid, handle, pid, (char*)msg, msg_len);
}

static void confch(
    cpg_handle_t handle,
    const struct cpg_name *group_name,
    const struct cpg_address *member_list, size_t member_list_entries,
    const struct cpg_address *left_list, size_t left_list_entries,
    const struct cpg_address *joined_list, size_t joined_list_entries)
{
	size_t i;

	printf("confchg group: %s, nodeid %x, handle: %lx\n", group_name->value, member_list[0].nodeid, handle);

	printf("  members: %lu\n", member_list_entries);
	for (i = 0; i < member_list_entries; i++)
		printf("    nodeid: %u, pid: %u, reason: %u\n", member_list[i].nodeid, member_list[i].pid, member_list[i].reason);

	printf("  left: %lu\n", left_list_entries);
	for (i = 0; i < left_list_entries; i++)
		printf("    nodeid: %u, pid: %u, reason: %u\n", left_list[i].nodeid, left_list[i].pid, left_list[i].reason);

	printf("  joined: %lu\n", joined_list_entries);
	for (i = 0; i < joined_list_entries; i++)
		printf("    nodeid: %u, pid: %u, reason: %u\n", joined_list[i].nodeid, joined_list[i].pid, joined_list[i].reason);
}

struct client
{
	cpg_handle_t handle;
	int fd;
	unsigned int nodeid;
};

int client_join(struct client* c)
{
	int rc;
	memset(c, 0, sizeof(struct client));
	cpg_callbacks_t cb = { &deliver, &confch };
	struct cpg_name group = { 3, "foo" };
	struct iovec msg = {(void *)"hello", 5};   /* discard const */

	rc = cpg_initialize(&c->handle, &cb);
	if (rc != CS_OK)
	{
		printf("cannot connect to corosync\n");
		return -1;
	}

	assert(CS_OK == cpg_local_get(c->handle, &c->nodeid));
	printf("nodeid: %x, handle: %lx\n", c->nodeid, c->handle);
	rc = cpg_fd_get(c->handle, &c->fd);
	printf("cpg_fd_get: %d, fd: %d\n", rc, c->fd);
	rc = cpg_join(c->handle, &group);
	printf("join: %d\n", rc);
	assert(rc == CS_OK);
	assert(CS_OK == cpg_mcast_joined(c->handle, CPG_TYPE_AGREED, &msg, 1));
	return 0;
}

void client_close(struct client* c)
{
	cpg_finalize(c->handle);
	memset(c, 0, sizeof(struct client));
}

#define MAX_FD 2

int main()
{
	struct client client;
	struct pollfd pfd[MAX_FD];
	int rc, i;
	int quit = 0, reconnect = 1;

	while (!quit)
	{
		if (reconnect)
		{
			memset(pfd, 0, sizeof(pfd));
			pfd[0].events = POLLIN;
			pfd[1].events = POLLIN;
			if (client_join(&client) == 0)
			{
				pfd[0].fd = 0;
				pfd[1].fd = client.fd;
				reconnect = 0;
			}
		}

		rc = poll(pfd, MAX_FD, 5000);
		cpg_dispatch(client.handle, CS_DISPATCH_ALL);
		assert(rc >= 0);

		for (i = 0; i < MAX_FD; i++)
		{
			if (pfd[i].fd == 0 && pfd[i].revents == 1)
				quit = 1;

			if (pfd[i].fd == client.fd && pfd[i].revents == (POLLIN | POLLHUP))
				reconnect = 1;
		}
	}

	client_close(&client);

	return (0);
}
