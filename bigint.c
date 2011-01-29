#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "bigint.h"

#define BIGINT_LIM (10)
#define BIGINT_RADIX (1000000)

bigint_t*	new_bigint(unsigned initial)
{
	bigint_t*	obj;

	obj = malloc(sizeof(*obj));
	obj->d = malloc(sizeof(int) * BIGINT_LIM);
	obj->d[0] = 0;
	obj->n = 0;
	obj->lim = BIGINT_LIM;
	int i;
	for (i = 0; i < obj->n+1; ++i)
		obj->d[i] = 0;
	for (i = 0; initial > 0; ++i) {
		int rem = initial % BIGINT_RADIX;
		obj->d[i] = rem;
		initial = initial / BIGINT_RADIX;
		obj->n = i;
	}
	return obj;
}

void	free_bigint(bigint_t** obj)
{
	assert(obj != NULL);
	if (*obj == NULL) return;

	if ((*obj)->d != NULL)
		free((*obj)->d);
	(*obj)->d = NULL;
	
	*obj = NULL;
}

// extend the bigint so that it fits at least n digits
void	bigint_widen(bigint_t* obj, int n)
{
	assert(obj != NULL);

	if (obj->lim >= n)
		return;

	obj->d = realloc(obj->d, sizeof(int)*n);
	obj->lim = n;
	for (int i = obj->n+1; i < obj->lim; ++i)
		obj->d[i] = 0;
}

void	bigint_set(bigint_t* a, unsigned v)
{
	int i;
	for (i = 0; i < a->n+1; ++i)
		a->d[i] = 0;
	for (i = 0; v > 0; ++i) {
		int rem = v % BIGINT_RADIX;
		a->d[i] = rem;
		v = v / BIGINT_RADIX;
		a->n = i;
	}
}

void	bigint_add(bigint_t* a, bigint_t* b, bigint_t* c)
{
	if (a->n < b->n) {
		bigint_add(b, a, c);
		return;
	}

	bigint_widen(c, a->n+2);

	// a->n is >= b->n
	int i;
	for (i = 0; i < b->n+1; ++i)
		c->d[i] = a->d[i] + b->d[i];

	for (; i < a->n+1; ++i)
		c->d[i] = a->d[i];

	int rem = 0;
	int carry = 0;
	int n = a->n+1;
	for (i = 0; i < n || carry; ++i) {
		c->d[i] += carry;
		rem = c->d[i] % BIGINT_RADIX;
		carry = c->d[i] / BIGINT_RADIX;
		c->d[i] = rem;
		c->n = i;
	}
}

void	print_bigint(bigint_t* a)
{
	assert(a != NULL);
	for (int i = a->n; i >= 0; --i) {
		if (i == a->n)
			printf("%d", a->d[i]);
		else
			printf("%06d", a->d[i]);
	}
	printf("\n");
}

void	bigint_to_str(bigint_t* a, char* str, int len)
{
	assert(a != NULL);
	int off = 0;
	for (int i = a->n; i >= 0 && off < len-1; --i) {
		if (i == a->n)
			off += snprintf(&str[off], len-off,
					"%d", a->d[i]);
		else
			off += snprintf(&str[off], len-off,
					"%06d", a->d[i]);
	}
}

