/**
 * Copyright (c) 2002-2011 Jesper Öqvist <jesper@llbit.se>
 *
 * This file is part of cTTF.
 *
 * cTTF is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cTTF is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with cTTF.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * cTTF loads a TrueType font.
 *
 * It only supports the bezier curve font format which is the
 * most common component of a TTF. Pixel fonts are not supported.
 *
 * [created] 2002-06-23
 * [updated] 2003-01-28
 * 	Added Resolution and EMPointSize
 * 	Various other small changes
 * [updated] 2004-01-29
 * [updated] 2006-03-03
 * 	Adapted for ScrapHeap
 * [updated] 2006-03-15
 * 	Added swenglish comments
 * [updated] 2011-01-11
 * 	Licensed under GPLv3
 * 	Rewritten to C99
*/

#include "cttf.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define TTF_MAXP_CODE (0x7078616D)
#define TTF_GLYF_CODE (0x66796C67)
#define TTF_HEAD_CODE (0x64616568)
#define TTF_HHEA_CODE (0x61656868)
#define TTF_HMTX_CODE (0x78746D68)
#define TTF_CMAP_CODE (0x70616D63)
#define TTF_LOCA_CODE (0x61636F6C)
#define TTF_CODE (0xF53C0F5F)

#define TTF_ON_CURVE (0x01)
#define TTF_XSHORT (0x02)
#define TTF_YSHORT (0x04)
#define TTF_FLAG_REPEAT (0x08)
#define TTF_XREPEAT (0x10)
#define TTF_YREPEAT (0x20)

#define TTF_WORD_ARGUMENTS (0x0001)
#define TTF_ARGUMENTS_ARE_XY (0x0002)
#define TTF_ROUND_XY_TO_GRID (0x0004)
#define TTF_SCALE (0x0008)
#define TTF_RESERVED (0x0010)
#define TTF_MORE_COMPONENTS (0x0020)
#define TTF_XY_SCALE (0x0040)
#define TTF_MATRIX2 (0x0080)
#define TTF_INSTRUCTIONS (0x0100)
#define TTF_USE_THESE_METRICS (0x0200)

#define TTF_GLYPH_TBL_SIZE (256)

ttf_t* ttf_new()
{
	ttf_t*	obj;

	obj = malloc(sizeof(ttf_t));
	obj->glyph_table = NULL;
	obj->glyph_data = NULL;
	obj->plhmtx = NULL;
	obj->plsb = NULL;
	obj->interpolation_level = 1;
	obj->ppem = 12;
	obj->resolution = 96;// Screen resolution DPI
	return obj;
}

void ttf_free(ttf_t** obj)
{
	assert(obj != NULL);
	if (*obj == NULL)
		return;

	if ((*obj)->glyph_table != NULL)
		free((*obj)->glyph_table);
	(*obj)->glyph_table = NULL;

	if ((*obj)->glyph_data != NULL)
		free((*obj)->glyph_data);
	(*obj)->glyph_data = NULL;

	if ((*obj)->plhmtx != NULL)
		free((*obj)->plhmtx);
	(*obj)->plhmtx = NULL;

	if ((*obj)->plsb != NULL)
		free((*obj)->plsb);
	(*obj)->plsb = NULL;

	*obj = NULL;
}

ttf_glyph_data_t* ttf_new_glyph_data()
{
	ttf_glyph_data_t*	obj;

	obj = malloc(sizeof(*obj));
	obj->endpoints = NULL;
	obj->state = NULL;
	obj->px = NULL;
	obj->py = NULL;
	return obj;
}

void ttf_free_glyph_data(ttf_glyph_data_t** obj)
{
	assert(obj != NULL);
	if (*obj == NULL)
		return;

	if ((*obj)->endpoints != NULL)
		free((*obj)->endpoints);
	(*obj)->endpoints = NULL;

	if ((*obj)->state != NULL)
		free((*obj)->state);
	(*obj)->state = NULL;

	if ((*obj)->px != NULL)
		free((*obj)->px);
	(*obj)->px = NULL;

	if ((*obj)->py != NULL)
		free((*obj)->py);
	(*obj)->py = NULL;

	*obj = NULL;
}

static void ttf_seek_header(FILE* file, ttf_table_header_t* table)
{
	table->offset = SWAP_ENDIAN_DWORD(table->offset);
	fseek(file, table->offset, SEEK_SET);
}

void ttf_gd_list_add(ttf_gd_list_t** list,
		ttf_glyph_data_t* data)
{
	assert(list != NULL);


	if (*list == NULL) {
		*list = malloc(sizeof(ttf_gd_list_t));
		(*list)->succ = NULL;
		(*list)->data = data;
	} else {
		ttf_gd_list_t*	p;
		p = *list;
		while (p->succ != NULL)
			p = p->succ;
		p->succ = malloc(sizeof(ttf_gd_list_t));
		p->succ->succ = NULL;
		p->succ->data = data;
	}
}

