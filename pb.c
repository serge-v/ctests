#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "pb.h"

struct message msg_string[] = {};
struct message msg_ignored[] = {};
struct message msg_custom[] = {};

size_t uint64_len(uint64_t v)
{
	uint8_t i = 0;
	while (v >= 0x80)
	{
		v >>= 7;
		i++;
	}
	i++;
	return i;
}

void pack_uint64(uint64_t v, uint8_t** ptr)
{
	while (v >= 0x80)
	{
		**ptr = (v & 0xFF) | 0x80;
		v >>= 7;
		(*ptr)++;
	}
	**ptr = v;
	(*ptr)++;
}

void pack_string(const char* s, uint8_t** ptr)
{
	size_t len = strlen(s);
	memcpy(*ptr, s, len);
	(*ptr) += len;
}

static void tag_pack(uint64_t id, enum wire_type wt, uint8_t** ptr)
{
	pack_uint64((id) << 3 | wt, ptr);
}

void pack_pair(uint64_t id, enum wire_type wt, uint64_t value, uint8_t** ptr)
{
	tag_pack(id, wt, ptr);
	pack_uint64(value, ptr);
}

struct pbstream
{
	const uint8_t*  p;      // current pointer
	const uint8_t*  pp;     // payload pointer
	const uint8_t*  end;    // end of buffer
	uint64_t	tag;    // current tag
	uint64_t	id;     // current id
	enum wire_type  wt;     // current wire type

	union
	{
		uint64_t len;   // payload length; OR
		uint64_t value; // value
	};
};

static uint64_t read_v64(struct pbstream* s)
{
	uint64_t v = 0;
	int shifts = 0;

	while (s->p < s->end)
	{
		uint8_t b = *s->p & 0x7f;
		v = v | ((uint64_t)b << shifts);
		if ((*s->p++ & 0x80) == 0)
			return v;
		shifts += 7;
	}

	return 0;
};

static int read_pair(struct pbstream* s, int depth)
{
	s->tag = read_v64(s);
	s->pp = s->p;

	s->wt = s->tag & 0x07;
	s->id = s->tag >> 3;

	switch (s->wt)
	{
	case WT_VARINT:
		s->value = read_v64(s);
		break;

	case WT_LENPX:
		s->len = read_v64(s);
		s->pp = s->p;
		s->p += s->len;
		break;

	case WT_FIXED64:
		s->value = *(uint64_t*)s->p;
		s->p += 8;
		break;

	case WT_FIXED32:
		s->value = *(uint32_t*)s->p;
		s->p += 4;
		break;

	default:
		printf("%*s%lu: [%d] -- unsupported tag type\n", depth * 4, "", s->id, s->wt);
		const uint8_t* p = s->p - 1;
		int i = 30;
		while (p < s->end && i--)
			printf("%02x ", *p++);
		printf("\n");
		exit(1);
		break;
	}

	return (s->p < s->end) ? 0 : -1;
}

struct message msg_invalid = { "", 0, {0} };

static const struct message* get_descr(const struct message* msg, size_t child_count, uint64_t id)
{
	size_t i;
	for (i = 0; i < child_count; i++)
	{
		if (msg[i].id == id)
			return &msg[i];
	}

	return &msg_invalid;
}

static void enum_packed(FILE* f, const struct message* field, struct pbstream* s, int depth)
{
	fprintf(f, "%*s%s[%lu,packed]\n", depth * 4, "", field->name, s->id);

	struct pbstream s8 = { .p = s->pp, .end = s->pp + s->len };

	do
	{
		uint64_t v = read_v64(&s8);

		if (field->p.msg == msg_custom && field->p.print != NULL)
		{
			fprintf(f, "%*s", (depth + 1) * 4, "");
			field->p.print(f, v);
		}
		else
			fprintf(f, "%*s%lu\n", (depth + 1) * 4, "", v);
	}
	while (s8.p < s8.end);
}

void enum_pb(FILE* f, struct message* msg, size_t child_count, const uint8_t* buf, size_t len, int depth)
{
	struct pbstream ss = { .p = buf, .end = buf + len };
	struct pbstream* s = &ss;

	int rc = 0;

	do
	{
		rc = read_pair(s, depth);
		const struct message* field = get_descr(msg, child_count, s->id);
//		printf(">>> %s,%d,%p\n", field->name, field->p.msg == msg_enum, field->p.print);
		if (field->p.packed)
			enum_packed(f, field, s, depth);
		else if (s->wt == WT_LENPX)
		{
			if (field->p.msg == msg_string)
				fprintf(f, "%*s%s[%lu,%02lx]: %.*s\n", depth * 4, "", field->name, s->id, s->tag, (int)s->len, s->pp);
			else
			{
				fprintf(f, "%*s%s[%lu,%02lx]\n", depth * 4, "", field->name, s->id, s->tag);
				enum_pb(f, field->p.msg, field->p.child_count, s->pp, s->len, depth + 1);
			}
		}
		else
		{
			if (field->p.msg == msg_custom && field->p.print != NULL)
			{
				fprintf(f, "%*s%s[%lu,%02lx]: ", depth * 4, "", field->name, s->id, s->tag);
				field->p.print(f, s->value);
			}
			else if (field->p.msg != msg_ignored)
				fprintf(f, "%*s%s[%lu,%02lx]: %lu\n", depth * 4, "", field->name, s->id, s->tag, s->value);
		}
	}
	while (rc == 0);
}

