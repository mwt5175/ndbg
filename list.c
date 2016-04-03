/************************************************************************
*
*	list.c - Linked list
*
************************************************************************/

#include <string.h>
#include <malloc.h>
#include "list.h"

/**
* Initialize list
* \arg in List
*/
list* listInit (list* in) {
	in->first = in->last = 0;
	in->count = 0;
	return in;
}

/**
* Adds element to list
*/
listNode* listAddElement (void* data, int size, list* root) {

	listNode* node = malloc (sizeof (listNode));
	if (!node)
		return 0;

	node->data = malloc (size);
	memcpy (node->data, data, size);
	root->count++;

	if (root->count == 1) {

		root->first = node;
		root->last = node;
		node->prev = 0;
		node->next = 0;
	}
	else {

		node->prev = root->last;
		node->next = 0;
		root->last->next = node;
		root->last = node;
	}
	return node;
}

/**
*	returns size of list
*/
unsigned int listSize (list* root) {
	if (root)
		return root->count;
	return 0;
}

/**
*	removes element
*/
listNode* listRemoveElement (unsigned int num, list* root) {

	// test if in range
	if (num >= root->count || num < 0)
		return 0;
	else if (num == root->count-1 && num == 0) {

		// this is the last element in the list
		listNode* node = root->first;
		free (node);
		root->count--;
		root->first=0;
		root->last=0;
		return node;
	}
	else if (num == 0) {

		listNode* node = root->first;
		root->first = node->next;
		node->next->prev = 0;
		free (node);
		root->count--;
		return node;
	}
	else if (num == root->count-1) {

		listNode* node = root->last;
		node->prev->next = 0;
		root->last = root->last->prev;
		free (node);
		root->count--;
		return node;
	}
	else {

		listNode* node = root->first;
		unsigned int i=0;
		for (i=0; i< num; i++)
			node = node->next;
		node->next->prev = node->prev;
		node->prev->next = node->next;
		root->count--;
		return node;
	}

	return 0;
}

/**
*	frees a list and elements
*/
void listFreeAll (list* root) {

	listNode* current = root->first;
	unsigned int c=0;

	if (!current)
		return;

	// free all data nodes
	for (c=0; c<root->count; c++) {

		if (current->data) {
			free (current->data);
			current->data = 0;
		}
		current = current->next;
	}

	// remove all elements
	while (root->count)
		listRemoveElement (0, root);
	free (root->first);

	// no more elements
	root->first = 0;
	root->last = 0;
	root->count = 0;
}
