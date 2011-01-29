#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <err.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include "ark.h"
#include "render.h"

#define INRADIUS 220
#define OUTRADIUS 230
#define EYERADIUS 270
#define BALLRADIUS 6
#define RIDGEHEIGHT 20

#define MEMBRANESTEP 4

#define MAXBRICK 16
#define MAXBALL 16

static void draw_text();

static int running = 1;
static SDL_Surface *screen;

static uint8_t eye_angle = 0;
static int key_dx = 0;

SDL_sem *semaphore;

GLUquadric *ballquad;

int worldmap_valid;

enum {
	TEX_GRID,
	TEX_BLOB
};

struct worldpixel {
	uint8_t		sinkage;
	uint8_t		color;
	uint8_t		height;
} worldmap[WORLDH][WORLDW];

struct brick {
	int		x, y;
	int		color;
} brick[MAXBRICK];
int nbrick;

#define BF_HELD		1

struct ball {
	double		x, y;
	double		dx, dy;
	int		flags;
} ball[MAXBALL];
int nball = 1;

void add_brick(int x, int y, int color) {
	if(nbrick == MAXBRICK) errx(1, "MAXBRICK");
	brick[nbrick].x = x;
	brick[nbrick].y = y;
	brick[nbrick].color = color;
	nbrick++;
}

void videosetup(int fullscreen) {
	Uint32 flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL;
	SDL_Rect **list;
	int w, h;

	if(fullscreen) flags |= SDL_FULLSCREEN;
	list = SDL_ListModes(0, flags | SDL_FULLSCREEN);
	if(!list) {
		errx(1, "No usable video modes found");
	} else if(list == (SDL_Rect **) -1) {
		w = 640;
		h = 400;
	} else {
		w = list[0]->w;
		h = list[0]->h;
	}
	if(w < WORLDW || h < WORLDH) errx(1, "Largest video mode is too small");

	screen = SDL_SetVideoMode(w, h, 0, flags);
	if(!screen) errx(1, "Failed to set video mode");
}

void glsetup() {
	uint8_t gridtexture[16][16];
	uint8_t blobtexture[BLOBSIZE][BLOBSIZE];
	int x, y;

	for(y = 0; y < 16; y++) {
		for(x = 0; x < 16; x++) {
			if(y == 8 || x == 8) {
				gridtexture[y][x] = 255;
			} else if(y == 7 || y == 9 || x == 7 || x == 9) {
				gridtexture[y][x] = 128;
			} else {
				gridtexture[y][x] = 0;
			}
		}
	}
	for(y = 0; y < BLOBSIZE; y++) {
		for(x = 0; x < BLOBSIZE; x++) {
			double xdist = (double) (x - BLOBSIZE / 2) / (BLOBSIZE / 2);
			double ydist = (double) (y - BLOBSIZE / 2) / (BLOBSIZE / 2);
			double dist2 = xdist * xdist + ydist * ydist;
			if(dist2 > 1) dist2 = 1;
			blobtexture[y][x] = 128 + 127 * cos(dist2 * M_PI);
		}
	}
	glBindTexture(GL_TEXTURE_2D, TEX_GRID);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE, 16, 16, GL_LUMINANCE, GL_UNSIGNED_BYTE, gridtexture);
	glBindTexture(GL_TEXTURE_2D, TEX_BLOB);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE, BLOBSIZE, BLOBSIZE, GL_LUMINANCE, GL_UNSIGNED_BYTE, blobtexture);

	ballquad = gluNewQuadric();

	add_brick(40, 40, 1);
	add_brick(90, 90, 2);
	add_brick(140, 140, 3);
	add_brick(190, 190, 4);
	add_brick(174, 190, 5);
	add_brick(158, 190, 6);
}

