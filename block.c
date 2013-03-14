#include <SDL/SDL_opengl.h>
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

static void draw_sides(block_t* block)
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
	draw_sides(block);

	// draw wireframe
	glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glColor3f(0, 0, 0);
	draw_sides(block);
	glPopAttrib();

	glPopMatrix();
}