void ttf_free_gd_list(ttf_gd_list_t** obj)
{
	assert(obj != NULL);
	if (*obj == NULL)
		return;

	ttf_gd_list_t*	p;
	p = *obj;
	while (p != NULL) {
		ttf_gd_list_t*	succ;
		succ = p->succ;
		free(p);
		p = succ;
	}
	*obj = NULL;
}

static void ttf_read_gh(FILE* file, ttf_glyph_header_t* gh)
{
	fread(gh, sizeof(*gh), 1, file);
	gh->number_of_contours = SWAP_ENDIAN_WORD(gh->number_of_contours);
	gh->xmin = SWAP_ENDIAN_WORD(gh->xmin);
	gh->ymin = SWAP_ENDIAN_WORD(gh->ymin);
	gh->xmax = SWAP_ENDIAN_WORD(gh->xmax);
	gh->ymax = SWAP_ENDIAN_WORD(gh->ymax);
}

/*
 * Returns NULL on failure.
 *
 * Prints error messages to stdout.
 */
ttf_t* ttf_load(FILE* file)
{
	ttf_t*	ttfobj;

	if (file == NULL) {
		fprintf(stderr, "Invalid font file: can not read from NULL!\n");
		return NULL;
	}

	ttfobj = ttf_new();

	ttf_tbl_directory_t td;
	fread(&td, sizeof(td), 1, file);
	td.num_tables = SWAP_ENDIAN_WORD(td.num_tables);
	if (td.num_tables < 10) {
		fprintf(stderr, "Invalid font file: too few tables!\n");
		return NULL;
	}

	// load the required TTF headers
	ttf_table_header_t	cmap;
	ttf_table_header_t	head;
	ttf_table_header_t	glyf;
	ttf_table_header_t	maxp;
	ttf_table_header_t	loca;
	ttf_table_header_t	hhea;
	ttf_table_header_t	hmtx;
	ttf_table_header_t	tmph;
	for (int i = 0; i < td.num_tables; i++) {
		// TODO: handle possible read errors
		fread(&tmph, sizeof(tmph), 1, file);
		switch (tmph.tag) {
			case TTF_MAXP_CODE:
				maxp = tmph;
				break;
			case TTF_GLYF_CODE:
				glyf = tmph;
				break;
			case TTF_HEAD_CODE:
				head = tmph;
				break;
			case TTF_LOCA_CODE:
				loca = tmph;
				break;
			case TTF_CMAP_CODE:
				cmap = tmph;
				break;
			case TTF_HHEA_CODE:
				hhea = tmph;
				break;
			case TTF_HMTX_CODE:
				hmtx = tmph;
				break;
		}
	}
	// TODO: check that all required headers were found

	// load head - header
	ttf_seek_header(file, &head);
	ttf_font_header_t fh;
	fread(&fh, sizeof(fh), 1, file);

	if (fh.magic != TTF_CODE) {
		fprintf(stderr, "Invalid font file: incorrect file code!\n");
		fprintf(stderr, "expected code %08X, was %08X\n", TTF_CODE, fh.magic);
		return NULL;
	}

	ttfobj->upem = SWAP_ENDIAN_WORD(fh.upem);
	ttfobj->ymin = SWAP_ENDIAN_WORD(fh.ymin);
	ttfobj->ymax = SWAP_ENDIAN_WORD(fh.ymax);
	ttfobj->xmin = SWAP_ENDIAN_WORD(fh.xmin);
	ttfobj->xmax = SWAP_ENDIAN_WORD(fh.xmax);
	ttfobj->zerobase = false;
	ttfobj->zerolsb = false;

	fh.flags = SWAP_ENDIAN_WORD(fh.flags);
	if (fh.flags & 0x0001) ttfobj->zerobase = true;
	if (fh.flags & 0x0002) ttfobj->zerolsb = true;

	// load maxp - maximum profiles
	ttf_seek_header(file, &maxp);
	ttf_max_profile_tbl_header_t	mpth;
	fread(&mpth, sizeof(mpth), 1, file);
	ttfobj->nglyphs = SWAP_ENDIAN_WORD(mpth.numGlyphs);

	// load hhea - horizontal header
	ttf_seek_header(file, &hhea);
	ttf_horizontal_header_t	hh;
	fread(&hh, sizeof(hh), 1, file);
	hh.num_h_metrics = SWAP_ENDIAN_WORD(hh.num_h_metrics);

	// load hmtx - horizontal metrics
	ttf_seek_header(file, &hmtx);
	ttfobj->plhmtx = malloc(sizeof(ttf_lhmetrics_t) * hh.num_h_metrics);
	ttfobj->nhmtx = hh.num_h_metrics;
	fread(ttfobj->plhmtx, sizeof(ttf_lhmetrics_t), hh.num_h_metrics, file);
	int i = ttfobj->nglyphs - hh.num_h_metrics;
	ttfobj->plsb = malloc(sizeof(int16_t) * i);
	if (i) fread(ttfobj->plsb, sizeof(int16_t), i, file);

	// load cmap - character mappings
	ttf_seek_header(file, &cmap);
	ttf_enctbl_info_t	eti;
	fread(&eti, sizeof(eti), 1, file);
	eti.table_version = SWAP_ENDIAN_WORD(eti.table_version);
	if (eti.table_version != 0) {
		fprintf(stderr, "Invalid font file: wrong table version!\n");
		return NULL;
	}

	ttf_enctbl_header_t* eth =
		malloc(sizeof(ttf_enctbl_header_t) * eti.num_encoding_tables);
	for (i = 0; i < eti.num_encoding_tables; i++) {
		fread(&eth[i], sizeof(ttf_enctbl_header_t), 1, file);
		// accept only windows unicode
		if ((SWAP_ENDIAN_WORD(eth[i].platform_id) == 3) &&
			(SWAP_ENDIAN_WORD(eth[i].encoding_id) == 1))
			goto found_platform;
	}
	free(eth);

	fprintf(stderr, "Invalid font file: only windows unicode acceptable!\n");
	return NULL;

found_platform:
	eth[i].offset = SWAP_ENDIAN_DWORD(eth[i].offset);
	fseek(file, eth[i].offset + cmap.offset, SEEK_SET);
	free(eth);

	// assume format 4 glyph indexing
	// FIXME?
	ttf_mapfmt4_header_t mf4h;
	fread(&mf4h.format, sizeof(mf4h.format), 1, file);
	if (SWAP_ENDIAN_WORD(mf4h.format) != 4) {
		fprintf(stderr, "Invalid font file: Segment mapping not found!\n");
		return NULL;
	}
	fread(&mf4h.length, sizeof(mf4h.length), 1, file);
	fread(&mf4h.version, sizeof(mf4h.version), 1, file);
	fread(&mf4h.seg_count_2, sizeof(mf4h.seg_count_2), 1, file);
	mf4h.seg_count_2 = SWAP_ENDIAN_WORD(mf4h.seg_count_2) >> 1;
	fread(&mf4h.search_range, sizeof(mf4h.search_range), 1, file);
	fread(&mf4h.entry_selector, sizeof(mf4h.entry_selector), 1, file);
	fread(&mf4h.range_shift, sizeof(mf4h.range_shift), 1, file);
	mf4h.end_count = malloc(sizeof(uint16_t) * mf4h.seg_count_2);
	fread(mf4h.end_count, sizeof(uint16_t), mf4h.seg_count_2, file);
	fread(&mf4h.reserved_pad, sizeof(mf4h.reserved_pad), 1, file);
	if (mf4h.reserved_pad) {
	    fprintf(stderr, "Invalid font file: reserved pad not NULL!\n");
	    return NULL;
	}
	mf4h.start_count = malloc(sizeof(uint16_t) * mf4h.seg_count_2);
	fread(mf4h.start_count, sizeof(uint16_t), mf4h.seg_count_2, file);
	mf4h.id_delta = malloc(sizeof(int16_t) * mf4h.seg_count_2);
	fread(mf4h.id_delta, sizeof(int16_t), mf4h.seg_count_2, file);
	long length = (SWAP_ENDIAN_WORD(mf4h.length) -
		16 + (mf4h.seg_count_2 << 3)) >> 1;
	mf4h.id_range_offset = malloc(sizeof(uint16_t) * length);
	fread(mf4h.id_range_offset, sizeof(uint16_t), length, file);

	// allocate space for the table
	ttfobj->glyph_table = malloc(sizeof(uint32_t) * TTF_GLYPH_TBL_SIZE);
	for (i = 0; i < mf4h.seg_count_2; i++) {
		mf4h.end_count[i] = SWAP_ENDIAN_WORD(mf4h.end_count[i]);
		mf4h.start_count[i] = SWAP_ENDIAN_WORD(mf4h.start_count[i]);
		mf4h.id_delta[i] = SWAP_ENDIAN_WORD(mf4h.id_delta[i]);
	}
	for (i = 0; i < length; i++) {
		mf4h.id_range_offset[i] = SWAP_ENDIAN_WORD(mf4h.id_range_offset[i]);
	}
	for (uint32_t c = 0; c < TTF_GLYPH_TBL_SIZE; c++) {
		for (i = 0; i < mf4h.seg_count_2; i++) {
			if (mf4h.end_count[i] >= c) {
				if (mf4h.start_count[i] > c) {
					//mf4h.start_count[i] måste vara <= c
					ttfobj->glyph_table[c] = 0;
					break;
				}

				if (mf4h.id_range_offset[i] != 0) {
					ttfobj->glyph_table[c] =
						*((mf4h.id_range_offset[i] >> 1)
						+ (c - mf4h.start_count[i])
						+ &mf4h.id_range_offset[i]);
					if (ttfobj->glyph_table[c] != 0)
						ttfobj->glyph_table[c] += mf4h.id_delta[i];
					break;
				} else {
					ttfobj->glyph_table[c] = mf4h.id_delta[i] + c;
					break;
				}
			}
		}
		// is the last segment 0xFFFF always zero?
		if (i == mf4h.seg_count_2-1)
			ttfobj->glyph_table[c] = 0;
	}
	free(mf4h.end_count);
	free(mf4h.start_count);
	free(mf4h.id_delta);
	free(mf4h.id_range_offset);

	// load loca - index to location
	ttf_seek_header(file, &loca);
	uint32_t* indextolocation = malloc(sizeof(uint32_t) * (ttfobj->nglyphs + 1));
	fh.index_to_loc_format = SWAP_ENDIAN_WORD(fh.index_to_loc_format);
	if (!fh.index_to_loc_format) {
		// short offsets
		uint16_t* indextolocation2 = malloc(sizeof(uint16_t) * (ttfobj->nglyphs + 1));
		fread(indextolocation2, sizeof(uint16_t), (ttfobj->nglyphs+1), file);
		for (i = 0; i <= ttfobj->nglyphs; i++)
			indextolocation[i] = (uint32_t)
				(((0xFF00 & indextolocation2[i]) >> 8) |
				 ((0x00FF & indextolocation2[i]) << 8)) << 1;
		free(indextolocation2);
	} else {
		// long offsets
		fread(indextolocation, sizeof(uint32_t), (ttfobj->nglyphs+1), file);
		for (i = 0; i <= ttfobj->nglyphs; i++)
			indextolocation[i] = SWAP_ENDIAN_DWORD(indextolocation[i]);
	}

	// load glyf - glyphs
	ttf_seek_header(file, &glyf);
	ttfobj->glyph_data = malloc(sizeof(ttf_glyph_data_t) * ttfobj->nglyphs);
	ttf_glyph_header_t gh;
	for (i = 0; i < ttfobj->nglyphs; i++) {
		if (indextolocation[i] == indextolocation[i+1]) {
			ttfobj->glyph_data[i].npoints = 0;
			ttfobj->glyph_data[i].endpoints = NULL;
			ttfobj->glyph_data[i].ncontours = 0;
			ttfobj->glyph_data[i].px = NULL;
			ttfobj->glyph_data[i].py = NULL;
			ttfobj->glyph_data[i].state = NULL;
			ttf_set_ls_aw(ttfobj, &gh, &ttfobj->glyph_data[i], i);
			continue;
		}
		fseek(file, glyf.offset + indextolocation[i], SEEK_SET);
		ttf_read_gh(file, &gh);
		ttf_read_glyph(ttfobj, file, &gh,
				&ttfobj->glyph_data[i],
				&glyf, indextolocation, i);
	}
	free(indextolocation);

	return ttfobj;
}

