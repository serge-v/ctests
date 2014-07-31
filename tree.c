#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

static struct node* node_alloc(const char* name)
{
	struct node* node = malloc(sizeof(struct node));
	node->parent = NULL;
	node->child = NULL;
	node->next = NULL;
	node->name = strdup(name);
	return node;
}

void node_free(struct node* node)
{
	free(node->name);
	struct node* child = node->child;
	while (child)
	{
		struct node* prev = child;
		child = child->next;
		node_free(prev);
	}
	free(node);
}

static void node_add_child(struct node* new, struct node* node)
{
	if (!node->child)
	{
		node->child = new;
		new->parent = node;
		return;
	}

	struct node* last_child = node->child;

	while (last_child)
	{
		if (!last_child->next)
		{
			last_child->next = new;
			new->parent = node;
			return;
		}
		last_child = last_child->next;
	}
}

/*
static void node_add_next(struct node* new, struct node* node)
{
	if (!node->next)
	{
		node->next = new;
		new->parent = node;
		return;
	}

	struct node* last_sibling = node->next;

	while (last_sibling)
	{
		if (!last_sibling->next)
		{
			last_sibling->next = new;
			new->parent = node;
			return;
		}
		last_sibling = node->next;
	}
}
*/

void node_dump(struct node* node, int indent)
{
	printf("%*s%s\n", indent * 2, "", node->name);
	fflush(stdout);
	struct node* child = node->child;
	while (child)
	{
		node_dump(child, indent + 1);
		child = child->next;
	}
}

struct node* node_find(struct node* node, const char* path)
{
	const char* sl = strchr(path, '/');
	if (sl)
	{
		struct node* last_child = node->child;
		while (last_child)
		{
			if (strncmp(last_child->name, path, sl - path) == 0)
				return node_find(last_child, sl+1);
			last_child = last_child->next;
		}
	}
	else
	{
		struct node* last_child = node->child;
		while (last_child)
		{
			if (strcmp(last_child->name, path) == 0)
				return last_child;
			last_child = last_child->next;
		}
	}
	return NULL;
}

const char* node_text(struct node* node)
{
	if (node->child)
		return node->child->name;
	return NULL;
}

static int count_leading_spaces(char* s)
{
	if (s == NULL || *s == 0)
		return -1;	// string is empty
	
	// trim EOL

	char* nl = strchr(s, '\n');
	*nl = 0;

	if (s == NULL || *s == 0)
		return -1; // string is empty after EOL trimming
	
	int c = 0;

	// count spaces

	while (*s && *s == ' ')
	{
		c++;
		s++;
	}

	if (!*s)
		return -1; // string contains spaces only

	return c;
}

struct node* node_parse(const char* path)
{
	char s[256];
	int i;
	int indent = 0;
	int prev_indent = 0;
	int line_num = 0;
	
	FILE* f = fopen(path, "rt");
	if (!f)
	{
		fprintf(stderr, "cannot open file: %s\n", path);
		return NULL;
	}

	struct node* root = node_alloc("root");
	struct node* last_parent = root;
	struct node* last_added = root;

	while (1)
	{
		fgets(s, 256, f);
		line_num++;

		if (feof(f))
			break;

		int sc = count_leading_spaces(s);
		if (sc < 0)
			continue;
			
		indent = sc / 2;
		
		if (prev_indent != -1 && indent > prev_indent + 1)
		{
			fprintf(stderr, "wrong indent at line: %d\n", line_num);
			return NULL;
		}

		if (indent > prev_indent)
			last_parent = last_added;
		else
			for (i = 0; i < prev_indent - indent; i++)
				last_parent = last_parent->parent;

		last_added = node_alloc(s + sc);
		node_add_child(last_added, last_parent);
		prev_indent = indent;
	}
	
	return root;
}
