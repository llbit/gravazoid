#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <err.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include "ark.h"
#include "render.h"
#include "text.h"
#include "bigint.h"

#define INRADIUS 250
#define OUTRADIUS 255
#define EYERADIUS 270
#define BALLRADIUS 4
#define RIDGEHEIGHT 20

#define BALLSPEED 2.1
#define GRAVITY 0.009

#define MEMBRANESTEP 7
#define NBLUR 96

#define MAX_PADDLE_VEL 100

#define MAXBRICK 256
#define MAXBALL 16

static int colors[][3] = {
	{0x65, 0x9D, 0xFD}, // light blue
	{0x63, 0x7E, 0x9A}, // dark blue
	{0x40, 0x4A, 0xE1}, // bright blue
	{0xDA, 0x0D, 0x18}, // bright red
	{0x8F, 0x09, 0x00}, // dark red
	{0xB6, 0xDE, 0xFF}, // pale blue

};

static void draw_text();

static int running = 1;
SDL_Surface *screen;

static int softglow = 0;

static int fps = 0;

static double eye_angle = 0;
static int key_dx = 0;
static int paddle_vel = 0;

int lives;
int gameover = 1;

static vfont_t* font = NULL;

SDL_sem *semaphore;

GLUquadric *ballquad;

static bigint_t* bonus;
static bigint_t* score;

int worldmap_valid;

enum {
	TEX_GRID,
	TEX_BLOB,
	TEX_OVERLAY
};

struct worldpixel {
	uint8_t		sinkage;
} worldmap[WORLDH][WORLDW];

struct brick {
	int		x, y;
	uint8_t		color;
	uint8_t		flags;
} brick[MAXBRICK];
int nbrick;

#define BALLF_HELD		1
#define BALLF_OUTSIDE		2

#define BRICKF_LIVE		1

struct ball {
	double		x, y;
	double		dx, dy;
	int		flags;
	float		xhist[NBLUR], yhist[NBLUR];
} ball[MAXBALL];
int nball;

void add_brick(int x, int y, int color) {
	if(nbrick == MAXBRICK) errx(1, "MAXBRICK");
	brick[nbrick].x = x;
	brick[nbrick].y = y;
	brick[nbrick].color = color;
	brick[nbrick].flags = BRICKF_LIVE;
	nbrick++;
}

