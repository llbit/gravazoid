#ifndef ARK_LIST_H
#define ARK_LIST_H

typedef struct list	list_t;
struct list {
	list_t*		succ;
	list_t*		pred;
	void*		data;
};

// if *list == NULL, create a new list otherwise append
void list_add(list_t** list, void* object);

// remove the first item from the list
void* list_remove(list_t** list);

// remove element containing data from list
int list_remove_item(list_t** list, void* data);

// remove all items from the list
int free_list(list_t** list);

int list_length(list_t* list);

// returns nonzero if list contains data
int list_contains(list_t* list, void* data);

#endif

