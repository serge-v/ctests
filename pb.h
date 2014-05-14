#include <stdint.h>

#define COUNT(x) sizeof(x) / sizeof(x[0])

struct payload
{
	struct message* msg;                      // can be msg_string or msg_ignored or msg_custom
	size_t          child_count;              // count of messages in msg field
	uint8_t         packed;                   // packed flag
	void (*print)(FILE* f, uint64_t value);   // visualizer for msg_custom
};

struct message
{
	char           name[20];
	uint8_t        id;
	struct payload p;
};

enum wire_type
{
	WT_VARINT   = 0,
	WT_FIXED64  = 1,
	WT_LENPX    = 2,
	WT_FIXED32  = 5
};

void enum_pb(FILE* f, struct message* msg, size_t child_count, const uint8_t* buf, size_t len, int depth);
void pack_pair(uint64_t id, enum wire_type wt, uint64_t value, uint8_t** ptr);
void pack_uint64(uint64_t v, uint8_t** ptr);
void pack_string(const char* s, uint8_t** ptr);
size_t uint64_len(uint64_t v);

extern struct message msg_string[];
extern struct message msg_ignored[];
extern struct message msg_custom[]; // should have print handler

