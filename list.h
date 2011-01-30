#ifndef ARK_LIST_H
#define ARK_LIST_H

typedef struct list	list_t;
struct list {
	list_t*		succ;
	list_t*		pred;
	void*		data;
};

void list_add(list_t** list, void* object);
void list_remove(list_t** list);

#endif

