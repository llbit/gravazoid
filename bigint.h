#ifndef ARK_BIGINT_H
#define ARK_BIGINT_H

typedef struct bigint	bigint_t;

struct bigint {
	int*	d;
	int	n;
	int	lim;
};

bigint_t*	new_bigint(unsigned initial);
void		free_bigint(bigint_t** obj);
void		bigint_add(bigint_t* a, bigint_t* b, bigint_t* c);
void		bigint_set(bigint_t* a, unsigned v);
void		print_bigint(bigint_t* a);
void		bigint_to_str(bigint_t* a, char* str, int len);

#endif

