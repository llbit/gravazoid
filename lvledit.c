#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <errno.h>
#include <float.h>

#include "ark.h"
#include "cttf/shape.h"
#include "cttf/list.h"
#include "block.h"

struct vec;
struct seg;

static list_t*	blocks;

static int		shift = 0;
static float		cx = -1;
static float		cy = -1;

// I/O streams
static FILE*	in = NULL;
static FILE*	out = NULL;

SDL_Surface*	screen;
static bool	running = true;

#define WINDOW_W (700)
#define WINDOW_H (700)

static void setup_video();
static void render();
static void handle_event(SDL_Event* event);
static void handle_key(SDLKey key, int down);
static void on_left_click(float x, float y);
static void on_right_click(float x, float y);
static void delete_closest_block(float x, float y);
static void print_help();
static list_t* closest_block(float x, float y);
static block_t* add_block(float x, float y);
static float from_screen_x(int x);
static float from_screen_y(int y);

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	atexit(SDL_Quit);

	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			if (!strcmp(argv[i], "-h")) {
				print_help();
				exit(0);
			} else {
				fprintf(stderr, "illegal option: %s\n",
						argv[i]);
				print_help();
				exit(1);
			}
		} else if (!in) {
			in = fopen(argv[1], "r");
			if (!in) {
				fprintf(stderr, "could not open file %s (%s)\n",
						argv[1], strerror(errno));
			}
		} else {
			fprintf(stderr, "too many command line parameters\n");
			print_help();
			exit(1);
		}
	}

	if (in) {
		fprintf(stderr, "level loading not implemented yet!\n");
		exit(1);
	}

	if (!out) out = stdout;

	setup_video();

	while (running) {
		SDL_Event event;

		render();

		SDL_GL_SwapBuffers();
		while (SDL_PollEvent(&event)) {
			handle_event(&event);
		}
	}

	// write level
	/*int	index = 0;
	list_t*	p;
	list_t*	h;
	p = h = vecs;
	if (p)
	do {
		struct vec*	vec = p->data;
		p = p->succ;

		vec->vindex = index++;
		fprintf(out, "v: %f, %f\n", vec->x, vec->y);
	} while (p != h);
	p = h = segs;
	if (p)
	do {
		struct seg*	seg = p->data;
		p = p->succ;

		fprintf(out, "s: %d, %d\n",
				seg->origin->vindex,
				seg->end->vindex);
	} while (p != h);*/

	return 0;
}

void print_help()
{
	printf("usage: ftest [OPTION] [FILE]\n");
	printf("  where FILE is a shape file to edit\n");
	printf("  OPTION may be one of\n");
	printf("    -h         print help\n");
	//printf("    -o TARGET  route output to TARGET\n");
}

/* Parallel projection looking down on the x-y plane
 */
static void top_view(float left, float right, float bottom, float top, float near, float far)
{
	float	mat[16];

	mat[0] = 2 / (right-left);
	mat[4] = 0;
	mat[8] = 0;
	mat[12] = - (right+left) / (right-left);

	mat[1] = 0;
	mat[5] = 0;
	mat[9] = 2 / (top-bottom);
	mat[13] = - (top+bottom) / (top-bottom);

	mat[2] = 0;
	mat[6] = - 2 / (far-near);
	mat[10] = 0;
	mat[14] = - (far+near) / (far-near);

	mat[3] = 0;
	mat[7] = 0;
	mat[11] = 0;
	mat[15] = 1;

	glLoadMatrixf(mat);
}

void setup_video()
{
	uint32_t	flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL;

	screen = SDL_SetVideoMode(WINDOW_W, WINDOW_H, 0, flags);
	if (!screen) {
		fprintf(stderr, "Failed to set video mode");
		exit(1);
	}

	SDL_ShowCursor(SDL_ENABLE);

	glViewport(0, 0, screen->w, screen->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	top_view(0, WORLDW, 0, WORLDH, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glColor3f(1.f, 1.f, 1.f);

}

void render()
{
	list_t*	p;

	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	p = blocks;
	if (p) do {
		block_t*	block = p->data;

		glColor3f(1, 1, 1);

		draw_block(block);

		p = p->succ;
	} while (p != blocks);
}

static float from_screen_x(int x) {
	return WORLDW * x / (float) WINDOW_W;
}

static float from_screen_y(int y) {
	return WORLDH * (WINDOW_H - y) / (float) WINDOW_H;
}

void handle_event(SDL_Event* event)
{
	float mx;
	float my;
	switch(event->type) {
		case SDL_MOUSEBUTTONDOWN:
			mx = from_screen_x(event->motion.x);
			my = from_screen_y(event->motion.y);
			if (event->button.button == SDL_BUTTON_LEFT)
				on_left_click(mx, my);
			else if (event->button.button == SDL_BUTTON_MIDDLE)
				delete_closest_block(mx, my);
			else if (event->button.button == SDL_BUTTON_RIGHT)
				on_right_click(mx, my);
			break;
		case SDL_MOUSEMOTION:
			cx = from_screen_x(event->motion.x);
			cy = from_screen_y(event->motion.y);
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

block_t* add_block(float x, float y)
{
	block_t*	block = load_block("blocks/block0.shape");
	block->x = x + WORLDW/2;//100 * x;
	block->y = 200;
	block->z = y + WORLDH/2;//100 * y;
	list_add(&blocks, block);
	return block;
}

/* Left click handler
 */
void on_left_click(float x, float y)
{
	add_block(x, y);
}

/* Right click handler
 */
void on_right_click(float x, float y)
{
}

/* Find the list item for the vertex closest
 * to the given point (x, y)
 */
list_t* closest_block(float x, float y)
{
	list_t*	closest = NULL;
	float	min_dist = 3;
	list_t*	p;
	p = blocks;
	if (p) do {
		block_t*	block = p->data;
		float		dx;
		float		dy;
		float		dist;

		dx = block->x - x;
		dy = block->y - y;
		dist = dx*dx + dy*dy;
		if (dist < min_dist) {
			min_dist = dist;
			closest = p;
		}
		p = p->succ;
	} while (p != blocks);
	return closest;
}

/* Delete block closest to cursor
 */
void delete_closest_block(float x, float y)
{
	list_t*	closest = closest_block(x, y);
	if (closest) {
		block_t*	block = closest->data;
		list_remove_item(&blocks, block);
		// TODO: delete_block(block);
	}
}

void handle_key(SDLKey key, int down)
{
	switch(key) {
	case SDLK_LSHIFT:
		shift = down;
		break;
	case SDLK_q:
		running = false;
		break;
	case SDLK_d:
		if (down) {
			delete_closest_block(cx, cy);
		}
		break;
	case SDLK_j:
	case SDLK_DOWN:
		break;
	case SDLK_k:
	case SDLK_UP:
		break;
	case SDLK_h:
	case SDLK_LEFT:
		break;
	case SDLK_l:
	case SDLK_RIGHT:
		break;
	case SDLK_EQUALS:
		break;
	case SDLK_MINUS:
		break;
	default:;
	}
}

