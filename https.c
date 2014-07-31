#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/stat.h>
#include <fcntl.h>

static int open_listener(int port)
{
	struct sockaddr_in addr;

	int sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	int one = 1;

	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)) == -1)
	{
		perror("Cannot set SO_REUSEADDR");
		goto err;
	}

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0)
	{
		perror("can't bind port");
		goto err;
	}

	if (listen(sd, 10) != 0)
	{
		perror("Cannot configure listening port");
		goto err;
	}

	return sd;
err:
	if (sd > 0)
		close(sd);

	return -1;
}

SSL_CTX* init_server_ctx(void)
{
	SSL_CTX *ctx;

	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	ctx = SSL_CTX_new(SSLv23_server_method());

	if (ctx == NULL)
	{
		ERR_print_errors_fp(stderr);
		return NULL;
	}

	return ctx;
}

static int init_certs(SSL_CTX *ctx)
{
	struct stat st;
	if (stat("server~.crt", &st) != 0)
	{
		printf("generating certificate\n");
		if (system("./gencert.sh") != 0)
		{
			perror("Cannot generate certificate\n");
			goto err;
		}
	}

	if (SSL_CTX_use_certificate_file(ctx, "server~.crt", SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		goto err;
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, "server~.key", SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		goto err;
	}

	if (!SSL_CTX_check_private_key(ctx))
	{
		fprintf(stderr, "Private key does not match the public certificate\n");
		goto err;
	}

	return 0;
err:
	return -1;
}

int main(int count, char**argv)
{
	SSL_CTX *ctx;

	int server, rc = 1;
	char *portnum;

	if ( count != 2 )
	{
		printf("Usage: %s <portnum>\n", argv[0]);
		return 2;
	}

	portnum = argv[1];

	SSL_library_init();
	ctx = init_server_ctx();

	if (init_certs(ctx) < 0)
		goto err;

	server = open_listener(atoi(portnum));
	if (server < 0)
		goto err;

	char buf[4096];
	char buf1[4096];
	SSL *ssl = SSL_new(ctx);

	while (1)
	{
		struct sockaddr_in addr;
		socklen_t len = sizeof(addr);

		int client = accept(server, (struct sockaddr*)&addr, &len);
		printf("Connection: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

		if (SSL_clear(ssl) == 0)
		{
			ERR_print_errors_fp(stderr);
			goto err;
		}

		SSL_set_fd(ssl, client);

		int sd, bytes, rc, total = 0, sslerr;
		const char headers[] =
		    "HTTP/1.1 200 OK\r\n"
		    "Content-Type: text/plain\r\n"
		    "\r\n"
		    "===\n";

		if (SSL_accept(ssl) == -1)
		{
			ERR_print_errors_fp(stderr);
			goto close;
		}

		char* ptr = buf;

		do
		{
			bytes = SSL_read(ssl, ptr, sizeof(buf) - total);
			sslerr = SSL_get_error(ssl, bytes);
			if (sslerr != SSL_ERROR_NONE)
				break;

			printf("bytes: %d, SSL_get_error: %d\n", bytes, sslerr);
			ptr += bytes;
			total += bytes;

			if (total >= 0)
			{
				buf[total] = 0;
				printf("total: %d, buf: [[[%s]]]\n", total, buf);
			}

			if (strncmp(buf, "GET ", 4) == 0) // TODO: need to process keep-alive here
				break;
		}
		while (sslerr == 0 && bytes > 0);

		if (total > 0)
		{
			rc = SSL_write(ssl, headers, sizeof(headers) - 1);
			printf("SSL_write: %d\n", rc);

			FILE* f = popen("ls -l /proc", "r");
			while (!feof(f))
			{
				int was_read = fread(buf1, 1, 4096, f);
				if (was_read > 1)
				{
					buf1[was_read] = 0;
					rc = SSL_write(ssl, buf1, was_read);
					printf("SSL_write: %d\n", rc);
				}
			}
			fclose(f);
		}

close:
		sd = SSL_get_fd(ssl);
		close(sd);
	}

	SSL_free(ssl);
	rc = 0;

err:

	close(server);
	SSL_CTX_free(ctx);

	return rc;
}

