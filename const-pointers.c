struct str
{
	int v;
};

static struct str s1;
static struct str s2;

typedef const struct str *const rostr;

int main()
{
	const struct str *ptr1 = &s1;
	ptr1++;

	const struct str *const ptr2 = &s1;
	
	int a = ptr2->v;

//	b->v = 1;
}
