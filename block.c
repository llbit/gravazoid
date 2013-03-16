#include <SDL/SDL_opengl.h>
#include <stdlib.h>

#include <stdlib.h>

#include "ark.h"
#include "block.h"

#define DEPTH (15.f)

blocktype_t*	blocktype[NUM_BLOCK_TYPE];

void load_blocks()
{
	for (int i = 0; i < NUM_BLOCK_TYPE; ++i) {
		char filename[128];
		snprintf(filename, 128, "blocks/block%d.shape", i);
		blocktype[i] = load_block(filename);
	}
}

void free_blocks()
{
	for (int i = 0; i < NUM_BLOCK_TYPE; ++i) {
		free_shape(&blocktype[i]->shape);
		free_edgelist(&blocktype[i]->edgelist);
		blocktype[i] = NULL;
	}
}

void free_block(block_t** block)
{
	if (block == NULL) return;

	if (*block != NULL) {
		free(*block);
	}
	*block = NULL;
}

blocktype_t* load_block(const char* filename)
{
	FILE*		file = fopen(filename, "rb");

	if (file) {
		blocktype_t*	block = malloc(sizeof(blocktype_t));
		block->shape = load_shape(file);
		fclose(file);
		if (block->shape) {
			block->edgelist = triangulate(block->shape);
		}
		return block;
	} else {
		return NULL;
	}
}

void draw_block_sides(block_t* block)
{
	int		i;
	shape_t*	shape = blocktype[block->type]->shape;

	glBegin(GL_QUADS);
	for (i = 0; i < shape->nseg; ++i) {
		float	x1 = shape->vec[shape->seg[i*2]].x;
		float	y1 = shape->vec[shape->seg[i*2]].y;
		float	x2 = shape->vec[shape->seg[i*2+1]].x;
		float	y2 = shape->vec[shape->seg[i*2+1]].y;

		// normal = (x1, y1, 0) x (x2, y2, -h)
		glNormal3d(-DEPTH*(y2-y1), 0, DEPTH*(x2-x1));
		glVertex3f(x1, -DEPTH, y1);
		glVertex3f(x2, -DEPTH, y2);
		glVertex3f(x2, 0, y2);
		glVertex3f(x1, 0, y1);
	}
	glEnd();
}

void draw_block(block_t* block)
{
	list_t*		p;
	list_t*		h;
	edge_list_t*	edgelist = blocktype[block->type]->edgelist;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(block->x-WORLDW/2-BLOCKSIZE/2,
			HEIGHTSCALE - SINKHEIGHTTOP * block->y,
			block->z-WORLDH/2-BLOCKSIZE/2);
	glScalef(BLOCKSIZE, BLOCKSIZE, BLOCKSIZE);

	glBegin(GL_TRIANGLES);
	p = h = edgelist->faces;
	if (p)
	do {
		face_t*	face = p->data;
		edge_t*	edge = face->outer_component;
		p = p->succ;

		if (!face->is_inside || edge == NULL)
			continue;

		if (edge->succ->succ->succ == edge) {

			edge_t*	e = edge;
			glNormal3d(0, 1, 0);
			
			do {
				glVertex3f(e->origin->vec.x, 0,
						e->origin->vec.y);
				e = e->pred;

			} while (e != edge);
		}

	} while (p != h);
	glEnd();

	// draw sides
	draw_block_sides(block);

	glPopMatrix();
}

int load_level(FILE* in, list_t** blocks)
{
	int	nblock;
	int	i;

	// free blocks
	while (*blocks) {
		block_t*	block = list_remove(blocks);
		free_block(&block);
	}

	fscanf(in, "%d", &nblock);
	for (i = 0; i < nblock; ++i) {
		int		type;
		/*int		color;*/
		int		x;
		int		z;
		block_t*	block;

		fscanf(in, "%d, %d, %d", &type, &x, &z);
		block = malloc(sizeof(block_t));
		block->type = type;
		block->color = 0;
		block->x = x;
		block->y = 100;
		block->z = z;
		list_add(blocks, block);
	}

	return nblock;
}

