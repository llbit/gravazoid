#ifndef ARK_BLOCK_H
#define ARK_BLOCK_H

#include <stdint.h>
#include <stdio.h>
#include "cttf/shape.h"
#include "cttf/triangulate.h"
#include "cttf/list.h"

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
	unsigned	color;
};

extern blocktype_t*	blocktype[NUM_BLOCK_TYPE];

/* Load all block models */
void load_blocks();

/* Free all data used by block models */
void free_blocks();

void free_block(block_t** block);

/* Load a single block type */
blocktype_t* load_block(FILE* fp);

void draw_block(block_t* block, int wireframe);

void draw_block_sides(block_t* block);

/**
 * @return number of blocks in level
 */
int load_level(FILE* in, list_t** blocks);

int load_ram_level(uint8_t *level, list_t **blocks);

#endif