/*
 * Read glyphs.
 *
 * Här laddas de enskilda tecknen "glyphs".
 * Sammansatta tecken är t.ex. ä, sammansatt av ¨ och a i de
 * flesta typsnitt. Detta är dock helt beroende av vad tillverkaren
 * tycker är lämpligt.
 *
 * Alla konverteringar är från TrueType-format till plattformens format.
 * Lägg märke till att endian varierar mellan olika plattformer.
*/
void ttf_read_glyph(ttf_t*		ttfobj,
		FILE*			file,
		ttf_glyph_header_t*	gh,
		ttf_glyph_data_t*	gd,
		ttf_table_header_t*	glyf,
		uint32_t*		indextolocation,
		uint32_t		i)
{
	if (gh->number_of_contours < 0) {
		// load composite glyph
		uint16_t		contour;
		uint16_t		flags;
		uint16_t		glyph_index;
		uint16_t		xoff;
		uint16_t		yoff;
		uint16_t		npoints = 0;
		uint16_t		ncontours = 0;
		uint32_t		readpos;
		ttf_glyph_data_t* 	component;
		ttf_gd_list_t*		components = NULL;
		bool			norigmtx = false;
		do {
			fread(&flags, sizeof(flags), 1, file);
			flags = SWAP_ENDIAN_WORD(flags);
			fread(&glyph_index, sizeof(glyph_index), 1, file);
			glyph_index = SWAP_ENDIAN_WORD(glyph_index);

			//Load subglyph{
				readpos = ftell(file);
				fseek(file, glyf->offset + indextolocation[glyph_index], SEEK_SET);
				ttf_read_gh(file, gh);
				component = malloc(sizeof(*component));
				ttf_read_glyph(ttfobj, file, gh, component,
						glyf, indextolocation, glyph_index);
				ttf_gd_list_add(&components, component);
				fseek(file, readpos, SEEK_SET);
			//}
			
			// flag out of range check
			assert(flags < 0x800);

			if (flags & TTF_WORD_ARGUMENTS) {
				fread(&xoff, sizeof(xoff), 1, file);
				xoff = SWAP_ENDIAN_WORD(xoff);
				fread(&yoff, sizeof(yoff), 1, file);
				yoff = SWAP_ENDIAN_WORD(yoff);
			} else {
				uint16_t tmp;
				fread(&tmp, sizeof(tmp), 1, file);
				yoff = 0xFF & tmp;
				xoff = tmp >> 8;
			}
			if (flags & TTF_ARGUMENTS_ARE_XY) {
				for (contour = 0; contour < component->npoints; contour++) {
					component->px[contour] += xoff;
					component->py[contour] += yoff;
				}
			}
			if (flags & TTF_SCALE) {
				float		scale;
				uint16_t	f2dot14;
				fread(&f2dot14, sizeof(f2dot14), 1, file);
				f2dot14 = SWAP_ENDIAN_WORD(f2dot14);
				//convert f2dot14 to float
				scale = (0xC000 & f2dot14) >> 14;
				f2dot14 &= 0x3FFF;
				scale += ((float) f2dot14) / 16383.f; //16383 == 0x3FFF
				for (contour = 0; contour < component->npoints; contour++) {
					component->px[contour] = (int16_t)(component->px[contour] * scale);
					component->py[contour] = (int16_t)(component->py[contour] * scale);
				}
			} else if (flags & TTF_XY_SCALE) {
				float		xScale;
				float		yScale;
				uint16_t	f2dot14;
				fread(&f2dot14, sizeof(f2dot14), 1, file);
				f2dot14 = SWAP_ENDIAN_WORD(f2dot14);
				//convert f2dot14 to float
				xScale = (0xC000 & f2dot14) >> 14;
				f2dot14 &= 0x3FFF;
				xScale += ((float) f2dot14) / 16383.f; //16383 == 0x3FFF
				fread(&f2dot14, sizeof(f2dot14), 1, file);
				f2dot14 = SWAP_ENDIAN_WORD(f2dot14);
				//convert f2dot14 to float
				yScale = (0xC000 & f2dot14) >> 14;
				f2dot14 &= 0x3FFF;
				yScale += ((float) f2dot14) / 16383.f;
				for (contour = 0; contour < component->npoints; contour++)
				{
					component->px[contour] = (int16_t)(component->px[contour] * xScale);
					component->py[contour] = (int16_t)(component->py[contour] * yScale);
				}
			} else if (flags & TTF_MATRIX2) {
				float	transform_11;
				float	transform_12;
				float	transform_21;
				float	transform_22;

				uint16_t f2dot14;
				fread(&f2dot14, sizeof(f2dot14), 1, file);
				f2dot14 = SWAP_ENDIAN_WORD(f2dot14);
				//convert f2dot14 to float
				transform_11 = (0xC000 & f2dot14) >> 14;
				f2dot14 &= 0x3FFF;
				transform_11 += ((float) f2dot14) / 16383.f; //16383 == 0x3FFF
				fread(&f2dot14, sizeof(f2dot14), 1, file);
				f2dot14 = SWAP_ENDIAN_WORD(f2dot14);
				//convert f2dot14 to float
				transform_12 = (0xC000 & f2dot14) >> 14;
				f2dot14 &= 0x3FFF;
				transform_12 += ((float) f2dot14) / 16383.f;
				fread(&f2dot14, sizeof(f2dot14), 1, file);
				f2dot14 = SWAP_ENDIAN_WORD(f2dot14);
				//convert f2dot14 to float
				transform_21 = (0xC000 & f2dot14) >> 14;
				f2dot14 &= 0x3FFF;
				transform_21 += ((float) f2dot14) / 16383.f;
				fread(&f2dot14, sizeof(f2dot14), 1, file);
				f2dot14 = SWAP_ENDIAN_WORD(f2dot14);
				//convert f2dot14 to float
				transform_22 = (0xC000 & f2dot14) >> 14;
				f2dot14 &= 0x3FFF;
				transform_22 += ((float) f2dot14) / 16383.f;
				for (contour = 0; contour < component->npoints; contour++) {
					component->px[contour] = (int16_t)(component->px[contour] * transform_11 + component->py[contour] * transform_21);
					component->py[contour] = (int16_t)(component->py[contour] * transform_12 + component->px[contour] * transform_22);
				}
			}

			if (flags & TTF_USE_THESE_METRICS) {
				norigmtx = true;
				ttf_set_ls_aw(ttfobj, gh, gd, glyph_index);
			}
			npoints += component->npoints;
			ncontours += component->ncontours;
		} while (flags & TTF_MORE_COMPONENTS);
		gd->npoints = npoints;
		gd->ncontours = ncontours;

		//use the components list to create a composite glyph
		uint16_t*	endpoints = malloc(sizeof(uint16_t)*ncontours);
		bool*		state = malloc(sizeof(bool)*npoints);
		int16_t*	px = malloc(sizeof(int16_t)*npoints);
		int16_t*	py = malloc(sizeof(int16_t)*npoints);
		uint16_t	point_off = 0;
		uint16_t	contour_off = 0;
		uint16_t	currentPoint;
		int		ccoff;
		int		coff;
		int		olim;
		bool		firstrun;

		memset(endpoints, 0, ncontours << 1);

		ttf_gd_list_t* p = components;
		int i = 0;
		while (p != NULL) {
			component = p->data;
			olim = component->ncontours - 1;
			firstrun = true;
			for (ccoff = contour_off, coff = 0; ccoff < ncontours; ccoff++) {
				if (coff < olim) {
					endpoints[ccoff] += component->endpoints[coff++];
				} else {
					if (firstrun) {
						endpoints[ccoff] += component->endpoints[coff];
						firstrun = false;
					} else {
						endpoints[ccoff] += component->endpoints[coff] + 1;
					}
				}
			}
			currentPoint = component->npoints << 1;
			memcpy(&px[point_off], component->px,
				currentPoint);
			memcpy(&py[point_off], component->py,
				currentPoint);
			memcpy(&state[point_off], component->state,
				currentPoint >> 1);
			point_off += component->npoints;
			contour_off += component->ncontours;
			free(component);

			++i;
			p = p->succ;
		}
		gd->px = px;
		gd->py = py;
		gd->state = state;
		gd->endpoints = endpoints;
		if (norigmtx) return;
		ttf_set_ls_aw(ttfobj, gh, gd, i);
		return;
	}

	//Load simple glyph
	ttf_simpglyphinfo_t simple;
	simple.endpoints_contour =
		malloc(sizeof(uint16_t) * gh->number_of_contours);
	fread(simple.endpoints_contour, sizeof(uint16_t), gh->number_of_contours, file);
	//calculate the number of points in the glyph
	int j, numpoints = 0;
	for (j = 0; j < gh->number_of_contours; j++) {
		simple.endpoints_contour[j] = SWAP_ENDIAN_WORD(simple.endpoints_contour[j]);
		if (simple.endpoints_contour[j] > numpoints)
			numpoints = simple.endpoints_contour[j];
	}
	numpoints++;
	fread(&simple.instruction_length, sizeof(simple.instruction_length), 1, file);
	if (numpoints <= 0)
	{
		return;
	}

	// read all the flags
	simple.instruction_length = SWAP_ENDIAN_WORD(simple.instruction_length);
	simple.instructions = malloc(sizeof(uint8_t) * simple.instruction_length);
	fread(simple.instructions, sizeof(uint8_t), simple.instruction_length, file);
	simple.flags = malloc(sizeof(uint8_t)*numpoints);
	uint8_t*	flagbuff = malloc(sizeof(uint8_t)*numpoints);
	uint16_t	flagind = 0;
	uint32_t	flagpos = ftell(file);
	uint8_t		tmpb;
	uint8_t		tmpb2;
	fread(flagbuff, sizeof(uint8_t), numpoints, file);
	j = 0;
	while (j < numpoints) {
		tmpb = flagbuff[flagind++];
		simple.flags[j++] = tmpb;
		if (tmpb & TTF_FLAG_REPEAT) {
			tmpb2 = flagbuff[flagind++];
			for (unsigned char rt = 0; rt < tmpb2; rt++)
				simple.flags[j++] = tmpb;
		}
	}
	fseek(file, flagpos+flagind, SEEK_SET);
	int16_t* px = malloc(sizeof(int16_t) * numpoints);
	int16_t* py = malloc(sizeof(int16_t) * numpoints);
	bool* state = malloc(sizeof(bool) * numpoints);
	/*
		Read in the maximum ammount of data needed for the
		points. This method should be faster than just
		reading point by point.
	*/
	uint8_t* pointbuff = malloc(sizeof(uint8_t) * (numpoints << 2));
	fread(pointbuff, sizeof(uint8_t), numpoints << 2, file);
	uint32_t pointind = 0;
	bool is_short = false;
	bool is_prev = false;
	int16_t last_point = 0;
	//xpass
	for (j = 0; j < numpoints; j++) {
		//on-curve check {
			if (simple.flags[j] & TTF_ON_CURVE) {
				state[j] = true;
			} else {
				state[j] = false;
			}
		//}
		if (simple.flags[j] & TTF_XSHORT) {
			//x-short
			is_short = true;
		}
		if (simple.flags[j] & TTF_XREPEAT) {
			//x-prev
			is_prev = true;
		}
		if (is_short) {
			if (is_prev) {
				last_point += pointbuff[pointind++];
				px[j] = last_point;
			} else {
				last_point -= pointbuff[pointind++];
				px[j] = last_point;
			}
		} else {
			if (is_prev) {
				px[j] = last_point;
			} else {
				last_point += (int16_t)(pointbuff[pointind+1] | (pointbuff[pointind] << 8));
				pointind += 2;
				px[j] = last_point;
			}
		}
		//reset
		is_short = is_prev = false;
	}
	//ypass
	last_point = 0;
	for (j = 0; j < numpoints; j++) {
		if (simple.flags[j] & TTF_YSHORT) {
			//y-short
			is_short = true;
		}
		if (simple.flags[j] & TTF_YREPEAT) {
			//y-prev
			is_prev = true;
		}
		if (is_short) {
			if (is_prev) {
				last_point += pointbuff[pointind++];
				py[j] = last_point;
			} else {
				last_point -= pointbuff[pointind++];
				py[j] = last_point;
			}
		} else {
			if (is_prev) {
				py[j] = last_point;
			} else {
				last_point += (int16_t)(pointbuff[pointind+1] | (pointbuff[pointind] << 8));
				pointind += 2;
				py[j] = last_point;
			}
		}
		//reset
		is_short = is_prev = false;
	}
	free(pointbuff);
	//save information to the GlyphData struct obj.
	gd->npoints = numpoints;
	gd->endpoints = simple.endpoints_contour;
	gd->ncontours = gh->number_of_contours;
	gd->px = px;
	gd->py = py;
	gd->state = state;
	ttf_set_ls_aw(ttfobj, gh, gd, i);
	free(simple.flags);
}

