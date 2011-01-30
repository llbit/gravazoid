#include <assert.h>
#include <stdlib.h>
#include "list.h"

void list_add(list_t** list, void* object)
{
	assert(list != NULL);
	if (*list == NULL) {
		*list = malloc(sizeof(list_t));
		(*list)->data = object;
		(*list)->succ = *list;
		(*list)->pred = *list;
	} else {
		list_t*	new = malloc(sizeof(list_t));
		new->data = object;
		new->succ = *list;
		new->pred = (*list)->pred;
		(*list)->pred = new;
		new->pred->succ = new;
	}
}

void list_remove(list_t** list)
{
	assert(list != NULL);
	if (*list == NULL) return;
	if ((*list)->succ == *list) {
		free(*list);
		*list = NULL;
	} else {
		list_t*	pred = (*list)->pred;
		list_t*	succ = (*list)->succ;
		pred->succ = succ;
		succ->pred = pred;
		free(*list);
		*list = succ;
	}
}

