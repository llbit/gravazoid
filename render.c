#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "ark.h"
#include "render.h"

struct worldpixel worldmap[WORLDH][WORLDW];

void render_brick(int coords[4][2], int y)
{
	//glNormal3d(0, 1, 0);
	glVertex3d(coords[0][0], HEIGHTSCALE - SINKHEIGHTTOP * y, coords[0][1]);
	glVertex3d(coords[1][0], HEIGHTSCALE - SINKHEIGHTTOP * y, coords[1][1]);
	glVertex3d(coords[2][0], HEIGHTSCALE - SINKHEIGHTTOP * y, coords[2][1]);
	glVertex3d(coords[3][0], HEIGHTSCALE - SINKHEIGHTTOP * y, coords[3][1]);
	for(int j = 0; j < 4; j++) {
		int k = (j + 1) % 4;
		//glNormal3d(coords[k][2] - coords[j][2], 0, coords[k][0] - coords[j][0]);
		glVertex3d(coords[j][0], HEIGHTSCALE - SINKHEIGHTTOP * y, coords[j][1]);
		glVertex3d(coords[j][0], -100, coords[j][1]);
		glVertex3d(coords[k][0], -100, coords[k][1]);
		glVertex3d(coords[k][0], HEIGHTSCALE - SINKHEIGHTTOP * y, coords[k][1]);
	}
}

void draw_membrane()
{
	int x, y;

	glBindTexture(GL_TEXTURE_2D, TEX_GRID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glDepthMask(GL_TRUE);
	glDisable(GL_CULL_FACE);
	glColor3ub(0x62, 0xBC, 0xE8);
	
	glBegin(GL_QUADS);

	glTexCoord2d(-BORDER * GRIDSCALE, -BORDER * GRIDSCALE);
	glVertex3d(-BORDER - WORLDW / 2, 0, -BORDER - WORLDH / 2);
	glTexCoord2d(0, -BORDER * GRIDSCALE);
	glVertex3d(-WORLDW / 2, 0, -BORDER - WORLDH / 2);
	glTexCoord2d(0, +(BORDER + WORLDH) * GRIDSCALE);
	glVertex3d(-WORLDW / 2, 0, +BORDER + WORLDH / 2);
	glTexCoord2d(-BORDER * GRIDSCALE, +(BORDER + WORLDH) * GRIDSCALE);
	glVertex3d(-BORDER - WORLDW / 2, 0, +BORDER + WORLDH / 2);

	glTexCoord2d(WORLDW * GRIDSCALE, -BORDER * GRIDSCALE);
	glVertex3d(+BORDER + WORLDW / 2, 0, -BORDER - WORLDH / 2);
	glTexCoord2d(+(WORLDW + BORDER) * GRIDSCALE, -BORDER * GRIDSCALE);
	glVertex3d(+(WORLDW - 1) / 2, 0, -BORDER - WORLDH / 2);
	glTexCoord2d(+(WORLDW + BORDER) * GRIDSCALE, +(BORDER + WORLDH) * GRIDSCALE);
	glVertex3d(+(WORLDW - 1) / 2, 0, +BORDER + WORLDH / 2);
	glTexCoord2d(WORLDW * GRIDSCALE, +(BORDER + WORLDH) * GRIDSCALE);
	glVertex3d(+BORDER + WORLDW / 2, 0, +BORDER + WORLDH / 2);

	glTexCoord2d(0, -BORDER * GRIDSCALE);
	glVertex3d(-WORLDW / 2, 0, -BORDER - WORLDH / 2);
	glTexCoord2d(+WORLDW * GRIDSCALE, -BORDER * GRIDSCALE);
	glVertex3d(+(WORLDW - 1) / 2, 0, -BORDER - WORLDH / 2);
	glTexCoord2d(+WORLDW * GRIDSCALE, 0);
	glVertex3d(+(WORLDW - 1) / 2, 0, -WORLDH / 2);
	glTexCoord2d(0, 0);
	glVertex3d(-WORLDW / 2, 0, -WORLDH / 2);

	glTexCoord2d(0, +(WORLDH + BORDER) * GRIDSCALE);
	glVertex3d(-WORLDW / 2, 0, +BORDER + WORLDH / 2);
	glTexCoord2d(+WORLDW * GRIDSCALE, +(WORLDH + BORDER) * GRIDSCALE);
	glVertex3d(+(WORLDW - 1) / 2, 0, +BORDER + WORLDH / 2);
	glTexCoord2d(+WORLDW * GRIDSCALE, +WORLDH * GRIDSCALE);
	glVertex3d(+(WORLDW - 1) / 2, 0, +(WORLDH - 1) / 2);
	glTexCoord2d(0, +WORLDH * GRIDSCALE);
	glVertex3d(-WORLDW / 2, 0, +(WORLDH - 1) / 2);

	glEnd();

	glEnable(GL_CULL_FACE);
	for(y = 0; y < WORLDH; y += MEMBRANESTEP) {
		glBegin(GL_QUAD_STRIP);
		for(x = 0; x < WORLDW; x += MEMBRANESTEP) {
			glTexCoord2d(x * GRIDSCALE, y * GRIDSCALE);
			glVertex3d(x - WORLDW / 2, -SINKHEIGHT * worldmap[y][x].sinkage, y - WORLDH / 2);
			glTexCoord2d(x * GRIDSCALE, (y + MEMBRANESTEP) * GRIDSCALE);
			if(y + MEMBRANESTEP >= WORLDH) {
				glVertex3d(x - WORLDW / 2, 0, y + MEMBRANESTEP - WORLDH / 2);
			} else {
				glVertex3d(x - WORLDW / 2, -SINKHEIGHT * worldmap[y + MEMBRANESTEP][x].sinkage, y + MEMBRANESTEP - WORLDH / 2);
			}
		}
		glTexCoord2d(WORLDW * GRIDSCALE, y * GRIDSCALE);
		glVertex3d(WORLDW / 2, 0, y - WORLDH / 2);
		glTexCoord2d(WORLDW * GRIDSCALE, (y + MEMBRANESTEP) * GRIDSCALE);
		glVertex3d(WORLDW / 2, 0, y + MEMBRANESTEP - WORLDH / 2);
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

