// Serge Voilokov, 2015
// generic data structures. See datautil.h

#include "struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>

void
buf_init(struct buf *b)
{
	memset(b, 0, sizeof(struct buf));
}

void
buf_clean(struct buf *b)
{
	if (b->s != NULL)
		free(b->s);
	buf_init(b);
}

void
buf_append(struct buf *b, const char *s, size_t n)
{
	if (b->len + n >= b->cap) {
		b->cap = (b->len + n + 1) * 2;
		if (b->s == NULL)
			b->s = malloc(b->cap);
		else
			b->s = realloc(b->s, b->cap);
	}

	memcpy(b->s + b->len, s, n);
	b->len += n;
	b->s[b->len] = 0;
}

void
buf_appendf(struct buf *b, const char *fmt, ...)
{
	va_list args, cp;
	va_start(args, fmt);
	va_copy(cp, args);

	int n = vsnprintf(NULL, 0, fmt, cp);

	if (b->len + n >= b->cap) {
		b->cap = (b->len + n + 1) * 2;
		if (b->s == NULL)
			b->s = malloc(b->cap);
		else
			b->s = realloc(b->s, b->cap);
	}

	vsnprintf(&b->s[b->len], n+1, fmt, args);
	b->len += n;

	va_end (args);
}
