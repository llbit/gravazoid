#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <err.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

static int running = 1;
static SDL_Surface *screen;

#define WORLDH 256
#define WORLDW 256

#define GRIDSCALE .08
#define HEIGHTSCALE 20
#define NORMSCALE 1.0
#define BLOBSIZE 32
#define SINKHEIGHT .1

static uint8_t eye_angle = 0;

SDL_sem *semaphore;

enum {
	TEX_GRID,
	TEX_BLOB
};

struct worldpixel {
	uint8_t		sinkage;
	uint8_t		color;
	uint8_t		height;
} worldmap[WORLDH][WORLDW];

void glsetup() {
	uint8_t gridtexture[16][16];
	uint8_t blobtexture[BLOBSIZE][BLOBSIZE];
	int x, y;

	for(y = 0; y < 16; y++) {
		for(x = 0; x < 16; x++) {
			gridtexture[y][x] = (y && x)? 0 : 255;
		}
	}
	for(y = 0; y < BLOBSIZE; y++) {
		for(x = 0; x < BLOBSIZE; x++) {
			double xdist = (double) (x - BLOBSIZE / 2) / (BLOBSIZE / 2);
			double ydist = (double) (y - BLOBSIZE / 2) / (BLOBSIZE / 2);
			double dist2 = xdist * xdist + ydist * ydist;
			blobtexture[y][x] = 255.0 / (dist2 * 250 + 1);
		}
	}
	glBindTexture(GL_TEXTURE_2D, TEX_GRID);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE, 16, 16, GL_LUMINANCE, GL_UNSIGNED_BYTE, gridtexture);
	glBindTexture(GL_TEXTURE_2D, TEX_BLOB);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE, BLOBSIZE, BLOBSIZE, GL_LUMINANCE, GL_UNSIGNED_BYTE, blobtexture);
}

void add_thing(int xpos, int ypos, int color) {
	int x, y;

	for(y = 0; y < 16; y++) {
		for(x = 0; x < 16; x++) {
			worldmap[ypos + y][xpos + x].color = color;
			worldmap[ypos + y][xpos + x].height = 1;
		}
	}
}

/*void cross(GLdouble *dest, GLdouble *a, GLdouble *b) {
	dest[0] = a[1] * b[2] - a[2] * b[1];
	dest[1] = a[2] * b[0] - a[0] * b[2];
	dest[2] = a[0] * b[1] - a[1] * b[0];
}*/

