#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "ark.h"
#include "render.h"

struct worldpixel worldmap[WORLDH][WORLDW];

void draw_membrane_part(int x0, int x1, int z0, int z1)
{
	glTexCoord2d(x0 * GRIDSCALE, z0 * GRIDSCALE);
	glVertex3d(x0, 0, z0);

	glTexCoord2d(x0 * GRIDSCALE, z1 * GRIDSCALE);
	glVertex3d(x0, 0, z1);

	glTexCoord2d(x1 * GRIDSCALE, z1 * GRIDSCALE);
	glVertex3d(x1, 0, z1);

	glTexCoord2d(x1 * GRIDSCALE, z0 * GRIDSCALE);
	glVertex3d(x1, 0, z0);

}

void draw_membrane()
{
	int x, y;
	int ox, oz;

	glBindTexture(GL_TEXTURE_2D, TEX_GRID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glDepthMask(GL_TRUE);
	glDisable(GL_CULL_FACE);
	glColor3ub(0x62, 0xBC, 0xE8);

	glBegin(GL_QUADS);

	ox = -WORLDW / 2;
	oz = -WORLDH / 2;
	draw_membrane_part(ox, ox + WORLDW, oz, oz - BORDER);

	ox = -WORLDW / 2;
	oz = WORLDH / 2;
	draw_membrane_part(ox, ox + WORLDW, oz, oz + BORDER);

	ox = -BORDER - WORLDW / 2;
	oz = -BORDER - WORLDH / 2;
	draw_membrane_part(ox, ox + BORDER, oz, oz + 2*BORDER + WORLDH);

	ox =  BORDER + WORLDW / 2;
	oz = -BORDER - WORLDH / 2;
	draw_membrane_part(ox, ox - BORDER, oz, oz + 2*BORDER + WORLDH);

	glEnd();

	glEnable(GL_CULL_FACE);
	for(y = 0; y < WORLDH; y += MEMBRANESTEP) {
		int z0 = y - WORLDH/2;
		int z1 = y + MEMBRANESTEP - WORLDH/2;
		int xmax = WORLDW/2;

		glBegin(GL_QUAD_STRIP);
		for(x = 0; x < WORLDW; x += MEMBRANESTEP) {
			int x0 = x - WORLDW/2;

			glTexCoord2d(x0 * GRIDSCALE, z0 * GRIDSCALE);
			glVertex3d(x0, -SINKHEIGHT * worldmap[y][x].sinkage, z0);
			glTexCoord2d(x0 * GRIDSCALE, z1 * GRIDSCALE);
			if(y + MEMBRANESTEP >= WORLDH) {
				glVertex3d(x0, 0, z1);
			} else {
				glVertex3d(x0, -SINKHEIGHT * worldmap[y + MEMBRANESTEP][x].sinkage, z1);
			}
		}
		glTexCoord2d(xmax * GRIDSCALE, z0 * GRIDSCALE);
		glVertex3d(xmax, 0, z0);
		glTexCoord2d(xmax * GRIDSCALE, z1 * GRIDSCALE);
		glVertex3d(xmax, 0, z1);
		glEnd();
	}

#if 1
	glBindTexture(GL_TEXTURE_2D, TEX_VIGNETTE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);

	glColor3d(1, 1, 1);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glBegin(GL_QUADS);
	glTexCoord2d(0, 0);
	glVertex3d(-VIGNETTE_BRDR - WORLDW / 2, 0, -VIGNETTE_BRDR - WORLDH / 2);
	glTexCoord2d(0, 1);
	glVertex3d(-VIGNETTE_BRDR - WORLDW / 2, 0, +VIGNETTE_BRDR + WORLDH / 2);
	glTexCoord2d(1, 1);
	glVertex3d(+VIGNETTE_BRDR + WORLDW / 2, 0, +VIGNETTE_BRDR + WORLDH / 2);
	glTexCoord2d(1, 0);
	glVertex3d(+VIGNETTE_BRDR + WORLDW / 2, 0, -VIGNETTE_BRDR - WORLDH / 2);
	glEnd();

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
#endif

}

