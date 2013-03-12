#ifndef ARK_BLOCK_H
#define ARK_BLOCK_H

#include "cttf/shape.h"
#include "cttf/triangulate.h"

#define NUM_BLOCK_TYPE (2)

typedef struct block block_t;

struct block {
	int		x;
	int		y;
	int		z;
	shape_t*	shape;
	edge_list_t*	edgelist;
};

block_t* load_block(const char* filename);
void draw_block(block_t* block);

#endif
