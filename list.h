/************************************************************************
*
*	list.h - Linked list
*
************************************************************************/

#ifndef LIST_H_INCLUDED
#define LIST_H_INCLUDED

/**
*	list node
*/
typedef struct _listNode {
	struct _listNode* next;
	struct _listNode* prev;
	void*  data;
}listNode;

/**
*	linked list
*/
typedef struct _list {
	unsigned int count;
	listNode* first;
	listNode* last;
}list;

extern
list* listInit (list* root);

extern
listNode* listAddElement (void* data,int size,list* root);

extern
unsigned int listSize (list* root);

extern
listNode* listRemoveElement (unsigned int num,list* root);

extern
void listFreeAll (list* root);

#endif
