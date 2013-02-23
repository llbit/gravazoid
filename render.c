#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "ark.h"
#include "render.h"

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