void ttf_export_chr_shape(ttf_t* ttfobj, uint16_t chr, shape_t* shape)
{
	if (ttfobj->interpolation_level) {
		// interpolate the curves
		vector_t*	points;
		uint16_t*	endpoints;

		ttf_interpolate(ttfobj, chr, &points, &endpoints,
				100.f/(float)ttfobj->upem);

		uint16_t	lim = 0;
		uint16_t	e;
		uint16_t	p = 0;
		//uint16_t	p2 = 0;
		uint16_t	origin;
		for (e = 0; e < ttfobj->glyph_data[ttfobj->glyph_table[chr]].ncontours; e++) {
			lim += endpoints[e];
			origin = p;
			for (; p < lim; p++) {
				shape_add_vec(shape, points[p].x, points[p].y);
                		if (p < (lim - 1))
					shape_add_seg(shape, p, p+1);
                		else
					shape_add_seg(shape, p, origin);
			}
		}
		free(points);
		free(endpoints);
	}
}

static uint16_t ttf_interpolate_chr(
		ttf_t*		ttfobj,
		uint16_t	chr,
		vector_t*	cpoints,
		vector_t*	points,
		uint16_t*	cpind,
		uint16_t	e)
{
	/*
 	 * This is a quadric Bezier curve interpolation algorithm using a
 	 * modified version of the de Casteljau algorithm to de-compose the
 	 * curve so that the actual interpolation goes faster.
 	 */
	
	bool*		states = ttfobj->glyph_data[ttfobj->glyph_table[chr]].state;
	uint16_t	pind;
	if (e)
		pind = ttfobj->glyph_data[ttfobj->glyph_table[chr]].endpoints[e - 1] + 1;
	else
		pind = 0;
	uint16_t firstpoint = pind;
	uint16_t lastpoint = ttfobj->glyph_data[ttfobj->glyph_table[chr]].endpoints[e];

	// if any state is true that means that the current point is on the curve
	bool ls = states[lastpoint], cs, ns = states[firstpoint];
	vector_t	lp = points[lastpoint];
	vector_t	cp;
	vector_t	np = points[firstpoint];

	uint16_t	c;
	float		cx;
	float		cy;
	float		dx;
	float		ddx;
	float		dy;
	float		ddy;
	float		oa, ob, oc, oo1, oo2;
	float		m = 1.f / ((float) ttfobj->interpolation_level);
	float		mm = m * m;

	oa = mm - 2.f*m;
	ob = 2.f*m - 2.f*mm;
	oc = mm;
	oo1 = 2.f*mm;
	oo2 = -4.f*mm;
	vector_t	ip1, ip2;
	uint16_t addon = *cpind;
	bool cont = true;
	do {
		cs = ns;
		cp = np;
		pind++;
		if (pind > lastpoint) {
			ns = states[firstpoint];
			np = points[firstpoint];
			cont = false;
		} else {
			ns = states[pind];
			np = points[pind];
		}
		if (!ls) {
			if (!cs) {
				if (!ns) {
					//		off-off-off
					ip1.x = (lp.x + cp.x)/2.f;
					ip1.y = (lp.y + cp.y)/2.f;
					ip2.x = (cp.x + np.x)/2.f;
					ip2.y = (cp.y + np.y)/2.f;
					dx = ip1.x * oa + cp.x * ob + ip2.x * oc;
					dy = ip1.y * oa + cp.y * ob + ip2.y * oc;
					ddx = ip1.x * oo1 + cp.x * oo2 + ip2.x * oo1;
					ddy = ip1.y * oo1 + cp.y * oo2 + ip2.y * oo1;
					cx = ip1.x;
					cy = ip1.y;
					for (c = 0; c < ttfobj->interpolation_level; c++) {
						cpoints[*cpind].x = cx;
						cpoints[(*cpind)++].y = cy;
						cx += dx;
						cy += dy;
						dx += ddx;
						dy += ddy;
					}
				} else {
					//		off-off-on
					ip1.x = (lp.x + cp.x)/2.f;
					ip1.y = (lp.y + cp.y)/2.f;
					dx = ip1.x * oa + cp.x * ob + np.x * oc;
					dy = ip1.y * oa + cp.y * ob + np.y * oc;
					ddx = ip1.x * oo1 + cp.x * oo2 + np.x * oo1;
					ddy = ip1.y * oo1 + cp.y * oo2 + np.y * oo1;
					cx = ip1.x;
					cy = ip1.y;
					for (c = 0; c < ttfobj->interpolation_level; c++) {
						cpoints[*cpind].x = cx;
						cpoints[(*cpind)++].y = cy;
						cx += dx;
						cy += dy;
						dx += ddx;
						dy += ddy;
					}
				}
			} else {
				if (ns) {
					cpoints[(*cpind)++] = cp;
				}
			}
		} else {
			if (!cs) {
				if (!ns) {
					//		on-off-off
					ip2.x = (cp.x + np.x)/2.f;
					ip2.y = (cp.y + np.y)/2.f;
					dx = lp.x * oa + cp.x * ob + ip2.x * oc;
					dy = lp.y * oa + cp.y * ob + ip2.y * oc;
					ddx = lp.x * oo1 + cp.x * oo2 + ip2.x * oo1;
					ddy = lp.y * oo1 + cp.y * oo2 + ip2.y * oo1;
					cx = lp.x;
					cy = lp.y;
					for (c = 0; c < ttfobj->interpolation_level; c++) {
						cpoints[*cpind].x = cx;
						cpoints[(*cpind)++].y = cy;
						cx += dx;
						cy += dy;
						dx += ddx;
						dy += ddy;
					}
				} else {
					//		on-off-on
					dx = lp.x * oa + cp.x * ob + np.x * oc;
					dy = lp.y * oa + cp.y * ob + np.y * oc;
					ddx = lp.x * oo1 + cp.x * oo2 + np.x * oo1;
					ddy = lp.y * oo1 + cp.y * oo2 + np.y * oo1;
					cx = lp.x;
					cy = lp.y;
					for (c = 0; c < ttfobj->interpolation_level; c++) {
						cpoints[*cpind].x = cx;
						cpoints[(*cpind)++].y = cy;
						cx += dx;
						cy += dy;
						dx += ddx;
						dy += ddy;
					}
				}
					
			} else {
				if (ns) {
					cpoints[(*cpind)++] = cp;
				}
			}
		}
		ls = cs;
		lp = cp;
	} while (cont);
	return *cpind - addon;
}