void add_thing(int xpos, int ypos, int color) {
	int x, y;

	for(y = -8; y <= 8; y++) {
		for(x = -8; x <= 8; x++) {
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

void ridge(int angstart, int angend) {
	int i;

	glNormal3d(0, 1, 0);
	glBegin(GL_QUAD_STRIP);
	for(i = angstart; i <= angend; i++) {
		glVertex3d(
			OUTRADIUS * cos(i * M_PI * 2 / 256),
			RIDGEHEIGHT,
			OUTRADIUS * sin(i * M_PI * 2 / 256));
		glVertex3d(
			INRADIUS * cos(i * M_PI * 2 / 256),
			RIDGEHEIGHT,
			INRADIUS * sin(i * M_PI * 2 / 256));
	}
	glEnd();
	glBegin(GL_QUAD_STRIP);
	for(i = angstart; i <= angend; i++) {
		glNormal3d(cos(i * M_PI * 2 / 256), 0, sin(i * M_PI * 2 / 256));
		glVertex3d(
			OUTRADIUS * cos(i * M_PI * 2 / 256),
			0,
			OUTRADIUS * sin(i * M_PI * 2 / 256));
		glVertex3d(
			OUTRADIUS * cos(i * M_PI * 2 / 256),
			RIDGEHEIGHT,
			OUTRADIUS * sin(i * M_PI * 2 / 256));
	}
	glEnd();
	/*glBegin(GL_QUAD_STRIP);
	for(i = angstart; i <= angend; i++) {
		glNormal3d(-cos(i * M_PI * 2 / 256), 0, -sin(i * M_PI * 2 / 256));
		glVertex3d(
			INRADIUS * cos(i * M_PI * 2 / 256),
			RIDGEHEIGHT,
			INRADIUS * sin(i * M_PI * 2 / 256));
		glVertex3d(
			INRADIUS * cos(i * M_PI * 2 / 256),
			0,
			INRADIUS * sin(i * M_PI * 2 / 256));
	}
	glEnd();*/
}

void draw_ball(struct ball *b) {
	double ypos = BALLRADIUS;
	int x = round(b->x), y = round(b->y);

	glPushMatrix();
	if(x >= -WORLDW / 2
	&& x < WORLDW / 2
	&& y >= -WORLDH / 2
	&& y < WORLDH / 2) {
		ypos -= SINKHEIGHT * worldmap[y + WORLDH / 2][x + WORLDW / 2].sinkage;
	}
	glTranslated(b->x, ypos, b->y);
	gluSphere(ballquad, BALLRADIUS, 8, 8);
	glPopMatrix();
}

void drawmembrane() {
	int x, y;

	glBindTexture(GL_TEXTURE_2D, TEX_GRID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glColor3d(.1, .1, .2);
	glDisable(GL_CULL_FACE);
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
	glColor3d(.2, .2, .4);
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
}

void drawframe() {
	int x, y, i, j;
	GLfloat light[4];
	uint8_t heightmap[WORLDH][WORLDW];

	if(!worldmap_valid) {
		memset(worldmap, 0, sizeof(worldmap));
		for(i = 0; i < nbrick; i++) {
			add_thing(brick[i].x, brick[i].y, brick[i].color);
		}

		glViewport(0, 0, WORLDW, WORLDH);
		glClear(GL_COLOR_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, WORLDW, 0, WORLDH, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glDisable(GL_FOG);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glBindTexture(GL_TEXTURE_2D, TEX_BLOB);
		glColor3d(.2, .2, .2);
		glBegin(GL_QUADS);
		for(i = 0; i < nbrick; i++) {
			x = brick[i].x;
			y = brick[i].y;
			glTexCoord2d(0, 0);
			glVertex3d(x - BLOBSIZE / 2, y - BLOBSIZE / 2, 0);
			glTexCoord2d(1, 0);
			glVertex3d(x + BLOBSIZE / 2, y - BLOBSIZE / 2, 0);
			glTexCoord2d(1, 1);
			glVertex3d(x + BLOBSIZE / 2, y + BLOBSIZE / 2, 0);
			glTexCoord2d(0, 1);
			glVertex3d(x - BLOBSIZE / 2, y + BLOBSIZE / 2, 0);
		}
		glEnd();
		//return;

		glReadPixels(0, 0, WORLDW, WORLDH, GL_LUMINANCE, GL_UNSIGNED_BYTE, heightmap);
		for(y = 0; y < WORLDH; y++) {
			for(x = 0; x < WORLDW; x++) {
				worldmap[y][x].sinkage = (heightmap[y][x] * heightmap[y][x]) >> 8;
			}
		}
		worldmap_valid = 1;

		glViewport(0, 0, screen->w, screen->h);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(80, screen->w / screen->h, 1, 800);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(
		EYERADIUS * sin(eye_angle * M_PI * 2 / 256), 250, EYERADIUS * cos(eye_angle * M_PI * 2 / 256),
		0, -100, 0,
		0, 1, 0);

	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_FOG);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glFogf(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, 200);
	glFogf(GL_FOG_END, 600);

	drawmembrane();
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	//glEnable(GL_LIGHT1);
	light[0] = 150 * sin(eye_angle * M_PI * 2 / 256);
	light[1] = 180;
	light[2] = 150 * cos(eye_angle * M_PI * 2 / 256);
	light[3] = 1;
	glLightfv(GL_LIGHT0, GL_POSITION, light);
	light[0] = 50;
	light[1] = 130;
	light[2] = -100;
	light[3] = 1;
	glLightfv(GL_LIGHT1, GL_POSITION, light);
	glEnable(GL_NORMALIZE);
	for(i = 0; i < nbrick; i++) {
		struct brick *b = &brick[i];
		int coords[4][3];

		light[0] = (b->color & 1)? 1 : 0;
		light[1] = (b->color & 2)? 1 : 0;
		light[2] = (b->color & 4)? 1 : 0;
		light[3] = 1;
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, light);
		light[0] = 1;
		light[1] = 1;
		light[2] = 1;
		light[3] = 1;
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, light);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 40);

		coords[0][0] = b->x - 8 - WORLDW / 2;
		coords[0][1] = worldmap[b->y - 8][b->x - 8].sinkage;
		coords[0][2] = b->y - 8 - WORLDH / 2;
		coords[1][0] = b->x - 8 - WORLDW / 2;
		coords[1][1] = worldmap[b->y + 8][b->x - 8].sinkage;
		coords[1][2] = b->y + 8 - WORLDH / 2;
		coords[2][0] = b->x + 8 - WORLDW / 2;
		coords[2][1] = worldmap[b->y + 8][b->x + 8].sinkage;
		coords[2][2] = b->y + 8 - WORLDH / 2;
		coords[3][0] = b->x + 8 - WORLDW / 2;
		coords[3][1] = worldmap[b->y - 8][b->x + 8].sinkage;
		coords[3][2] = b->y - 8 - WORLDH / 2;

		glBegin(GL_QUADS);
		glNormal3d(0, 1, 0);
		glVertex3d(coords[0][0], HEIGHTSCALE - SINKHEIGHTTOP * coords[0][1], coords[0][2]);
		glVertex3d(coords[1][0], HEIGHTSCALE - SINKHEIGHTTOP * coords[1][1], coords[1][2]);
		glVertex3d(coords[2][0], HEIGHTSCALE - SINKHEIGHTTOP * coords[2][1], coords[2][2]);
		glVertex3d(coords[3][0], HEIGHTSCALE - SINKHEIGHTTOP * coords[3][1], coords[3][2]);
		for(j = 0; j < 4; j++) {
			int k = (j + 1) % 4;
			glNormal3d(coords[k][2] - coords[j][2], 0, coords[k][0] - coords[j][0]);
			glVertex3d(coords[j][0], HEIGHTSCALE - SINKHEIGHTTOP * coords[j][1], coords[j][2]);
			glVertex3d(coords[j][0], -100, coords[j][2]);
			glVertex3d(coords[k][0], -100, coords[k][2]);
			glVertex3d(coords[k][0], HEIGHTSCALE - SINKHEIGHTTOP * coords[k][1], coords[k][2]);
		}
		glEnd();
	}
	/*
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
				light[0] = 1;
				light[1] = 1;
				light[2] = 1;
				light[3] = 1;
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, light);
				glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 40);

				// top
				glNormal3d(
					-(worldmap[y][x - 1].sinkage - worldmap[y][x + 1].sinkage) * NORMSCALE,
					1,
					(worldmap[y - 1][x].sinkage - worldmap[y + 1][x].sinkage) * NORMSCALE);
				glVertex3d(xstart, -SINKHEIGHTTOP * worldmap[y][x].sinkage + wp->height * HEIGHTSCALE, ystart);
				glNormal3d(
					-(worldmap[y + 1][x - 1].sinkage - worldmap[y + 1][x + 1].sinkage) * NORMSCALE,
					1,
					(worldmap[y + 1 - 1][x].sinkage - worldmap[y + 1 + 1][x].sinkage) * NORMSCALE);
				glVertex3d(xstart, -SINKHEIGHTTOP * worldmap[y + 1][x].sinkage + wp->height * HEIGHTSCALE, ystart + 1);
				glNormal3d(
					-(worldmap[y + 1][x + 1 - 1].sinkage - worldmap[y + 1][x + 1 + 1].sinkage) * NORMSCALE,
					1,
					(worldmap[y + 1 - 1][x + 1].sinkage - worldmap[y + 1 + 1][x + 1].sinkage) * NORMSCALE);
				glVertex3d(xstart + 1, -SINKHEIGHTTOP * worldmap[y + 1][x + 1].sinkage + wp->height * HEIGHTSCALE, ystart + 1);
				glNormal3d(
					-(worldmap[y][x + 1 - 1].sinkage - worldmap[y][x + 1 + 1].sinkage) * NORMSCALE,
					1,
					(worldmap[y - 1][x + 1].sinkage - worldmap[y + 1][x + 1].sinkage) * NORMSCALE);
				glVertex3d(xstart + 1, -SINKHEIGHTTOP * worldmap[y][x + 1].sinkage + wp->height * HEIGHTSCALE, ystart);

				if(!worldmap[y + 1][x].height) {
					// front
					glNormal3d(0, 0, -1);
					glVertex3d(xstart, -SINKHEIGHTTOP * worldmap[y + 1][x].sinkage + wp->height * HEIGHTSCALE, ystart + 1);
					glVertex3d(xstart + 1, -SINKHEIGHTTOP * worldmap[y + 1][x + 1].sinkage + wp->height * HEIGHTSCALE, ystart + 1);
					glVertex3d(xstart + 1, -SINKHEIGHT * worldmap[y + 1][x + 1].sinkage, ystart + 1);
					glVertex3d(xstart, -SINKHEIGHT * worldmap[y + 1][x].sinkage, ystart + 1);
				}
				if(!worldmap[y - 1][x].height) {
					// back
					glNormal3d(0, 0, 1);
					glVertex3d(xstart, -SINKHEIGHTTOP * worldmap[y][x].sinkage + wp->height * HEIGHTSCALE, ystart);
					glVertex3d(xstart + 1, -SINKHEIGHTTOP * worldmap[y][x + 1].sinkage + wp->height * HEIGHTSCALE, ystart);
					glVertex3d(xstart + 1, -SINKHEIGHT * worldmap[y][x + 1].sinkage, ystart);
					glVertex3d(xstart, -SINKHEIGHT * worldmap[y][x].sinkage, ystart);
				}
				if(!worldmap[y][x + 1].height) {
					// right
					glNormal3d(1, 0, 0);
					glVertex3d(xstart + 1, -SINKHEIGHTTOP * worldmap[y][x + 1].sinkage + wp->height * HEIGHTSCALE, ystart);
					glVertex3d(xstart + 1, -SINKHEIGHTTOP * worldmap[y + 1][x + 1].sinkage + wp->height * HEIGHTSCALE, ystart + 1);
					glVertex3d(xstart + 1, -SINKHEIGHT * worldmap[y + 1][x + 1].sinkage, ystart + 1);
					glVertex3d(xstart + 1, -SINKHEIGHT * worldmap[y][x + 1].sinkage, ystart);
				}
				if(!worldmap[y][x - 1].height) {
					// left
					glNormal3d(-1, 0, 0);
					glVertex3d(xstart, -SINKHEIGHTTOP * worldmap[y][x].sinkage + wp->height * HEIGHTSCALE, ystart);
					glVertex3d(xstart, -SINKHEIGHTTOP * worldmap[y + 1][x].sinkage + wp->height * HEIGHTSCALE, ystart + 1);
					glVertex3d(xstart, -SINKHEIGHT * worldmap[y + 1][x].sinkage, ystart + 1);
					glVertex3d(xstart, -SINKHEIGHT * worldmap[y][x].sinkage, ystart);
				}
			}
		}
	}
	glEnd();*/

	light[0] = .5;
	light[1] = .5;
	light[2] = .7;
	light[3] = 1;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, light);
	ridge(-eye_angle - 10 + 64, -eye_angle + 10 + 64);

	light[0] = 1;
	light[1] = .6;
	light[2] = .4;
	light[3] = 1;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, light);

	for(i = 0; i < nball; i++) {
		draw_ball(&ball[i]);
	}
}

