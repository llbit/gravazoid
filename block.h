#ifndef ARK_BLOCK_H
#define ARK_BLOCK_H

#include "cttf/shape.h"
#include "cttf/triangulate.h"

#define NUM_BLOCK_TYPE (2)

typedef struct block block_t;
typedef struct blocktype blocktype_t;

struct blocktype {
	shape_t*	shape;
	edge_list_t*	edgelist;
};

struct block {
	int		x;
	int		y;
	int		z;
	unsigned	type;
};

extern blocktype_t*	blocktype[NUM_BLOCK_TYPE];

/* Load all block models */
void load_blocks();

/* Free all data used by block models */
void free_blocks();

/* Load a single block type */
blocktype_t* load_block(const char* filename);

void draw_block(block_t* block);

#endif
