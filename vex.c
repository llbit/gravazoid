/**
 * Vector Editor
 */
#include <stdio.h>
#include <err.h>
#include <math.h>
#include <stdbool.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include "render.h"
#include "text.h"
#include "cttf/shape.h"

static shape_t*	shape;
static int	curr = 0;
static int	pred = -1;
static int	first = -1;
static int	cx = -1;
static int	cy = -1;

SDL_Surface*	screen;
static bool	running = true;

#define WINDOW_W (500)
#define WINDOW_H (500)

static void render();
static void handle_event(SDL_Event* event);
static void handle_key(SDLKey key, int down);
static void on_left_click(int x, int y);
static void on_right_click(int x, int y);

void videosetup()
{
	uint32_t	flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL;

	screen = SDL_SetVideoMode(WINDOW_W, WINDOW_H, 0, flags);
	if (!screen)
		errx(1, "Failed to set video mode");

	SDL_ShowCursor(SDL_ENABLE);
}

int main(int argc, const char** argv)
{
	shape = new_shape();

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	atexit(SDL_Quit);

	videosetup();

	while (running) {
		SDL_Event event;

		render();

		SDL_GL_SwapBuffers();
		while (SDL_PollEvent(&event)) {
			handle_event(&event);
		}
	}

	return 0;
}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT);
	render_shape(shape, 0.f, 0.f);
	if (first != -1) {
		glPushAttrib(GL_COLOR_BUFFER_BIT);
		glColor3f(0.84f, 0.84f, 0.84f);
		glBegin(GL_LINES);
		glVertex3f(shape->vec[shape->nvec-1].x,
				shape->vec[shape->nvec-1].y,
				0.f);
		glVertex3f((float)cx, (float)cy, 0.f);
		glEnd();
		glPopAttrib();
	}
}

void handle_event(SDL_Event* event)
{
	switch(event->type) {
		case SDL_MOUSEBUTTONDOWN:
			if (event->button.button == SDL_BUTTON_LEFT)
				on_left_click(event->button.x,
						WINDOW_H - event->button.y);
			else if (event->button.button == SDL_BUTTON_RIGHT)
				on_right_click(event->button.x,
						WINDOW_H - event->button.y);
			break;
		case SDL_MOUSEMOTION:
			cx = event->motion.x;
			cy = WINDOW_H - event->motion.y;
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

void on_left_click(int x, int y)
{
	shape_add_vec(shape, x, y);
	printf("v: %f, %f\n", x / (float)WINDOW_W,
			y / (float)WINDOW_H);

	if (pred >= 0) {
		shape_add_seg(shape, pred, curr);
		printf("s: %d, %d\n", pred, curr);
	}
	if (first == -1)
		first = curr;
	pred = curr;
	curr += 1;
}

void on_right_click(int x, int y)
{
	if (first != -1) {
		shape_add_seg(shape, pred, first);
		first = -1;
		pred = -1;
	}
}

void handle_key(SDLKey key, int down)
{
	switch(key) {
		case SDLK_q:
			running = false;
			break;
		default:;
	}
}

