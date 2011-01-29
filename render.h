/**
 * Functions to render different geometrical objects for
 * ARKANOID.
 */
#ifndef ARK_RENDER_H
#define ARK_RENDER_H

#include "cttf/shape.h"

void render_shape(shape_t* shape, float x, float y);
void render_brick(int coords[4][2], int y);

#endif

