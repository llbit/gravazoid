#ifndef CTTF_SHAPE_H
#define CTTF_SHAPE_H

#include <stdio.h>
#include "vector.h"

typedef struct shape	shape_t;

shape_t* new_shape();
void free_shape(shape_t** shape);
void shape_add_vec(shape_t* shape, float x, float y);
void shape_add_seg(shape_t* shape, int n, int m);
shape_t* load_shape(FILE* file);

struct shape {
	vector_t*	vec;
	int		nvec;
	int		maxvec;
	int*		seg;
	int		nseg;
	int		maxseg;
};

#endif

