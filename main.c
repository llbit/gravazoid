#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include "ark.h"
#include "render.h"
#include "cttf/text.h"
#include "bigint.h"
#include "block.h"
#include "cttf/list.h"
#include "sfx.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

static void errx(int exitval, const char* msg)
{
	fprintf(stderr, "%s", msg);
	exit(exitval);
}

static int colors[][3] = {
	{0x65, 0x9D, 0xFD}, // light blue
	{0x63, 0x7E, 0x9A}, // dark blue
	{0x40, 0x4A, 0xE1}, // bright blue
	{0xDA, 0x0D, 0x18}, // bright red
	{0x8F, 0x09, 0x00}, // dark red
	{0xB6, 0xDE, 0xFF}, // pale blue

};

static int ball_color[3] = { 0x33, 0x8C, 0x5C };

static void draw_hud();
static void draw_utf_word(font_t* font, const char* word, float x, float y);

static int running = 1;
SDL_Surface *screen;

static int softglow = 0;

static int fps = 0;

static double eye_angle = 0;
static int key_dx = 0;
static int paddle_vel = 0;

int lives;
int gameover = 1;

static font_t* font = NULL;

SDL_sem *semaphore;

GLUquadric *ballquad;

static bigint_t* bonus;
static bigint_t* score;
static bigint_t* diff;
static int score_up = 0;

int worldmap_valid;

typedef struct shard {
	bool		visited;
	int		visible;
	int		color;
	float		x;
	float		y;
	float		z;
	float		xvel;
	float		yvel;
	float		zvel;
	int		ttl;
} shard_t;

static list_t*	shards = NULL;

list_t*	blocks;
int bricks_left;

#define BALLF_HELD		1
#define BALLF_OUTSIDE		2

struct ball {
	double		x, y;
	double		dx, dy;
	int		flags;
	float		xhist[NBLUR], yhist[NBLUR];
} ball[MAXBALL];
int nball;

static void handle_event(SDL_Event* event);
static void on_motion(SDL_MouseMotionEvent* motion);
static void on_button(SDL_MouseButtonEvent* button);
static void on_action();

void videosetup(bool fullscreen) {
	Uint32 flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL;
	SDL_Rect **list;
	int w, h;

	if(fullscreen) flags |= SDL_FULLSCREEN;
	list = SDL_ListModes(0, flags);
	if(!list) {
		errx(1, "No usable video modes found");
	} else if(list == (SDL_Rect **) -1) {
		w = 800;
		h = 600;
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
	uint8_t vignette[32][32];
	uint8_t shard[4][4] = {
		{0xFF, 0xFF, 0xFF, 0xFF},
		{0xFF, 0xFF, 0xFF, 0xFF},
		{0xFF, 0xFF, 0xFF, 0xFF},
		{0xD4, 0xD4, 0xD4, 0xD4},
	};
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
	for (y = 0; y < 32; ++y) {
		for (x = 0; x < 32; ++x) {
			int	xc = x-16;
			int	yc = y-16;
			double	r = (xc*xc+yc*yc) / (double)(16*16);
			if (r > 1.0) r = 1.0;
			r = 1 - r;
			vignette[y][x] = 255*r;
		}
	}
	glBindTexture(GL_TEXTURE_2D, TEX_GRID);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE, 16, 16, GL_LUMINANCE, GL_UNSIGNED_BYTE, gridtexture);
	glBindTexture(GL_TEXTURE_2D, TEX_BLOB);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE, BLOBSIZE, BLOBSIZE, GL_LUMINANCE, GL_UNSIGNED_BYTE, blobtexture);
	glBindTexture(GL_TEXTURE_2D, TEX_VIGNETTE);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE, 32, 32, GL_LUMINANCE, GL_UNSIGNED_BYTE, vignette);
	glBindTexture(GL_TEXTURE_2D, TEX_SHARD);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE, 4, 4, GL_LUMINANCE, GL_UNSIGNED_BYTE, shard);

	ballquad = gluNewQuadric();
}