/*
 * LSB == Left Side Bound
 * AW == Advance Width
 */
void ttf_set_ls_aw(ttf_t* ttfobj,
		ttf_glyph_header_t* gh,
		ttf_glyph_data_t* gd,
		uint16_t i)
{
	int16_t	xmin;
	int16_t	xmax;

	xmin = gh->xmin;
	xmax = gh->xmax;

	gd->maxwidth = xmax - xmin;
	if (ttfobj->nhmtx > i) {
		if (ttfobj->zerolsb) {
			gd->lsb = 0;
			gd->aw = SWAP_ENDIAN_WORD(ttfobj->plhmtx[i].aw);
			return;
		}
		gd->lsb = xmin - SWAP_ENDIAN_WORD(ttfobj->plhmtx[i].lsb);
		gd->aw = SWAP_ENDIAN_WORD(ttfobj->plhmtx[i].aw);
		return;
	}

	if (ttfobj->zerolsb) {
		gd->lsb = 0;
		gd->aw = SWAP_ENDIAN_WORD(ttfobj->plhmtx[ttfobj->nhmtx-1].aw);
		return;
	}
	gd->lsb = xmin - SWAP_ENDIAN_WORD(ttfobj->plsb[i-ttfobj->nhmtx]);
	gd->aw = SWAP_ENDIAN_WORD(ttfobj->plhmtx[ttfobj->nhmtx-1].aw);
}

/*
 * Transform the interpolated coordinates to the correct unit.
 */
void ttf_interpolate(
		ttf_t*		ttfobj,
		uint16_t	chr,		// character code
		vector_t**	points,
		uint16_t**	endpoints,
		float		scale)
{
	uint16_t	contour;
	uint16_t	point;
	uint16_t	cpind = 0;

	vector_t*		cpoints;
	ttf_glyph_data_t*	glyph;

	glyph = &(ttfobj->glyph_data[ttfobj->glyph_table[chr]]);
	cpoints = malloc(sizeof(vector_t) * glyph->npoints);
	*points = malloc(sizeof(vector_t) * glyph->npoints * ttfobj->interpolation_level);
	*endpoints = malloc(sizeof(uint16_t) * glyph->ncontours);

	for (contour = 0, point = 0; contour < glyph->ncontours; contour++) {
		for (; point <= glyph->endpoints[contour]; point++) {
			cpoints[point].x = scale * (glyph->px[point] - glyph->lsb);
			cpoints[point].y = scale * glyph->py[point];
		}
		(*endpoints)[contour] = ttf_interpolate_chr(
				ttfobj, chr, *points,
				cpoints, &cpind, contour);
	}
	free(cpoints);
}
