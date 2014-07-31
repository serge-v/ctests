#include <stdio.h>
#include "tree.h"

int main(int argc, char** argv)
{
	const char *path = "testcfg.txt";
	if (argc > 1)
		path = argv[1];

	struct node* root = node_parse(path);
	node_dump(root, 0);

	const char* paths[] = {
		"1/11/112",
		"1/12/123",
		"1/11/111",
		"2/2/214",
		"2/servers",
	};
	
	int i = 0;
	for (i = 0; i < sizeof(paths) / sizeof(paths[0]); i++)
	{
		const char* path = paths[i];
		struct node* n = node_find(root, path);
		if (n)
			printf("found: %s at %s. text: %s\n", n->name, path, node_text(n));
		else
			printf("not found at: %s\n", path);
	}
	
	struct node* n = node_find(root, "2/servers");
	struct node* c;
	for (c = n->child ; c != NULL; c = c->next)
		printf("child: %s\n", c->name);

	node_free(root);
}
