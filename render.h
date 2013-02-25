/**
 * Functions to render different geometrical objects for
 * ARKANOID.
 */
#ifndef ARK_RENDER_H
#define ARK_RENDER_H

enum {
	TEX_GRID,
	TEX_BLOB,
	TEX_OVERLAY,
	TEX_VIGNETTE,
	TEX_SHARD,
};

struct worldpixel {
	uint8_t		sinkage;
};

extern struct worldpixel worldmap[WORLDH][WORLDW];

void render_brick(int coords[4][2], int y);
void draw_membrane();

#endif