/* Load the level file */
void resetlevel(int restart) {
	FILE*	in;

	srand(0);
	free_list(&blocks);
	in = fopen("level", "r");
	bricks_left = load_level(in, &blocks);
	fclose(in);

	if(restart) {
		bigint_set(score, 0);
		lives = 5;
	}

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

void draw_shards()
{
	GLfloat light[4];
	list_t*	p;
	list_t*	h;
	p = h = shards;

	glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, TEX_SHARD);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	if (p)
	do {
		shard_t*	shard;
		
		shard = p->data;
		p = p->succ;

		if (!shard->visible) continue;
		int px = shard->x - WORLDW/2;
		int py = HEIGHTSCALE - SINKHEIGHTTOP * shard->y;
		int pz = shard->z - WORLDH/2;
		light[0] = colors[shard->color][0] / 255.;
		light[1] = colors[shard->color][1] / 255.;
		light[2] = colors[shard->color][2] / 255.;
		light[3] = 0.3f;
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, light);
		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
		//glLoadIdentity();
		glTranslatef(px, py, pz);
		glRotatef(180+eye_angle * 360 / 256.f, 0, 1, 0);
		glBegin(GL_QUADS);
		glTexCoord2d(0, 1);
		glVertex3d(-SHARDW, -SHARDH, 0);
		glTexCoord2d(0, 0);
		glVertex3d(-SHARDW, +SHARDH, 0);
		glTexCoord2d(1, 0);
		glVertex3d(+SHARDW, +SHARDH, 0);
		glTexCoord2d(1, 1);
		glVertex3d(+SHARDW, -SHARDH, 0);
		glEnd();
		glPopMatrix();

	} while (p != h);
	glPopAttrib();
}

void drawscene(int with_membrane) {
	int	i, j;
	GLfloat	light[4];
	list_t*	p;
	list_t*	h;

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

	if(with_membrane) draw_membrane();

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	light[0] = 150 * sin(eye_angle * M_PI * 2 / 256);
	light[1] = 180;
	light[2] = 150 * cos(eye_angle * M_PI * 2 / 256);
	light[3] = 0;
	glLightfv(GL_LIGHT0, GL_POSITION, light);
	glEnable(GL_NORMALIZE);
	p = h = blocks;
	if (p) do {
		block_t*	block = p->data;
		p = p->succ;

		light[0] = colors[block->color][0] / 255.;
		light[1] = colors[block->color][1] / 255.;
		light[2] = colors[block->color][2] / 255.;
		light[3] = 1;
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, light);

		draw_block(block);

	} while (p != h);

	draw_shards();

	light[0] = .2;
	light[1] = .2;
	light[2] = .2;
	light[3] = 1;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, light);
	ridge(-eye_angle - 10 + 64, -eye_angle + 10 + 64);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	light[0] = ball_color[0] / 255.0;
	light[1] = ball_color[1] / 255.0;
	light[2] = ball_color[2] / 255.0;
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
	int	x, y;
	//int	x, y, i;
	//uint8_t	heightmap[WORLDH][WORLDW];
	uint8_t *overlay = alloca((screen->w/2) * (screen->h/2) * 3);

	glNormal3d(0, 1, 0);

	/*if(!worldmap_valid) {
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
	}*/

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

	draw_hud();
}

static void draw_hud()
{
	char ibuf[64];
	char buf[64];
	int i;
	float text_height;
	float offset = 0.2f;

	text_height = line_height(font);

	glPushAttrib(GL_VIEWPORT_BIT | GL_TRANSFORM_BIT);
	//glViewport(0, 0, screen->w, screen->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 20, 0, 20, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glScalef(1, 1, 1);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	// score:
	bigint_to_str(score, ibuf, sizeof ibuf);
	snprintf(buf, sizeof buf, "Score: %s", ibuf);
	glColor3f(1.f, 1.f, 1.f);
	draw_utf_word(font, buf, offset, offset);

	// score_up:
	if (score_up > 0) {
		buf[0] = '+';
		bigint_to_str(diff, buf+1, sizeof buf - 1);
		float v = logf(score_up) / logf(1000);
		glColor3f(v, v, v);
		draw_utf_word(font, buf, offset + line_width(font, "Score:"),
				0.6f * text_height + 1.f*v);
	}

#ifdef DRAW_FPS
	snprintf(buf, sizeof(buf), "%3d fps", fps);
	glColor3f(1.f, 1.f, 1.f);
	draw_utf_word(font, buf, offset, 20 - offset - text_height);
#endif
	for (i = 0; i < lives; i++) {
		draw_utf_word(font, "*", 20 - offset - (i+1) * line_width(font, "*"),
				20 - offset - text_height);
	}

	if(gameover) {
		float width = line_width(font, "game over");
		draw_utf_word(font, "game over", 10-width/2, 10);
	}

	glPopAttrib();
}

