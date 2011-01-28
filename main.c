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

#define GRIDSCALE .1

static uint8_t eye_angle = 0;

enum {
	TEX_GRID
};

struct worldpixel {
	uint8_t		sinkage;
	uint8_t		color;
	uint8_t		height;
} worldmap[WORLDH][WORLDW];

void glsetup() {
	uint8_t gridtexture[16][16];
	int x, y;

	for(y = 0; y < 16; y++) {
		for(x = 0; x < 16; x++) {
			gridtexture[y][x] = (y && x)? 0 : 255;
		}
	}
	glBindTexture(GL_TEXTURE_2D, TEX_GRID);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE, 16, 16, GL_LUMINANCE, GL_UNSIGNED_BYTE, gridtexture);
}

void add_thing(int xpos, int ypos) {
	int x, y, xx, yy;

	// todo optimize
	for(yy = -4; yy <= 4; yy++) {
		for(xx = -4; xx <= 4; xx++) {
			for(y = 0; y < 16; y++) {
				for(x = 0; x < 16; x++) {
					worldmap[ypos + y + yy][xpos + x + xx].color = rand() & 7;
					worldmap[ypos + y + yy][xpos + x + xx].height = 1;
					worldmap[ypos + y + yy][xpos + x + xx].sinkage++;
				}
			}
		}
	}
}

void drawframe() {
	int x, y;

	memset(worldmap, 0, sizeof(worldmap));
	add_thing(40, 40);
	add_thing(90, 90);
	add_thing(140, 140);
	add_thing(190, 190);
	add_thing(174, 190);
	add_thing(158, 190);

	//glViewport(0, 0, screen->w, screen->h);
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

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, TEX_GRID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	for(y = 0; y < WORLDH - 1; y++) {
		glBegin(GL_QUAD_STRIP);
		for(x = 0; x < WORLDW; x++) {
			glTexCoord2d(x * GRIDSCALE, y * GRIDSCALE);
			glVertex3d(x - WORLDW / 2, -.2 * worldmap[y][x].sinkage, y - WORLDH / 2);
			glTexCoord2d(x * GRIDSCALE, (y + 1) * GRIDSCALE);
			glVertex3d(x - WORLDW / 2, -.2 * worldmap[y + 1][x].sinkage, y + 1 - WORLDH / 2);
		}
		glEnd();
	}
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

int main() {
	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);

	screen = SDL_SetVideoMode(640, 400, 0, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL);
	if(!screen) errx(1, "SDL_SetVideoMode");

	glsetup();

	while(running) {
		SDL_Event event;

		drawframe();

		SDL_GL_SwapBuffers();
		if(!SDL_WaitEvent(&event)) break;

		switch(event.type) {
			case SDL_KEYDOWN:
				handle_key(event.key.keysym.sym);
				break;
			default:
				break;
		}
	}

	return 0;
}
