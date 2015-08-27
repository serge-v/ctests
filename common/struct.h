#include <unistd.h>

struct buf
{
	char *s;         /* data pointer */
	size_t len;      /* data length */
	size_t cap;      /* allocated capacity */
};

void buf_init(struct buf *b);
void buf_clean(struct buf *b);
void buf_append(struct buf *b, const char *s, size_t n);
void buf_appendf(struct buf *b, const char *fmt, ...);
