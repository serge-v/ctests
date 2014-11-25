static void
unhexify(uint8_t *obuf, const char *ibuf)
{
	unsigned char c, c2;
	assert(!(strlen(ibuf) % 1)); /* must be even number of bytes */

	while (*ibuf != 0)
	{
		c = *ibuf++;
		if (c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'a' && c <= 'f')
			c -= 'a' - 10;
		else if (c >= 'A' && c <= 'F')
			c -= 'A' - 10;
		else
			assert(0);

		c2 = *ibuf++;
		if (c2 >= '0' && c2 <= '9')
			c2 -= '0';
		else if (c2 >= 'a' && c2 <= 'f')
			c2 -= 'a' - 10;
		else if (c2 >= 'A' && c2 <= 'F')
			c2 -= 'A' - 10;
		else
			assert(0);

		*obuf++ = (c << 4) | c2;
	}
}

static void
hexify(char *obuf, const uint8_t *ibuf, int len)
{
	unsigned char l, h;

	while (len != 0)
	{
		h = (*ibuf) / 16;
		l = (*ibuf) % 16;

		if (h < 10)
			*obuf++ = '0' + h;
		else
			*obuf++ = 'a' + h - 10;

		if (l < 10)
			*obuf++ = '0' + l;
		else
			*obuf++ = 'a' + l - 10;

		++ibuf;
		len--;
	}
}

static void
print_hex(const char* name, const uint8_t* b, size_t len)
{
	if (len >= 1024)
	{
		fprintf(stderr, "len >= 1024.\n");
		exit(1);
	}
	char hex[2048];
	memset(hex, 0, 2048);
	hexify(hex, b, len);
	printf("%s: %s\n", name, hex);
}

