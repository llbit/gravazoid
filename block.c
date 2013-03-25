#include <SDL/SDL_opengl.h>
#include <stdint.h>
#include <stdlib.h>

#include "ark.h"
#include "block.h"
#include "render.h"
#include "memfile.h"

#include "data_blocks.h"

#define DEPTH (15.f)

blocktype_t*	blocktype[NUM_BLOCK_TYPE];

void load_blocks()
{
	blocktype[0] = load_block(mem_to_file(data_block0, sizeof(data_block0)));
	blocktype[1] = load_block(mem_to_file(data_block1, sizeof(data_block1)));
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

blocktype_t* load_block(FILE* fp)
{
	if (fp) {
		blocktype_t*	block = malloc(sizeof(blocktype_t));
		block->shape = load_shape(fp);
		fclose(fp);
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

void draw_block(block_t* block, int wireframe)
{
	list_t*		p;
	list_t*		h;
	edge_list_t*	edgelist = blocktype[block->type]->edgelist;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(block->x-WORLDW/2-BLOCKSIZE/2,
			HEIGHTSCALE - SINKHEIGHTTOP * (block->y + worldmap[block->z][block->x].sinkage),
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


	if (wireframe) {
		// draw wireframe
		glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_LIGHTING);
		glDisable(GL_FOG);
		glColor3f(0, 0, 0);
		draw_block_sides(block);
		glPopAttrib();
	}

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

int load_ram_level(uint8_t *src, list_t **blocks) {
	int nblock, i;

	// free blocks
	while (*blocks) {
		block_t *block = list_remove(blocks);
		free_block(&block);
	}

	nblock = *src++;
	nblock |= (*src++) << 8;
	for(i = 0; i < nblock; ++i) {
		block_t *block = malloc(sizeof(block_t));
		block->type = *src++;
		block->color = *src++;
		block->x = *src++;
		block->y = 100;
		block->z = *src++;
		list_add(blocks, block);
	}

	return nblock;
}