static void draw_utf_word(font_t* font, const char* word, float x, float y)
{
	glPushMatrix();
	glTranslatef(x, y, 0.f);

	draw_filled_word(font, word);

	glPopMatrix();
}

void handle_key(SDLKey key, int down) {

	switch(key) {
		case SDLK_q:
		case SDLK_ESCAPE:
			running = 0;
			break;
		case SDLK_h:
		case SDLK_LEFT:
			if(down) {
				key_dx = -1;
			} else {
				if(key_dx == -1) key_dx = 0;
			}
			break;
		case SDLK_l:
		case SDLK_RIGHT:
			if(down) {
				key_dx = 1;
			} else {
				if(key_dx == 1) key_dx = 0;
			}
			break;
		case SDLK_SPACE:
			if (down)
				on_action();
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

void add_shards(int x, int y, int z, int color)
{
	shard_t*	shard;

	for (int i = 0; i < 20; ++i) {
		double angle = rand()%360;
		angle = angle * 2*M_PI / 360.0;

		shard = malloc(sizeof(shard_t));
		shard->visible = rand()%2;
		shard->color = color;
		shard->x = x + cos(angle) * (1.7+rand()%4);
		shard->y = y + rand()%11-5;
		shard->z = z + sin(angle) * (1.7+rand()%4);
		shard->xvel = 10*cos(angle);
		shard->yvel = 0;
		shard->zvel = 10*sin(angle);
		shard->ttl = 230 + rand()%200;

		list_add(&shards, shard);
	}
}

static void remove_block(block_t* block) {
	worldmap_valid = 0;

	add_shards(block->x, block->y, block->z, block->color);
	list_remove_item(&blocks, block);

	bricks_left--;
	if(!bricks_left) {
		resetlevel(0);
	}

	score_up = 1000;
	bigint_add(score, bonus, score);
	bigint_add(diff, bonus, diff);
	bigint_add(bonus, bonus, bonus);
}

int collide(struct ball *ba, double prevx, double prevy, block_t* block) {
	int rm = 0;

	if(ba->x >= block->x - 8 - WORLDW/2
	&& ba->x <= block->x + 8 - WORLDW/2) {
		if(ba->dy < 0
		&& ba->y < (block->z + 8 - WORLDH/2)
		&& prevy >= (block->z + 8 - WORLDH/2)) {
			ba->dy = -ba->dy * 20;
			rm = 1;
		} else if(ba->dy > 0
		&& ba->y > (block->z - 8 - WORLDH/2)
		&& prevy <= (block->z - 8 - WORLDH/2)) {
			ba->dy = -ba->dy * 20;
			rm = 1;
		}
	}
	if(ba->y >= block->z - 8 - WORLDH/2
	&& ba->y <= block->z + 8 - WORLDH/2) {
		if(ba->dx < 0
		&& ba->x < (block->x + 8 - WORLDW/2)
		&& prevx >= (block->x + 8 - WORLDW/2)) {
			ba->dx = -ba->dx * 20;
			rm = 1;
		} else if(ba->dx > 0
		&& ba->x > (block->x - 8 - WORLDW/2)
		&& prevx <= (block->x - 8 - WORLDW/2)) {
			ba->dx = -ba->dx * 20;
			rm = 1;
		}
	}
	if(rm) remove_block(block);
	return rm;
}

void rotate(double *xp, double *yp, double a) {
	double x = *xp, y = *yp;

	*xp = x * cos(a) - y * sin(a);
	*yp = x * sin(a) + y * cos(a);
}

void bonus_reset()
{
	bigint_set(bonus, 1);
}

void update_particles()
{
	list_t*	p;
	list_t*	h;
	p = h = shards;

	if (p)
	do {
		shard_t*	shard = p->data;
		p = p->succ;
		shard->visited = false;
	} while (p != h);

	while (shards) {
		shard_t*	shard = shards->data;
		
		if (shard->visited)
			break;
		shard->ttl -= 10;
		if (shard->ttl < 0) {
			free(shard);
			list_remove(&shards);
		} else {
			shard->visited = true;
			shard->visible = rand()%2;
			shard->x += 0.01 * shard->xvel;
			shard->y += 0.01 * shard->yvel;
			shard->z += 0.01 * shard->zvel;
			shard->yvel += 8.79;
			shards = shards->succ;
		}
	}
}

void physics() {
	int	i;
	//int	j;
	double r, size;
	double prevx, prevy;

	update_particles();

	paddle_vel = (paddle_vel * .9 + key_dx * MAX_PADDLE_VEL * .1);
	if(gameover) paddle_vel = (paddle_vel * .9 + 10);

	eye_angle += paddle_vel * .0018;
	if(eye_angle >= 256) eye_angle -= 256;
	if(eye_angle < 0) eye_angle += 256;

	for(i = 0; i < nball; i++) {
		if(ball[i].flags & BALLF_HELD) {
			ball[i].x = (INRADIUS - BALLRADIUS) * cos((64 - eye_angle) * M_PI * 2 / 256);
			ball[i].y = (INRADIUS - BALLRADIUS) * sin((64 - eye_angle) * M_PI * 2 / 256);
			ball[i].dx = ball[i].dy = 0;
		} else {
			/*for(j = 0; j < nbrick; j++) {
				if(brick[j].flags & BRICKF_LIVE) {
					double xdiff = (brick[j].x - WORLDW / 2) - ball[i].x;
					double ydiff = (brick[j].y - WORLDH / 2) - ball[i].y;
					double dist2 = xdiff * xdiff + ydiff * ydiff;
					if(dist2) {
						ball[i].dx += GRAVITY * xdiff / dist2;
						ball[i].dy += GRAVITY * ydiff / dist2;
					}
				}
			}*/
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
						sfx_gameover();
						sfx_startsong(0);
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
						sfx_hitbase();
					}
				}
			} else {
				list_t*	p;
				list_t*	h;
				p = h = blocks;
				if (p) do {
					block_t*	block = p->data;
					p = p->succ;

					if(collide(&ball[i], prevx, prevy, block)) {
						sfx_hitbrick();
						break;
					}
				} while (p != h);
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
	font = load_font("Pusselbit.ttf", 3);
	load_blocks(); /* load all block types */
	score = new_bigint(0);
	bonus = new_bigint(1);
	diff = new_bigint(0);
}

int main(int argc, char* argv[]) {
	int	time = 0;
	int	frames = 0;
	bool	fullscreen = true;
	Uint32	millis;

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-window"))
			fullscreen = false;
	}

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		return 1;
	}
	atexit(SDL_Quit);

	load_resources();

	videosetup(fullscreen);
	glsetup();

	SDL_ShowCursor(SDL_DISABLE);

	sfx_init();
	sfx_startsong(0);

	physics();
	millis = SDL_GetTicks();
	resetlevel(1);
	nball = 0;

	while(running) {
		SDL_Event event;
		Uint32 now = SDL_GetTicks();

		if (score_up > 0) {
			score_up -= now - millis;
			if (score_up <= 0)
				bigint_set(diff, 0);
		}
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
			handle_event(&event);
		}
	}

	free_blocks();

	return 0;
}

void handle_event(SDL_Event* event)
{
	switch(event->type) {
		case SDL_MOUSEBUTTONDOWN:
			on_button(&event->button);
			break;
		case SDL_MOUSEMOTION:
			on_motion(&event->motion);
			break;
		case SDL_KEYDOWN:
			handle_key(event->key.keysym.sym, 1);
			break;
		case SDL_KEYUP:
			handle_key(event->key.keysym.sym, 0);
			break;
		default:;
	}
}

void on_motion(SDL_MouseMotionEvent* motion)
{
	eye_angle += motion->xrel * .18;
}

void on_button(SDL_MouseButtonEvent* button)
{
	if (button->button == SDL_BUTTON_LEFT)
		on_action();
}

void on_action()
{
	int i;
	if(gameover) {
		gameover = 0;
		resetlevel(1);
		sfx_startsong(0);
	} else {
		sfx_advance();
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
