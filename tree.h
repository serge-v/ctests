struct node
{
	struct node* parent; 	// parent
	struct node* child;		// first child
	struct node* next;		// next sibling
	char* name;
};

struct node* node_parse(const char* path);
void node_free(struct node* node);
void node_dump(struct node* node, int indent);

struct node* node_find(struct node* node, const char* path);
const char* node_text(struct node* node);