void draw_text()
{
	shape_t*	shape = new_shape();
	shape_add_vec(shape, 100.f, 100.f);
	shape_add_vec(shape, 200.f, 100.f);
	shape_add_vec(shape, 200.f, 100.f);
	shape_add_vec(shape, 200.f, 0.f);
	shape_add_seg(shape, 0, 1);
	shape_add_seg(shape, 2, 3);
	render_shape(shape);
}

void handle_key(SDLKey key, int down) {
	int i;

	switch(key) {
		case SDLK_q:
			running = 0;
			break;
		case SDLK_h:
			if(down) {
				key_dx = -1;
			} else {
				if(key_dx == -1) key_dx = 0;
			}
			break;
		case SDLK_l:
			if(down) {
				key_dx = 1;
			} else {
				if(key_dx == 1) key_dx = 0;
			}
			break;
		case SDLK_SPACE:
			for(i = 0; i < nball; i++) {
				if(ball[i].flags & BF_HELD) {
					ball[i].flags &= ~BF_HELD;
					ball[i].dx = -ball[i].x / (INRADIUS - BALLRADIUS);
					ball[i].dy = -ball[i].y / (INRADIUS - BALLRADIUS);
				}
			}
			break;
		default:
			break;
	}
}

void mirror(double *x1, double *y1, double x2, double y2) {
	double len2 = hypot(x2, y2);
	double dot;
	
	x2 /= len2;
	y2 /= len2;

	dot = sqrt((*x1 * x2) + (*y1 * y2));	// |v1| * cos(alpha)
	x2 = x2 * dot;				// v1 projected on v2
	y2 = y2 * dot;
	*x1 -= x2 * 2;
	*y1 -= y2 * 2;
}

