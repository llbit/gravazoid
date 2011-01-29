#include <SDL/SDL_opengl.h>
#include "ark.h"
#include "render.h"

void render_shape(shape_t* shape, float x, float y)
{
	glPushAttrib(GL_VIEWPORT_BIT | GL_TRANSFORM_BIT);
	glViewport(0, 0, WORLDW, WORLDH);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, WORLDW, 0, WORLDH, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(x, y, 0.f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glColor3f(1.f, 1.f, 1.f);

	glBegin(GL_LINES);

	for (int i = 0; i < shape->nseg; ++i) {
		int	n;
		int	m;

		n = shape->seg[i*2];
		m = shape->seg[i*2+1];
		glVertex3f(shape->vec[n].x, shape->vec[n].y, 0.f);
		glVertex3f(shape->vec[m].x, shape->vec[m].y, 0.f);
	}

	glEnd();
	glPopAttrib();
}