void drawframe() {
	int x, y;
	GLfloat light[4];
	uint8_t heightmap[WORLDH][WORLDW];

	memset(worldmap, 0, sizeof(worldmap));
	add_thing(40, 40, 1);
	add_thing(90, 90, 2);
	add_thing(140, 140, 3);
	add_thing(190, 190, 4);
	add_thing(174, 190, 5);
	add_thing(158, 190, 6);

	glViewport(0, 0, WORLDW, WORLDH);
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, WORLDW, 0, WORLDH, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBindTexture(GL_TEXTURE_2D, TEX_BLOB);
	glColor3d(.05, .05, .05);
	glBegin(GL_QUADS);
	for(y = 0; y < WORLDH; y++) {
		for(x = 0; x < WORLDW; x++) {
			if(worldmap[y][x].height) {
				glTexCoord2d(0, 0);
				glVertex3d(x - BLOBSIZE / 2, y - BLOBSIZE / 2, 0);
				glTexCoord2d(1, 0);
				glVertex3d(x + BLOBSIZE / 2, y - BLOBSIZE / 2, 0);
				glTexCoord2d(1, 1);
				glVertex3d(x + BLOBSIZE / 2, y + BLOBSIZE / 2, 0);
				glTexCoord2d(0, 1);
				glVertex3d(x - BLOBSIZE / 2, y + BLOBSIZE / 2, 0);
			}
		}
	}
	glEnd();

	//return;
	glReadPixels(0, 0, WORLDW, WORLDH, GL_LUMINANCE, GL_UNSIGNED_BYTE, heightmap);
	for(y = 0; y < WORLDH; y++) {
		for(x = 0; x < WORLDW; x++) {
			worldmap[y][x].sinkage = heightmap[y][x];
		}
	}

	glViewport(0, 0, screen->w, screen->h);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90, screen->w / screen->h, 1, 800);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(
		150 * sin(eye_angle * M_PI * 2 / 256), 150, 150 * cos(eye_angle * M_PI * 2 / 256),
		0, 0, 0,
		0, 1, 0);

	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, TEX_GRID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glColor3d(1, 1, 1);
	for(y = 0; y < WORLDH - 1; y++) {
		glBegin(GL_QUAD_STRIP);
		for(x = 0; x < WORLDW; x++) {
			glTexCoord2d(x * GRIDSCALE, y * GRIDSCALE);
			glVertex3d(x - WORLDW / 2, -SINKHEIGHT * worldmap[y][x].sinkage, y - WORLDH / 2);
			glTexCoord2d(x * GRIDSCALE, (y + 1) * GRIDSCALE);
			glVertex3d(x - WORLDW / 2, -SINKHEIGHT * worldmap[y + 1][x].sinkage, y + 1 - WORLDH / 2);
		}
		glEnd();
	}
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	light[0] = 100;
	light[1] = 150;
	light[2] = 100;
	light[3] = 1;
	glLightfv(GL_LIGHT0, GL_POSITION, light);
	light[0] = 50;
	light[1] = 130;
	light[2] = -100;
	light[3] = 1;
	glLightfv(GL_LIGHT1, GL_POSITION, light);
	glEnable(GL_NORMALIZE);
	glBegin(GL_QUADS);
	for(y = 0; y < WORLDH - 1; y++) {
		for(x = 0; x < WORLDW - 1; x++) {
			struct worldpixel *wp = &worldmap[y][x];
			if(wp->height) {
				int xstart, ystart;
				xstart = x - WORLDW / 2;
				ystart = y - WORLDH / 2;
				light[0] = (wp->color & 1)? 1 : 0;
				light[1] = (wp->color & 2)? 1 : 0;
				light[2] = (wp->color & 4)? 1 : 0;
				light[3] = 1;
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, light);

				// top
				glNormal3d(
					-(worldmap[y][x - 1].sinkage - worldmap[y][x + 1].sinkage) * NORMSCALE,
					1,
					(worldmap[y - 1][x].sinkage - worldmap[y + 1][x].sinkage) * NORMSCALE);
				glVertex3d(xstart, -SINKHEIGHT * worldmap[y][x].sinkage + wp->height * HEIGHTSCALE, ystart);
				glNormal3d(
					-(worldmap[y + 1][x - 1].sinkage - worldmap[y + 1][x + 1].sinkage) * NORMSCALE,
					1,
					(worldmap[y + 1 - 1][x].sinkage - worldmap[y + 1 + 1][x].sinkage) * NORMSCALE);
				glVertex3d(xstart, -SINKHEIGHT * worldmap[y + 1][x].sinkage + wp->height * HEIGHTSCALE, ystart + 1);
				glNormal3d(
					-(worldmap[y + 1][x + 1 - 1].sinkage - worldmap[y + 1][x + 1 + 1].sinkage) * NORMSCALE,
					1,
					(worldmap[y + 1 - 1][x + 1].sinkage - worldmap[y + 1 + 1][x + 1].sinkage) * NORMSCALE);
				glVertex3d(xstart + 1, -SINKHEIGHT * worldmap[y + 1][x + 1].sinkage + wp->height * HEIGHTSCALE, ystart + 1);
				glNormal3d(
					-(worldmap[y][x + 1 - 1].sinkage - worldmap[y][x + 1 + 1].sinkage) * NORMSCALE,
					1,
					(worldmap[y - 1][x + 1].sinkage - worldmap[y + 1][x + 1].sinkage) * NORMSCALE);
				glVertex3d(xstart + 1, -SINKHEIGHT * worldmap[y][x + 1].sinkage + wp->height * HEIGHTSCALE, ystart);

				if(!worldmap[y + 1][x].height) {
					// front
					glNormal3d(0, 0, -1);
					glVertex3d(xstart, -SINKHEIGHT * worldmap[y + 1][x].sinkage + wp->height * HEIGHTSCALE, ystart + 1);
					glVertex3d(xstart + 1, -SINKHEIGHT * worldmap[y + 1][x + 1].sinkage + wp->height * HEIGHTSCALE, ystart + 1);
					glVertex3d(xstart + 1, -SINKHEIGHT * worldmap[y + 1][x + 1].sinkage, ystart + 1);
					glVertex3d(xstart, -SINKHEIGHT * worldmap[y + 1][x].sinkage, ystart + 1);
				}
				if(!worldmap[y - 1][x].height) {
					// back
					glNormal3d(0, 0, 1);
					glVertex3d(xstart, -SINKHEIGHT * worldmap[y][x].sinkage + wp->height * HEIGHTSCALE, ystart);
					glVertex3d(xstart + 1, -SINKHEIGHT * worldmap[y][x + 1].sinkage + wp->height * HEIGHTSCALE, ystart);
					glVertex3d(xstart + 1, -SINKHEIGHT * worldmap[y][x + 1].sinkage, ystart);
					glVertex3d(xstart, -SINKHEIGHT * worldmap[y][x].sinkage, ystart);
				}
				if(!worldmap[y][x + 1].height) {
					// right
					glNormal3d(1, 0, 0);
					glVertex3d(xstart + 1, -SINKHEIGHT * worldmap[y][x + 1].sinkage + wp->height * HEIGHTSCALE, ystart);
					glVertex3d(xstart + 1, -SINKHEIGHT * worldmap[y + 1][x + 1].sinkage + wp->height * HEIGHTSCALE, ystart + 1);
					glVertex3d(xstart + 1, -SINKHEIGHT * worldmap[y + 1][x + 1].sinkage, ystart + 1);
					glVertex3d(xstart + 1, -SINKHEIGHT * worldmap[y][x + 1].sinkage, ystart);
				}
				if(!worldmap[y][x - 1].height) {
					// left
					glNormal3d(-1, 0, 0);
					glVertex3d(xstart, -SINKHEIGHT * worldmap[y][x].sinkage + wp->height * HEIGHTSCALE, ystart);
					glVertex3d(xstart, -SINKHEIGHT * worldmap[y + 1][x].sinkage + wp->height * HEIGHTSCALE, ystart + 1);
					glVertex3d(xstart, -SINKHEIGHT * worldmap[y + 1][x].sinkage, ystart + 1);
					glVertex3d(xstart, -SINKHEIGHT * worldmap[y][x].sinkage, ystart);
				}
			}
		}
	}
	glEnd();
}

void handle_key(SDLKey key) {
	switch(key) {
		case SDLK_q:
			running = 0;
			break;
		case SDLK_h:
			eye_angle -= 2;
			break;
		case SDLK_l:
			eye_angle += 2;
			break;
		default:
			break;
	}
}

void physics() {
	eye_angle++;
}

int main() {
	Uint32 millis;

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	atexit(SDL_Quit);

	screen = SDL_SetVideoMode(640, 400, 0, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL);
	if(!screen) errx(1, "SDL_SetVideoMode");

	glsetup();

	millis = SDL_GetTicks();

	while(running) {
		SDL_Event event;
		Uint32 now = SDL_GetTicks();

		while(now > millis + 20) {
			physics();
			millis += 20;
		}

		drawframe();

		SDL_GL_SwapBuffers();
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					handle_key(event.key.keysym.sym);
					break;
				default:
					break;
			}
		}
	}

	return 0;
}
