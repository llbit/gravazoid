#ifndef ARK_TEXT_H
#define ARK_TEXT_H

#include "cttf/cttf.h"

typedef struct vfont	vfont_t;

// the vfont structure is a vector font representation
struct vfont {
	ttf_t*		ttf;
	shape_t**	chr;
	float		scale;
};

// returns non-NULL on success
vfont_t* load_font(const char* name, float scale, int ipl);
void free_font(vfont_t** font);
void font_load_chr(vfont_t* font, uint16_t chr);
void draw_utf_str(vfont_t* font, const char* str);

#endif