void physics() {
	int i, j;
	double r;

	eye_angle += key_dx;

	for(i = 0; i < nball; i++) {
		if(ball[i].flags & BF_HELD) {
			ball[i].x = (INRADIUS - BALLRADIUS) * cos((64 - eye_angle) * M_PI * 2 / 256);
			ball[i].y = (INRADIUS - BALLRADIUS) * sin((64 - eye_angle) * M_PI * 2 / 256);
			ball[i].dx = ball[i].dy = 0;
		} else {
			for(j = 0; j < nbrick; j++) {
				double xdiff = (brick[j].x - WORLDW / 2) - ball[i].x;
				double ydiff = (brick[j].y - WORLDH / 2) - ball[i].y;
				double dist2 = xdiff * xdiff + ydiff * ydiff;
				if(dist2 && dist2 < 100) {
					ball[i].dx += xdiff / dist2;
					ball[i].dy += ydiff / dist2;
				}
			}
			if(ball[i].dx || ball[i].dy) {
				double size = hypot(ball[i].dx, ball[i].dy);
				if(size < 1) {
					ball[i].dx /= size;
					ball[i].dy /= size;
				}
			}
			ball[i].x += ball[i].dx;
			ball[i].y += ball[i].dy;
			r = hypot(ball[i].x, ball[i].y);
			if(r > INRADIUS) {
				mirror(&ball[i].dx, &ball[i].dy, ball[i].x, ball[i].y);
			}
		}
	}
}

int main() {
	Uint32 millis;

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	atexit(SDL_Quit);

	videosetup(0);
	glsetup();

	SDL_ShowCursor(SDL_DISABLE);

	ball[0].flags = BF_HELD;

	physics();
	millis = SDL_GetTicks();

	while(running) {
		SDL_Event event;
		Uint32 now = SDL_GetTicks();

		while(now > millis + 10) {
			physics();
			millis += 10;
		}

		drawframe();

		SDL_GL_SwapBuffers();
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					handle_key(event.key.keysym.sym, 1);
					break;
				case SDL_KEYUP:
					handle_key(event.key.keysym.sym, 0);
					break;
				default:
					break;
			}
		}
	}

	return 0;
}