void videosetup(int fullscreen) {
	Uint32 flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL;
	SDL_Rect **list;
	int w, h;

	if(fullscreen) flags |= SDL_FULLSCREEN;
	list = SDL_ListModes(0, flags);
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
			if(y == 8 && x == 8) {
				gridtexture[y][x] = 255;
			} else if(y == 8 || x == 8) {
				gridtexture[y][x] = 128;
			} else if(y == 7 || y == 9 || x == 7 || x == 9) {
				gridtexture[y][x] = 64;
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
}

void resetlevel() {
	int x, y;

	srand(0);
	nbrick = 0;
	for(y = 0; y < 12; y++) {
		for(x = 0; x < 18; x++) {
			if(x != 8 && x != 9 && y != 5 && y != 6) {
				add_brick(x * 16 + 24, y * 16 + 32, rand() % 6);
			}
		}
	}

	bigint_set(score, 0);
	lives = 5;

	nball = 1;
	ball[0].flags = BALLF_HELD;
	worldmap_valid = 0;
}

/*void cross(GLdouble *dest, GLdouble *a, GLdouble *b) {
	dest[0] = a[1] * b[2] - a[2] * b[1];
	dest[1] = a[2] * b[0] - a[0] * b[2];
	dest[2] = a[0] * b[1] - a[1] * b[0];
}*/

void ridge(double angstart, double angend) {
	int i;

	glNormal3d(0, 1, 0);
	glBegin(GL_QUAD_STRIP);
	for(i = 0; i < 64; i++) {
		double angle = angstart + (angend - angstart) * i / 64;
		glVertex3d(
			OUTRADIUS * cos(angle * M_PI * 2 / 256),
			RIDGEHEIGHT,
			OUTRADIUS * sin(angle * M_PI * 2 / 256));
		glVertex3d(
			INRADIUS * cos(angle * M_PI * 2 / 256),
			RIDGEHEIGHT,
			INRADIUS * sin(angle * M_PI * 2 / 256));
	}
	glEnd();
	glBegin(GL_QUAD_STRIP);
	for(i = 0; i < 64; i++) {
		double angle = angstart + (angend - angstart) * i / 64;
		glNormal3d(cos(angle * M_PI * 2 / 256), 0, sin(angle * M_PI * 2 / 256));
		glVertex3d(
			OUTRADIUS * cos(angle * M_PI * 2 / 256),
			0,
			OUTRADIUS * sin(angle * M_PI * 2 / 256));
		glVertex3d(
			OUTRADIUS * cos(angle * M_PI * 2 / 256),
			RIDGEHEIGHT,
			OUTRADIUS * sin(angle * M_PI * 2 / 256));
	}
	glEnd();
}

void draw_ball(double bx, double by, int segments) {
	double ypos = BALLRADIUS;
	int x = round(bx), y = round(by);

	glPushMatrix();
	if(x >= -WORLDW / 2
	&& x < WORLDW / 2
	&& y >= -WORLDH / 2
	&& y < WORLDH / 2) {
		ypos -= SINKHEIGHT * worldmap[y + WORLDH / 2][x + WORLDW / 2].sinkage;
	}
	glTranslated(bx, ypos, by);
	gluSphere(ballquad, BALLRADIUS, segments, segments);
	glPopMatrix();
}

void drawmembrane() {
	int x, y;

	glBindTexture(GL_TEXTURE_2D, TEX_GRID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glColor3d(.15, .10, .2);
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

void drawscene(int with_membrane) {
	int i, j;
	GLfloat light[4];

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(80, screen->w / screen->h, 1, 800);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(
		EYERADIUS * sin(eye_angle * M_PI * 2 / 256), 290, EYERADIUS * cos(eye_angle * M_PI * 2 / 256),
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

	if(with_membrane) drawmembrane();
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	light[0] = 150 * sin(eye_angle * M_PI * 2 / 256);
	light[1] = 180;
	light[2] = 150 * cos(eye_angle * M_PI * 2 / 256);
	light[3] = 0;
	glLightfv(GL_LIGHT0, GL_POSITION, light);
	glEnable(GL_NORMALIZE);
	for(i = 0; i < nbrick; i++) {
		struct brick *b = &brick[i];
		if(b->flags & BRICKF_LIVE) {
			int coords[4][3];

			light[0] = colors[b->color][0] / 255.;
			light[1] = colors[b->color][1] / 255.;
			light[2] = colors[b->color][2] / 255.;
			light[3] = 1;
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, light);
			/*light[0] = .5;
			light[1] = .5;
			light[2] = .5;
			light[3] = 1;
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, light);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 40);*/

			coords[0][0] = b->x - 8 - WORLDW / 2;
			coords[0][1] = worldmap[b->y - 8][b->x - 8].sinkage;
			coords[0][2] = b->y - 8 - WORLDH / 2;
			coords[1][0] = b->x - 8 - WORLDW / 2;
			coords[1][1] = worldmap[b->y + 8][b->x - 8].sinkage;
			coords[1][2] = b->y + 7 - WORLDH / 2;
			coords[2][0] = b->x + 7 - WORLDW / 2;
			coords[2][1] = worldmap[b->y + 8][b->x + 8].sinkage;
			coords[2][2] = b->y + 7 - WORLDH / 2;
			coords[3][0] = b->x + 7 - WORLDW / 2;
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

			// draw wirebox
			glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDisable(GL_LIGHTING);
			glDisable(GL_FOG);
			glColor3f(0, 0, 0);
			glBegin(GL_QUADS);
			glVertex3d(coords[0][0], HEIGHTSCALE - SINKHEIGHTTOP * coords[0][1], coords[0][2]);
			glVertex3d(coords[1][0], HEIGHTSCALE - SINKHEIGHTTOP * coords[1][1], coords[1][2]);
			glVertex3d(coords[2][0], HEIGHTSCALE - SINKHEIGHTTOP * coords[2][1], coords[2][2]);
			glVertex3d(coords[3][0], HEIGHTSCALE - SINKHEIGHTTOP * coords[3][1], coords[3][2]);
			for(j = 0; j < 4; j++) {
				int k = (j + 1) % 4;
				glVertex3d(coords[j][0], HEIGHTSCALE - SINKHEIGHTTOP * coords[j][1], coords[j][2]);
				glVertex3d(coords[j][0], -100, coords[j][2]);
				glVertex3d(coords[k][0], -100, coords[k][2]);
				glVertex3d(coords[k][0], HEIGHTSCALE - SINKHEIGHTTOP * coords[k][1], coords[k][2]);
			}
			glEnd();
			glPopAttrib();
		}
	}

	light[0] = .2;
	light[1] = .2;
	light[2] = .2;
	light[3] = 1;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, light);
	ridge(-eye_angle - 10 + 64, -eye_angle + 10 + 64);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	light[0] = 1;
	light[1] = .6;
	light[2] = .4;
	light[3] = 1;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, light);

	for(i = 0; i < nball; i++) {
		draw_ball(ball[i].x, ball[i].y, 7);
	}

	for(j = 0; j < nball; j++) {
		for(i = 0; i < NBLUR; i++) {
			light[0] = .1 * (i + 1) / NBLUR;
			light[1] = .1 * (i + 1) / NBLUR;
			light[2] = .2 * (i + 1) / NBLUR;
			light[3] = 1;
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, light);

			draw_ball(ball[j].xhist[i], ball[j].yhist[i], 5);
		}
	}
}

