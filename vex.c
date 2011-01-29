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

SDL_Surface*	screen;
static bool	running = true;

#define WINDOW_W (300)
#define WINDOW_H (300)

static void handle_event(SDL_Event* event);
static void handle_key(SDLKey key, int down);

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

		glClear(GL_COLOR_BUFFER_BIT);
		render_shape(shape, 0.f, 0.f);

		SDL_GL_SwapBuffers();
		while (SDL_PollEvent(&event)) {
			handle_event(&event);
		}
	}

	return 0;
}

void handle_event(SDL_Event* event)
{
	switch(event->type) {
		case SDL_MOUSEBUTTONDOWN:
			{
				int x = event->button.x;
				int y = WINDOW_H - event->button.y;
				shape_add_vec(shape, x, y);
				if (pred >= 0)
					shape_add_seg(shape, pred, curr);
				pred = curr;
				curr += 1;
				printf("%f, %f\n", x / (float)WINDOW_W,
						y / (float)WINDOW_H);
					break;
			}
		case SDL_KEYDOWN:
			handle_key(event->key.keysym.sym, 1);
			break;
		case SDL_KEYUP:
			handle_key(event->key.keysym.sym, 0);
			break;
		default:;
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

