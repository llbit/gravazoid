#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <stdlib.h>
#include <assert.h>
#include "text.h"
#include "render.h"

extern SDL_Surface *screen;

vfont_t* load_font(const char* name, float scale, int ipl)
{
	FILE*	fp;
	fp = fopen(name, "r");
	if (!fp) {
		fprintf(stderr, "Could not find font \"%s\"\n", name);
		return NULL;
	}
	ttf_t*	ttf;
	ttf = ttf_load(fp);
	fclose(fp);
	if (ttf) {
		ttf->interpolation_level = ipl;

		vfont_t*	obj;
		obj = malloc(sizeof(*obj));
		obj->ttf = ttf;
		obj->chr = malloc(sizeof(shape_t*)*0x10000);
		obj->scale = scale;
		return obj;
	} else {
		return NULL;
	}
}

void free_font(vfont_t** font)
{
	assert(font != NULL);
	if (*font == NULL) return;
	for (int i = 0; i < 0x10000; ++i) {
		if ((*font)->chr[i] != NULL) {
			free_shape(&(*font)->chr[i]);
			(*font)->chr[i] = NULL;
		}
	}
	*font = NULL;
}

void font_load_chr(vfont_t* font, uint16_t chr)
{
	if (font == NULL) return;

	if (font->ttf) {
		font->chr[chr] = new_shape();
		ttf_export_chr_shape(font->ttf, chr,
				font->chr[chr], font->scale);
	}
}

void draw_utf_str(vfont_t* font, const char* str, float x, float y)
{
	if (font == NULL) return;

	glPushAttrib(GL_VIEWPORT_BIT | GL_TRANSFORM_BIT);
	//glViewport(0, 0, screen->w, screen->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, screen->w, 0, screen->h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	const char* p = str;
	float xoff = 0.f;
	while (*p != '\0') {
		uint16_t chr = (uint16_t)*p;
		if (font->chr[chr] == NULL)
			font_load_chr(font, chr);
		glPushMatrix();
		glTranslatef(x+xoff, y, 0.f);
		render_shape(font->chr[chr]);
		glPopMatrix();
		xoff += font->scale * ttf_get_chr_width(font->ttf, chr);
		p += 1;
	}

	glPopAttrib();
}