void drawframe() {
	int x, y, i;
	uint8_t heightmap[WORLDH][WORLDW];
	uint8_t *overlay = alloca((screen->w/2) * (screen->h/2) * 3);

	if(!worldmap_valid) {
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
			if(brick[i].flags & BRICKF_LIVE) {
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
	}

	if(softglow) {
		glViewport(0, 0, screen->w / 2, screen->h / 2);
		drawscene(1);
		glReadPixels(0, 0, screen->w / 2, screen->h / 2, GL_RGB, GL_UNSIGNED_BYTE, overlay);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, TEX_OVERLAY);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB, screen->w / 2, screen->h / 2, 0, GL_RGB, GL_UNSIGNED_BYTE, overlay);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	glViewport(0, 0, screen->w, screen->h);
	drawscene(1);

	if(softglow) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, 1, 0, 1, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glDisable(GL_FOG);
		glDisable(GL_CULL_FACE);
		glColor3d(.05, .05, .05);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBegin(GL_QUADS);
		for(y = -4; y <= 4; y += 2) {
			for(x = -4; x <= 4; x += 2) {
				if(x || y) {
					glTexCoord2d(x, y);
					glVertex3d(0, 0, 0);
					glTexCoord2d(x + screen->w / 2, y);
					glVertex3d(1, 0, 0);
					glTexCoord2d(x + screen->w / 2, y + screen->h / 2);
					glVertex3d(1, 1, 0);
					glTexCoord2d(x, y + screen->h / 2);
					glVertex3d(0, 1, 0);
				}
			}
		}
		glEnd();
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
	}

	draw_text();
}

