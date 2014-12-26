#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <stdlib.h>

#include "common/net.h"

int main(int argc, char **argv)
{
	if (argc < 4) {
		printf("mailto. Tool to send short email from me to somebody\n");
		printf("usage: mailto \"recipient <email@host.com>\" \"subject\" \"short body\"\n");
		return 1;
	}

	struct message m = {
		.from = "serge0x76@gmail.com",
		.to = argv[1],
		.subject = argv[2],
		.body = argv[3]
	};

	int rc = send_email(&m);

	if (rc != 0) {
		printf("error: %d\n", rc);
		return 1;
	}

	return 0;
}

