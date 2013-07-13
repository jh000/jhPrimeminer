
typedef struct _fastSorter_treeEntry_t
{
	void *p;
	signed int value;
	_fastSorter_treeEntry_t *left; // less
	//_fastSorter_treeEntry_t *center; // equal
	_fastSorter_treeEntry_t *right; // above
}fastSorter_treeEntry_t;

typedef struct
{
	fastSorter_treeEntry_t *nodeList;
	unsigned int nodeCount;
	unsigned int nodeLimit;
	bool autoIncreaseLimit;
	fastSorter_treeEntry_t **traverseStack; /* small buffer(4*nodeLimit bytes) for non-recursive tree traversal */
}fastSorter_t;


void fastSorter_init(fastSorter_t *f, unsigned int initialLimit, bool autoIncreaseLimit);
void fastSorter_clear(fastSorter_t *f);
void fastSorter_add(fastSorter_t *f, signed int value, void *p);
void fastSorter_traverseAscending(fastSorter_t *f, void (__fastcall *cb)(void *p));