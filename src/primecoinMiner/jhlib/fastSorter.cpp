#include<Windows.h>
#include"fastSorter.h"

/*
 * Initializes and allocates a new fastSorter
 * @initialLimit is the number of elements that can be sorted(added) at once
 * @autoIncreaseLimit if true the limit is dynamically increased if needed.
 *					  if false, elements that do not fit are dropped
 */
void fastSorter_init(fastSorter_t *f, unsigned int initialLimit, bool autoIncreaseLimit)
{
	f->nodeCount = 0;
	f->nodeLimit = initialLimit;
	f->nodeList = (fastSorter_treeEntry_t*)malloc(sizeof(fastSorter_treeEntry_t) * initialLimit);
	f->traverseStack = (fastSorter_treeEntry_t**)malloc(sizeof(fastSorter_treeEntry_t*) * initialLimit);
	f->autoIncreaseLimit = autoIncreaseLimit;
}

void fastSorter_clear(fastSorter_t *f)
{
	f->nodeCount = 1;
	// set default entry
	f->nodeList[0].left = NULL;
	f->nodeList[0].right = NULL;
	f->nodeList[0].value = 0;
	f->nodeList[0].p = NULL;
}

void fastSorter_enlarge(fastSorter_t *f)
{
	unsigned int newLimit = f->nodeLimit;
	if( f->nodeLimit <= 8 )
		newLimit += 4;
	else
		newLimit += f->nodeLimit/2; // increase by 50%
	fastSorter_treeEntry_t *newNodeList = (fastSorter_treeEntry_t*)malloc(sizeof(fastSorter_treeEntry_t)*newLimit);
	if( f->nodeCount )
	{
		// copy nodes
		RtlCopyMemory(newNodeList, f->nodeList, sizeof(fastSorter_treeEntry_t)*f->nodeCount);
	}
	// correct pointers (there is always at least the default element)
	unsigned int cBias = (unsigned int)newNodeList - (unsigned int)f->nodeList;
	for(unsigned int i=0; i<f->nodeCount; i++)
	{
		if( newNodeList[i].left )
			newNodeList[i].left = (fastSorter_treeEntry_t*)((unsigned char*)(newNodeList[i].left) + cBias);
		if( newNodeList[i].right )
			newNodeList[i].right = (fastSorter_treeEntry_t*)((unsigned char*)(newNodeList[i].right) + cBias);
	}
	// free old list
	free(f->nodeList);
	// increase traverse stack
	fastSorter_treeEntry_t **newTraverseStack = (fastSorter_treeEntry_t**)malloc(sizeof(fastSorter_treeEntry_t*) * newLimit);
	free(f->traverseStack);
	f->traverseStack = newTraverseStack;
	// set new list
	f->nodeList = newNodeList;
	f->nodeLimit = newLimit;
}

void fastSorter_add(fastSorter_t *f, signed int value, void *p)
{
	if( f->nodeLimit == f->nodeCount )
	{
		if( f->autoIncreaseLimit == false )
			return;
		// increase limit
		fastSorter_enlarge(f);
	}
	// travel the tree
	fastSorter_treeEntry_t *entry = f->nodeList+0;
	while( true )
	{
		if( value < entry->value )
		{
			// left branch
			fastSorter_treeEntry_t *next = entry->left;
			if( next )
			{
				entry = next;
				continue;
			}
			else
			{
				// create left branch and leave
				next = f->nodeList + f->nodeCount;
				f->nodeCount++;	
				entry->left = next;
				next->left = NULL;
				next->right = NULL;
				next->p = p;
				next->value = value;
				return;
			}
		}
		else
		{
			// right branch
			fastSorter_treeEntry_t *next = entry->right;
			if( next )
			{
				entry = next;
				continue;
			}
			else
			{
				// create right branch and leave
				next = f->nodeList + f->nodeCount;
				f->nodeCount++;	
				entry->right = next;
				next->left = NULL;
				next->right = NULL;
				next->p = p;
				next->value = value;
				return;
			}
		}
	}



}

void fastSorter_traverseAscending(fastSorter_t *f, void (__fastcall *cb)(void *p))
{
	fastSorter_treeEntry_t **entryStack = f->traverseStack;
	unsigned int stackSize = 0;
	fastSorter_treeEntry_t *entry = f->nodeList+0;
	while( true )
	{
		if( entry->left )
		{
			// remember on stack
			entryStack[stackSize] = entry;
			stackSize++;
			// traverse left
			entry = entry->left;
			continue;
		}
		_fastSorter_ta_continue:
		// take value
		if( entry->p )
			cb(entry->p);
		// right
		if( entry->right )
		{
			// traverse right
			entry = entry->right;
			continue;
		}
		// if we get here, then node is parsed
		if( stackSize == 0 )
			return;
		stackSize--;
		entry = entryStack[stackSize];
		goto _fastSorter_ta_continue;
	}
}