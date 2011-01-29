#include "shape.h"

#define CTTF_SHAPE_MIN_VECS (12)
#define CTTF_SHAPE_MIN_SEGS (12)

shape_t* new_shape()
{
	shape_t*	obj;

	obj = malloc(sizeof(*obj));
	obj->vectors = malloc(sizeof(vetor_t)*CTTF_SHAPE_MIN_VECS);
	obj->nvec = 0;
	obj->maxvec = CTTF_SHAPE_MIN_VECS;
	obj->seg = malloc(sizeof(int)*CTTF_SHAPE_MIN_SEGS*2);
	obj->nseg = 0;
	obj->maxseg = CTTF_SHAPE_MIN_SEGS;

	return obj;
}

void free_shape(shape_t** shape)
{
	assert(shape != NULL);
	if (*shape == NULL)
		return;
	
	if ((*shape)->vectors)
		free((*shape)->vectors);
	(*shape)->vectors = NULL;

	if ((*shape)->seg)
		free((*shape)->seg);
	(*shape)->seg = NULL;

	*shape = NULL;
}

void shape_add_vec(shape_t* shape, float x, float y)
{
	// assert consistency
	assert(shape->nvec <= shape->maxvec);

	if (shape->nvec == shape->maxvec) {
		shape->maxvec *= 2;
		realloc(shape->vec, sizeof(vector_t)*shape->maxvec);
	}
	shape->vec[shape->nvec].x = x;
	shape->vec[shape->nvec].y = y;
	shape->nvec += 1;
}

void shape_add_seg(shape_t* shape, int n, int m)
{
	// assert consistency
	assert(shape->nseg <= shape->maxseg);

	if (shape->nseg == shape->maxseg) {
		shape->maxseg *= 2;
		realloc(shape->seg, sizeof(vector_t)*shape->maxseg*2);
	}
	shape->seg[shape->nseg*2] = n;
	shape->seg[shape->nseg*2 + 1] = m;
	shape->nseg += 1;
}