void draw_text() {
	char buf[32];
	char scorestr[32];
	int i;

	bigint_to_str(score, scorestr, sizeof(scorestr));
	snprintf(buf, sizeof(buf), "Score: %s", scorestr);
	draw_utf_str(font, buf, 2.f, 4.f);
	snprintf(buf, sizeof(buf), "%3d fps", fps);
	draw_utf_str(font, buf, 2.f, screen->h - 100);
	for(i = 0; i < lives; i++) {
		draw_utf_str(font, "*", screen->w - 300 + 60 * i, screen->h - 100);
	}

	if(gameover) {
		draw_utf_str(font, "game over", screen->w / 2 - 270, screen->h / 2);
	}
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
			if(down) {
				if(gameover) {
					gameover = 0;
					resetlevel();
				} else {
					for(i = 0; i < nball; i++) {
						if(ball[i].flags & BALLF_HELD) {
							ball[i].flags &= ~BALLF_HELD;
							ball[i].x = (INRADIUS - BALLRADIUS) * cos((64 - eye_angle) * M_PI * 2 / 256);
							ball[i].y = (INRADIUS - BALLRADIUS) * sin((64 - eye_angle) * M_PI * 2 / 256);
							ball[i].dx = -ball[i].x / (INRADIUS - BALLRADIUS);
							ball[i].dy = -ball[i].y / (INRADIUS - BALLRADIUS);
						}
					}
				}
			}
			break;
		case SDLK_g:
			if(down) {
				softglow ^= 1;
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

void removebrick(struct brick *b) {
	b->flags &= ~BRICKF_LIVE;
	worldmap_valid = 0;
	bigint_add(score, bonus, score);
	bigint_add(bonus, bonus, bonus);
}

int collide(struct ball *ba, double prevx, double prevy, struct brick *br) {
	int rm = 0;

	if(ba->x >= br->x - 8 - WORLDW/2
	&& ba->x <= br->x + 8 - WORLDW/2) {
		if(ba->dy < 0
		&& ba->y < (br->y + 8 - WORLDH/2)
		&& prevy >= (br->y + 8 - WORLDH/2)) {
			ba->dy = -ba->dy * 20;
			rm = 1;
		} else if(ba->dy > 0
		&& ba->y > (br->y - 8 - WORLDH/2)
		&& prevy <= (br->y - 8 - WORLDH/2)) {
			ba->dy = -ba->dy * 20;
			rm = 1;
		}
	}
	if(ba->y >= br->y - 8 - WORLDH/2
	&& ba->y <= br->y + 8 - WORLDH/2) {
		if(ba->dx < 0
		&& ba->x < (br->x + 8 - WORLDW/2)
		&& prevx >= (br->x + 8 - WORLDW/2)) {
			ba->dx = -ba->dx * 20;
			rm = 1;
		} else if(ba->dx > 0
		&& ba->x > (br->x - 8 - WORLDW/2)
		&& prevx <= (br->x - 8 - WORLDW/2)) {
			ba->dx = -ba->dx * 20;
			rm = 1;
		}
	}
	if(rm) removebrick(br);
	return rm;
}

void rotate(double *xp, double *yp, double a) {
	double x = *xp, y = *yp;

	*xp = x * cos(a) - y * sin(a);
	*yp = x * sin(a) + y * cos(a);
}

void bonus_reset()
{
	bigint_set(bonus, 21349512);
}

void physics() {
	int i, j;
	double r, size;
	double prevx, prevy;

	paddle_vel = (paddle_vel + key_dx * MAX_PADDLE_VEL) / 2;

	eye_angle += paddle_vel * .01;
	if(eye_angle >= 256) eye_angle -= 256;
	if(eye_angle < 0) eye_angle += 256;

	for(i = 0; i < nball; i++) {
		if(ball[i].flags & BALLF_HELD) {
			ball[i].x = (INRADIUS - BALLRADIUS) * cos((64 - eye_angle) * M_PI * 2 / 256);
			ball[i].y = (INRADIUS - BALLRADIUS) * sin((64 - eye_angle) * M_PI * 2 / 256);
			ball[i].dx = ball[i].dy = 0;
		} else {
			for(j = 0; j < nbrick; j++) {
				if(brick[j].flags & BRICKF_LIVE) {
					double xdiff = (brick[j].x - WORLDW / 2) - ball[i].x;
					double ydiff = (brick[j].y - WORLDH / 2) - ball[i].y;
					double dist2 = xdiff * xdiff + ydiff * ydiff;
					if(dist2) {
						ball[i].dx += GRAVITY * xdiff / dist2;
						ball[i].dy += GRAVITY * ydiff / dist2;
					}
				}
			}
			size = hypot(ball[i].dx, ball[i].dy);
			if(size < .01) {
				ball[i].dx += (rand() & 1)? 1 : -1;
				ball[i].dy += (rand() & 1)? 1 : -1;
			} else {
				ball[i].dx /= size;
				ball[i].dy /= size;
			}
			prevx = ball[i].x;
			prevy = ball[i].y;
			ball[i].x += ball[i].dx * BALLSPEED;
			ball[i].y += ball[i].dy * BALLSPEED;
			r = hypot(ball[i].x, ball[i].y);
			if(r > INRADIUS) {
				bonus_reset();
				if(ball[i].flags & BALLF_OUTSIDE) {
					if(r > 2 * INRADIUS) {
						ball[i].flags = BALLF_HELD;
						if(lives) {
							lives--;
						} else {
							gameover = 1;
							nball = 0;
						}
					}
				} else {
					double angle = atan2(ball[i].y, ball[i].x);
					double paddle = (64 - eye_angle) * M_PI * 2 / 256;
					double angdiff = angle - paddle;
					if(angdiff > M_PI) angdiff -= M_PI * 2;
					if(angdiff < -M_PI) angdiff += M_PI * 2;
					if(angdiff > M_PI / 8
					|| angdiff < -M_PI / 8) {
						ball[i].flags |= BALLF_OUTSIDE;
					} else {
						double normx = ball[i].x;
						double normy = ball[i].y;
						rotate(&normx, &normy, -angdiff * .9);
						mirror(&ball[i].dx, &ball[i].dy, normx, normy);
						ball[i].x = prevx;
						ball[i].y = prevy;
					}
				}
			} else {
				for(j = 0; j < nbrick; j++) {
					if(brick[j].flags & BRICKF_LIVE) {
						if(collide(&ball[i], prevx, prevy, &brick[j])) break;
					}
				}
			}
		}
		memmove(ball[i].xhist, ball[i].xhist + 1, sizeof(float) * (NBLUR - 1));
		memmove(ball[i].yhist, ball[i].yhist + 1, sizeof(float) * (NBLUR - 1));
		ball[i].xhist[NBLUR - 1] = ball[i].x;
		ball[i].yhist[NBLUR - 1] = ball[i].y;
	}
}

void load_resources()
{
	font = load_font("testfont.ttf", 100.f, 3);
	score = new_bigint(0);
	bonus = new_bigint(21349512);
}

int main() {
	int	time = 0;
	int	frames = 0;
	Uint32	millis;

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	atexit(SDL_Quit);

	load_resources();

	videosetup(1);
	glsetup();

	SDL_ShowCursor(SDL_DISABLE);

	physics();
	millis = SDL_GetTicks();
	resetlevel();
	nball = 0;

	while(running) {
		SDL_Event event;
		Uint32 now = SDL_GetTicks();

		time += now - millis;
		if (time >= 1000) {
			fps = (int) (frames / (time / 1000.f));
			frames = time = 0;
		}

		while(now > millis + 10) {
			physics();
			millis += 10;
		}

		drawframe();
		frames++;

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
